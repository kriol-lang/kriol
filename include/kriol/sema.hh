#ifndef _KRIOL_SEMA_HEADER
#define _KRIOL_SEMA_HEADER

#include "ast.hh"
#include "constants.hh"

#include <string>
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

        struct VarInitState {
            bool isArray = false;
            bool fullyInitialized = false;
            std::vector<bool> elementInitialized;
        };

        struct RecordInfo {
            std::vector<ast::VarDeclSttmt*> fields;
            std::unordered_map<std::string, std::size_t> fieldIndex;
        };

        // Scoped initialization table: each entry is one scope level
        // (name -> initialization state)
        std::vector<std::unordered_map<std::string, VarInitState>> InitScopes;

        // Signature record for a user-defined function
        struct FuncInfo {
            Type retType;
            std::vector<Type> paramTypes;
        };

        // Known user-defined functions (name -> signature)
        std::unordered_map<std::string, FuncInfo> FunctionTable;

        // Known user-defined record types (name -> fields)
        std::unordered_map<std::string, RecordInfo> RecordTable;

        // Return type of the function currently being analysed ("" at top level)
        Type CurrFuncRetType;
        std::string CurrFuncName;

        // How many loops deep we currently are (used for break/continue validation)
        int LoopDepth = 0;

        // How many function bodies deep we currently are (nested functions are
        // not supported and must be rejected).
        int FunctionDepth = 0;

        // Collected errors
        std::vector<std::string> Errors;

        // Source file name, used for error location prefixes
        std::string SourceFile;

        // Returns "file:line: " prefix for error messages, or "file: " if line is 0.
        std::string errLoc(int lineNum) const {
            if (lineNum == 0) return SourceFile.empty() ? "" : SourceFile + ": ";
            return (SourceFile.empty() ? "" : SourceFile + ":") + std::to_string(lineNum) + ": ";
        }

        // --- scope helpers ---
        void pushScope() {
            SymbolScopes.push_back({});
            InitScopes.push_back({});
        }
        void popScope()  {
            if (!SymbolScopes.empty()) SymbolScopes.pop_back();
            if (!InitScopes.empty()) InitScopes.pop_back();
        }

        void declareVar(const std::string& name, const Type& type) {
            if (!SymbolScopes.empty())
                SymbolScopes.back()[name] = type;
        }

        void declareInitState(const std::string& name, VarInitState state) {
            if (!InitScopes.empty())
                InitScopes.back()[name] = std::move(state);
        }

        // Returns the Kriol type of the variable if found in any scope, or empty string.
        std::optional<Type> lookupVar(const std::string& name) const {
            for (auto it = SymbolScopes.rbegin(); it != SymbolScopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) return found->second;
            }
            return std::nullopt;
        }

        const VarInitState* lookupInitState(const std::string& name) const {
            for (auto it = InitScopes.rbegin(); it != InitScopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) return &found->second;
            }
            return nullptr;
        }

        VarInitState* lookupInitStateMutable(const std::string& name) {
            for (auto it = InitScopes.rbegin(); it != InitScopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) return &found->second;
            }
            return nullptr;
        }

        // Returns true if every reachable code path in the block ends with a
        // return statement. Conservative: only if/else with both branches
        // returning is recognised as a definite return.
        bool blockDefinitelyReturns(ast::BlockSttmt* block) const;

        // Returns true if assigning/returning `from` where `to` is expected
        // is a legal implicit widening (nter->num, bool->nter, bool->num).
        static bool isWideningCoercion(const Type& from, const Type& to);
        static bool integerLiteralFits(const ast::Expr* expr, const Type& to);
        static bool canCoerceExprTo(const ast::Expr* expr, const Type& to);
        static bool isPrintableType(const Type& type, bool allowArray);

        // Pre-registers a function's full signature into FunctionTable without
        // visiting the body. Called in the first pass of Check().
        void registerFuncSignature(ast::FuncDeclSttmt& node);

        void registerRecord(ast::MoldaDeclSttmt& node);
        bool validateTypeKnown(const Type& type, int lineNum, const std::string& context);

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

        // If expr (after stripping ParExpr wrappers) is an array variable identifier,
        // validates its init state, annotates ResolvedType on both the IdentExpr and expr,
        // and returns true so the caller can skip its normal accept() dispatch.
        // Returns false for non-array or non-identifier expressions.
        // Also returns true (and emits an error) for undefined identifiers to avoid
        // a redundant second error from accept().
        bool handleArrayIdentArg(ast::Expr& expr);

        // Adds an error message to the error list
        void addError(const std::string& msg) { Errors.push_back(msg); }

    public:
        SemanticAnalyzer() = default;

        // Entry point: walk the whole program AST and collect errors.
        void Check(ast::BlockSttmt* program);

        bool HasErrors() const { return !Errors.empty(); }
        const std::vector<std::string>& GetErrors() const { return Errors; }

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
    };

} // namespace sema
} // namespace kriol

#endif // _KRIOL_SEMA_HEADER
