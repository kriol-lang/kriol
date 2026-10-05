#ifndef _KRIOL_SEMA_HEADER
#define _KRIOL_SEMA_HEADER

#include "diagnostic.hh"
#include "ast.hh"
#include "constants.hh"
#include "prelude.hh"

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace kriol {
namespace sema {

    class SemanticAnalyzer : public ast::Visitor {
    private:
        // Scoped symbol table: each entry is one scope level (name -> Kriol type)
        std::vector<std::unordered_map<std::string, Type>> SymbolScopes;

        struct RecordInfo {
            std::vector<ast::VarDeclSttmt*> fields;
            std::unordered_map<std::string, std::size_t> fieldIndex;
        };

        // Signature record for a user-defined function
        struct FuncInfo {
            Type retType;
            std::vector<Type> paramTypes;
            bool canFail = false;
        };

        // Known user-defined functions (name -> signature)
        std::unordered_map<std::string, FuncInfo> FunctionTable;

        // Known record types (name -> fields), including the built-in Erru
        std::unordered_map<std::string, RecordInfo> RecordTable;
        std::unique_ptr<ast::MoldaDeclSttmt> ErrorTypeDecl = prelude::makeErrorTypeDecl();

        // Return type of the function currently being analysed ("" at top level)
        Type CurrFuncRetType;
        std::string CurrFuncName;

        // How many loops deep we currently are (used for break/continue validation)
        int LoopDepth = 0;

        // How many function bodies deep we currently are (nested functions are
        // not supported and must be rejected).
        int FunctionDepth = 0;

        // Whether the function being analysed declares an error type.
        bool CurrFuncCanFail = false;

        // The fallible call that the enclosing 'tenta' or 'sinon' handles;
        // any other fallible call is an unhandled error.
        const ast::FunCallExpr* HandledCall = nullptr;

        // The variable whose initializer is being analysed, which must not
        // read the variable itself.
        std::string InitializingVar;

        // Collected errors
        std::vector<std::string> Errors;

        // Collected warnings: problems that do not stop compilation
        std::vector<std::string> Warnings;

        // Source file name, used for error location prefixes
        std::string SourceFile;

        // The "file:line: erro: " and "file:line: aviso: " message prefixes.
        std::string errLoc(int lineNum) const { return DiagnosticPrefix(SourceFile, lineNum, "erro"); }
        std::string warnLoc(int lineNum) const { return DiagnosticPrefix(SourceFile, lineNum, "aviso"); }

        // --- scope helpers ---
        void pushScope() {
            SymbolScopes.push_back({});
        }
        void popScope()  {
            if (!SymbolScopes.empty()) SymbolScopes.pop_back();
        }

        void declareVar(const std::string& name, const Type& type) {
            if (!SymbolScopes.empty())
                SymbolScopes.back()[name] = type;
        }

        // Returns the Kriol type of the variable if found in any scope, or empty string.
        std::optional<Type> lookupVar(const std::string& name) const {
            for (auto it = SymbolScopes.rbegin(); it != SymbolScopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) return found->second;
            }
            return std::nullopt;
        }

        // Returns true if every reachable code path in the block ends with a
        // return or a call to paniku()/sai(). Conservative: of the branching
        // statements, only if/else with both branches returning counts.
        bool blockDefinitelyReturns(ast::BlockSttmt* block) const;

        // Returns true if assigning/returning `from` where `to` is expected
        // is a legal implicit widening (int->num, bool->int, bool->num).
        static bool isWideningCoercion(const Type& from, const Type& to);
        static bool integerLiteralFits(const ast::Expr* expr, const Type& to);
        static bool canCoerceExprTo(const ast::Expr* expr, const Type& to);
        static bool isPrintableType(const Type& type, bool allowArray);

        // Whether `op` is a bitwise binary operator ("&", "|" or "^").
        static bool isBitwiseOp(std::string_view op);

        void visitArgs(ast::FunCallExpr& node);

        // Reports an error unless argument `index` of `callee` has type `expected`.
        void expectArgType(ast::FunCallExpr& node, std::size_t index, const Type& expected,
                           const std::string& callee);

        // Reports an error if 'tenta' or 'lansa' is used outside a function
        // that declares an error type.
        void requireFallibleFunction(const std::string& keyword, int lineNum);

        // Reports an error if `name` is the variable whose initializer is
        // being analysed.
        void checkNotSelfInitialized(const std::string& name, int lineNum);

        // The call under any parentheses, or null if `expr` is not a call.
        static ast::FunCallExpr* unwrapCallExpr(ast::Expr* expr);

        // Visits the operand of 'tenta' or 'sinon' and checks it is a call to
        // a function that can fail. Returns false after reporting an error.
        bool visitFallibleOperand(ast::Expr* operand, const std::string& keyword, int lineNum);

        // Pre-registers a function's full signature into FunctionTable without
        // visiting the body. Called in the first pass of Check().
        void registerFuncSignature(ast::FuncDeclSttmt& node);

        // `builtin` skips the reserved-name check, which rejects user
        // declarations of built-in types.
        void registerRecord(ast::MoldaDeclSttmt& node, bool builtin = false);
        bool validateTypeKnown(const Type& type, int lineNum, const std::string& context);

        // Approximate storage size of a value of `type` (ignores padding),
        // saturating instead of overflowing for huge arrays.
        std::size_t storageBytes(const Type& type) const;

        // Validates that a condition expression has a type usable in a branch
        // (bool or numeric). Anything else cannot be lowered to a truth value.
        void checkConditionType(const ast::Expr* cond, int lineNum);

        // Validates that a literal's textual value actually fits the host
        // representation used during codegen (i64 / double), so codegen never
        // throws on std::stoll/std::stod.
        void validateLiteralRange(ast::LiteralExpr& node);
        bool validateArrayInitializer(const Type& expectedType,
                                      ast::Expr* init,
                                      int lineNum,
                                      const std::string& context);
        Type resolveAssignableType(ast::Expr* expr, int lineNum);


        // Checks if a name is reserved and cannot be declared (used for variables, parameters, functions).
        static bool isReservedKeyword(const std::string& name);

        // Checks if a name is reserved and cannot be declared, and if so adds an error.
        bool checkDeclaredNameValid(const std::string& name, const std::string& kind, int lineNum);

        // Visits a value stored into an array of type `expected`: an array
        // initializer is validated against it, any other expression must have
        // exactly that type. `context` names the destination in errors.
        bool visitArrayValue(ast::Expr* value, const Type& expected, int lineNum,
                             const std::string& context);

        // Adds an error message to the error list
        void addError(const std::string& msg) { Errors.push_back(msg); }
        void addWarning(const std::string& msg) { Warnings.push_back(msg); }

    public:
        SemanticAnalyzer() = default;

        // Entry point: walk the whole program AST and collect errors.
        void Check(ast::BlockSttmt* program);

        bool HasErrors() const { return !Errors.empty(); }
        const std::vector<std::string>& GetErrors() const { return Errors; }
        const std::vector<std::string>& GetWarnings() const { return Warnings; }

        // Sets the source filename included in error location prefixes.
        void SetSourceFile(const std::string& f) { SourceFile = f; }

        // --- visitor overrides ---
        void visit(ast::VarDeclSttmt&      node) override;
        void visit(ast::MoldaDeclSttmt&    node) override;
        void visit(ast::BlockSttmt&        node) override;
        void visit(ast::FuncArgs&          node) override;
        void visit(ast::FuncDeclSttmt&     node) override;
        void visit(ast::IfSttmt&           node) override;
        void visit(ast::WhileSttmt&        node) override;
        void visit(ast::JumpSttmt&         node) override;
        void visit(ast::ReturnSttmt&       node) override;
        void visit(ast::FuncCallArgs&      node) override;
        void visit(ast::FunCallExpr&       node) override;
        void visit(ast::BinExpr&           node) override;
        void visit(ast::LiteralExpr&       node) override;
        void visit(ast::ExprSttmt&         node) override;
        void visit(ast::IdentExpr&         node) override;
        void visit(ast::ParExpr&           node) override;
        void visit(ast::ArrayAccessExpr&   node) override;
        void visit(ast::MemberAccessExpr&  node) override;
        void visit(ast::QualifiedAccessExpr& node) override;
        void visit(ast::ArrayLiteralExpr&  node) override;
        void visit(ast::ArrayRepeatExpr&   node) override;
        void visit(ast::RecordLiteralExpr& node) override;
        void visit(ast::AssignExpr&        node) override;
        void visit(ast::ForSttmt&          node) override;
        void visit(ast::ImportSttmt&       node) override;
        void visit(ast::FStringExpr&       node) override;
        void visit(ast::UnaryExpr&         node) override;
        void visit(ast::CastExpr&          node) override;
        void visit(ast::TypeCallExpr&      node) override;
    };

} // namespace sema
} // namespace kriol

#endif // _KRIOL_SEMA_HEADER
