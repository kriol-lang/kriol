#ifndef _KRIOL_CODEGEN_HEADER
#define _KRIOL_CODEGEN_HEADER

#include "ast.hh"
#include "prelude.hh"

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/STLFunctionalExtras.h>

namespace kriol {
namespace ast {

    enum class CodegenTarget {
        Native,
        Wasm32Wasi,
        X86_64Windows
    };

    struct EmitOptions {
        CodegenTarget Target = CodegenTarget::Native;
        unsigned OptLevel = 2;
    };

    class CodeGenVisitor : public Visitor {
    private:
        llvm::LLVMContext Context;
        std::unique_ptr<llvm::Module> Mod;
        std::unique_ptr<llvm::IRBuilder<>> Builder;

        // Result register for expression visits
        llvm::Value* LastValue = nullptr;

        // Currently-emitting function
        llvm::Function* CurrentFunction = nullptr;
        Type CurrentReturnType;
        bool CurrentCanFail = false;
        bool CurrentIsMain = false;
        // Where a function returning an array writes it: its hidden first argument.
        llvm::Value* CurrentReturnSlot = nullptr;

        // Loop exit / continue targets (for break / continue)
        llvm::BasicBlock* LoopExit     = nullptr;
        llvm::BasicBlock* LoopContinue = nullptr;

        // Scope stack: variable name -> AllocaInst*
        std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> Scopes;

        // Module-scope globals: variable name -> GlobalVariable*
        std::unordered_map<std::string, llvm::GlobalVariable*> GlobalVars;

        struct FuncSig {
            Type retType;
            std::vector<Type> paramTypes;
        };

        std::unordered_map<std::string, FuncSig> FunctionSigs;

        struct RecordInfo {
            std::vector<ast::VarDeclSttmt*> fields;
            std::unordered_map<std::string, std::size_t> fieldIndex;
            llvm::StructType* llvmType = nullptr;
        };

        std::unordered_map<std::string, RecordInfo> Records;
        std::unique_ptr<ast::MoldaDeclSttmt> ErrorTypeDecl = prelude::makeErrorTypeDecl();

        struct LValue {
            llvm::Value* Ptr = nullptr;
            llvm::Type* Type = nullptr;
        };

        // Non-constant global initializers deferred until inisiu's preamble
        struct DeferredGlobalInit {
            llvm::GlobalVariable*  Var;
            // non-owning; AST outlives codegen
            kriol::ast::Expr*      InitExpr;
            Type                   TargetType;
        };
        std::vector<DeferredGlobalInit> DeferredGlobalInits;

        llvm::Type*          mapType(const Type& kriolType);
        llvm::StructType*    getOrCreateRecordType(const std::string& name);
        llvm::AllocaInst*    createEntryAlloca(llvm::Function* fn,
                                               const std::string& name,
                                               llvm::Type* ty);
        llvm::AllocaInst*    lookupVar(const std::string& name);
        llvm::GlobalVariable* lookupGlobal(const std::string& name);
        llvm::Value*         getVariableStorage(const std::string& name);
        llvm::Value*         createArrayElementPtr(llvm::Value* storage,
                               llvm::Type* arrayTy,
                               llvm::Value* index);
        // `allowTemporary` lets a read through a non-addressable base, such as
        // `f().v[0]`, spill the base into a temporary.
        LValue               resolveLValue(ast::Expr* expr, bool allowTemporary = false);
        llvm::Function*      getOrDeclareKriolCheckBounds();
        llvm::Function*      getOrDeclareKriolCheckDiv();
        llvm::Function*      getOrDeclareKriolCheckFloatDiv();
        llvm::Function*      getOrDeclarePanicAt();

        // After emitting a block terminator (break/continue/return/sai) moves
        // the insert point into a fresh unreachable block so that any further
        // statements in the source block still produce valid (dead) IR.
        void startDeadBlock();

        // Emits a runtime divisor check before a division or remainder: zero
        // for every type, and MIN / -1 overflow for signed integers.
        void emitDivGuard(llvm::Value* lhs, llvm::Value* rhs,
                          const Type& operandType, int lineNum);

        // `lhs op rhs` for an arithmetic or bitwise operator ("+", "/", "&", ...),
        // with both operands already of `operandType`.
        llvm::Value* emitArithmetic(const std::string& op, llvm::Value* lhs, llvm::Value* rhs,
                                    const Type& operandType, int lineNum);

        // Emits short-circuit evaluation for '&&' and '||'.
        void emitShortCircuit(ast::BinExpr& node);

        // A function that can fail returns { i1 failed, <value>, Erru }, with
        // the value omitted when the function returns nothing.
        llvm::StructType* failableResultType(const Type& valueType);
        llvm::Type* functionReturnType(const ast::FuncDeclSttmt& node);

        // Aggregates (arrays and molda) travel by address: an expression of
        // aggregate type evaluates to a pointer to its storage, an aggregate
        // parameter is a pointer the callee copies, and a function returning an
        // aggregate writes it into a buffer the caller passes as a hidden first
        // argument. Large aggregates as LLVM values make the backend crawl.
        static bool isAggregate(const Type& type) { return type.isArray() || type.isNamed(); }
        llvm::Type* abiType(const Type& type);
        // The address of an aggregate value, materializing a literal in a temporary.
        llvm::Value* emitAggregateAddress(ast::Expr* expr, const Type& type);
        // Stores the aggregate value of `expr` into `dest`, building literals in place.
        void emitAggregateStore(llvm::Value* dest, const Type& type, ast::Expr* expr);
        void emitAggregateCopy(llvm::Value* dest, llvm::Value* src, const Type& type);
        void emitRecordLiteral(llvm::Value* dest, ast::RecordLiteralExpr& node);

        // The failable result of a built-in call: `message` is the runtime's
        // error text, read only when `failed` is true.
        llvm::Value* makeFailableResult(const Type& valueType, llvm::Value* failed,
                                        llvm::Value* value, llvm::Value* message);
        llvm::Value* erruMessage(llvm::Value* erru);

        // Fails the current function with the given Erru value: returns it to
        // the caller, or reports it and exits when the function is inisiu.
        // Ends the current block with a terminator.
        void emitFailure(llvm::Value* erruValue);

        // For a call whose error nobody handles: stops the program with the
        // error message if the call failed, and leaves the plain value.
        void emitUnhandledFailureCheck(ast::FunCallExpr& node);

        // 'T::konverti(text)'.
        void emitConversionCall(ast::TypeCallExpr& node);

        // 'tenta call' and 'call sinon fallback'.
        void emitTenta(ast::UnaryExpr& node);
        void emitSinon(ast::BinExpr& node);

        void pushScope() { Scopes.push_back({}); }
        void popScope()  { if (!Scopes.empty()) Scopes.pop_back(); }
        void declareVar(const std::string& name, llvm::AllocaInst* a) {
            if (!Scopes.empty()) Scopes.back()[name] = a;
        }

        FuncSig makeFuncSig(const ast::FuncDeclSttmt& node) const;
        // The LLVM function for `node`, declared now if it does not exist yet.
        llvm::Function* declareFunction(const ast::FuncDeclSttmt& node);

        // Forward-declare a user function in the LLVM module (type + name, no body).
        // Called in the program-root pre-pass so mutual/forward calls resolve.
        void forwardDeclareFunc(ast::FuncDeclSttmt& node);
        void registerRecord(ast::MoldaDeclSttmt& node);

        // Central scalar coercion table: convert v to targetTy.
        // Supported pairs: int->num (SIToFP), bool->int (ZExt),
        // bool->num (UIToFP), num->int (saturating). Identity is a no-op.
        // Throws for unsupported or pointer conversions.
        llvm::Value* coerce(llvm::Value* v, llvm::Type* targetTy);
        llvm::Value* coerceToType(llvm::Value* v,
                                  const Type& sourceType,
                                  const Type& targetType);

        // Coerce value to i1 for use as a branch condition
        llvm::Value* toBool(llvm::Value* v);

        static std::string processEscapes(const std::string& raw);
        static const char* formatSpec(const Type& kriolType);
        static Type        llvmTypeToKriol(llvm::Type* ty);

        llvm::Value* emitArrayToText(llvm::Value* storage, const Type& arrayType);

        // Emits a loop calling `body` with the address of each element from `first` on.
        void emitArrayLoop(llvm::Value* storage, llvm::ArrayType* arrayTy, uint64_t first,
                           llvm::function_ref<void(llvm::Value*)> body);

        void emitArrayInitializer(llvm::Value* storage,
                                  const Type& arrayType,
                                  ast::Expr* init);
        bool emitPreludeCall(ast::FunCallExpr& node, const std::string& name);
        void emitPrintBuiltin(ast::FuncCallArgs* args, bool addNewline);

    public:
        CodegenTarget CurrentTarget = CodegenTarget::Native;

        explicit CodeGenVisitor(const std::string& moduleName);

        /// Serialise the module as LLVM IR text.
        std::string emitIR();

        /// Compile the module to the selected executable/object format.
        void emit(const std::string& outputPath, const EmitOptions& options = {});

        /// Compile the module and return the emitted bytes without keeping a file.
        std::vector<unsigned char> emitToMemory(const EmitOptions& options = {});

        void visit(VarDeclSttmt&      node) override;
        void visit(MoldaDeclSttmt&    node) override;
        void visit(BlockSttmt&        node) override;
        void visit(FuncArgs&          node) override;
        void visit(FuncDeclSttmt&     node) override;
        void visit(IfSttmt&           node) override;
        void visit(WhileSttmt&        node) override;
        void visit(JumpSttmt&         node) override;
        void visit(ReturnSttmt&       node) override;
        void visit(FuncCallArgs&      node) override;
        void visit(FunCallExpr&       node) override;
        void visit(BinExpr&           node) override;
        void visit(LiteralExpr&       node) override;
        void visit(ExprSttmt&         node) override;
        void visit(IdentExpr&         node) override;
        void visit(ParExpr&           node) override;
        void visit(ArrayAccessExpr&   node) override;
        void visit(MemberAccessExpr&  node) override;
        void visit(QualifiedAccessExpr& node) override;
        void visit(ArrayLiteralExpr&  node) override;
        void visit(ArrayRepeatExpr&   node) override;
        void visit(RecordLiteralExpr& node) override;
        void visit(AssignExpr&        node) override;
        void visit(ForSttmt&          node) override;
        void visit(ImportSttmt&       node) override;
        void visit(FStringExpr&       node) override;
        void visit(UnaryExpr&         node) override;
        void visit(CastExpr&          node) override;
        void visit(TypeCallExpr&      node) override;
    };

} // namespace ast
} // namespace kriol

#endif // _KRIOL_CODEGEN_HEADER
