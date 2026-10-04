#include "../../include/kriol/codegen.hh"
#include "../../include/kriol/prelude.hh"

#include <llvm/IR/Attributes.h>

#include <stdexcept>

namespace kriol {
namespace ast {

namespace {

static llvm::Function* getOrDeclareRuntimePrint(llvm::Module& Mod,
                                                llvm::LLVMContext& Context,
                                                const std::string& suffix,
                                                llvm::Type* argTy,
                                                bool newline)
{
    std::string name = newline ? "__kriol_println_" + suffix : "__kriol_print_" + suffix;
    if (auto* fn = Mod.getFunction(name)) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {argTy}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, name, Mod);
}

static llvm::Function* getOrDeclarePutchar(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_print_char")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    auto* ftype = llvm::FunctionType::get(voidTy, {i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_print_char", Mod);
}

static llvm::Function* getOrDeclareRuntimeReadLine(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_read_line")) return fn;
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* ftype = llvm::FunctionType::get(ptrTy, {ptrTy, ptrTy}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_read_line", Mod);
}

static llvm::Function* getOrDeclareExit(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("exit")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i32Ty  = llvm::Type::getInt32Ty(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {i32Ty}, false);
    auto* fn = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "exit", Mod);
    fn->addFnAttr(llvm::Attribute::NoReturn);
    return fn;
}

static llvm::Function* getOrDeclareRuntimeAssert(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_assert")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i32Ty  = llvm::Type::getInt32Ty(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {i32Ty, i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_assert", Mod);
}

static llvm::Function* getOrDeclareRuntimeAssertMessage(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_assert_message")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i32Ty  = llvm::Type::getInt32Ty(Context);
    auto* ptrTy  = llvm::PointerType::getUnqual(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {i32Ty, i32Ty, ptrTy}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_assert_message", Mod);
}

static llvm::Function* getOrDeclareRuntimeBoolToString(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_bool_to_string")) return fn;
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    auto* ftype = llvm::FunctionType::get(ptrTy, {i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_bool_to_string", Mod);
}

static llvm::Function* getOrDeclareRuntimeArrayToText(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_array_to_text")) return fn;
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* i64Ty = llvm::Type::getInt64Ty(Context);
    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    auto* ftype = llvm::FunctionType::get(ptrTy, {ptrTy, i64Ty, i32Ty, i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_array_to_text", Mod);
}

// Element kinds understood by __kriol_array_to_text in the runtime.
enum class ArrayElementKind : int {
    Signed = 0,
    Unsigned = 1,
    Float = 2,
    Bool = 3,
    Text = 4
};

static llvm::Function* getOrDeclareRuntimeFormat(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_format")) return fn;
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* ftype = llvm::FunctionType::get(ptrTy, {ptrTy}, /*isVarArg=*/true);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_format", Mod);
}

} // namespace

llvm::Function* CodeGenVisitor::getOrDeclarePanicAt()
{
    if (auto* fn = Mod->getFunction("__kriol_panic_at")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i32Ty  = llvm::Type::getInt32Ty(Context);
    auto* ptrTy  = llvm::PointerType::getUnqual(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {ptrTy, i32Ty}, false);
    auto* fn = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_panic_at", *Mod);
    fn->addFnAttr(llvm::Attribute::NoReturn);
    return fn;
}

llvm::Value* CodeGenVisitor::emitArrayToText(llvm::Value* storage, const Type& arrayType) {
    const Type& element = arrayType.elementType();
    ArrayElementKind kind;
    unsigned bits = element.bitWidth();
    if (element.isInteger())
        kind = element.isSigned() ? ArrayElementKind::Signed : ArrayElementKind::Unsigned;
    else if (element.isFloat())
        kind = ArrayElementKind::Float;
    else if (element == Type::Bool())
        kind = ArrayElementKind::Bool;
    else if (element == Type::Text())
        kind = ArrayElementKind::Text;
    else
        throw std::runtime_error("internal error: cannot format an array of '" + element.str() + "'");

    return Builder->CreateCall(getOrDeclareRuntimeArrayToText(*Mod, Context), {
        storage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), arrayType.arraySize()),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), static_cast<int>(kind)),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), bits)
    }, "array.text");
}

bool CodeGenVisitor::emitPreludeCall(ast::FunCallExpr& node, const std::string& name) {
    switch (prelude::lookupBuiltin(name)) {
        case prelude::Builtin::Mostra:
            emitPrintBuiltin(node.Args.get(), false);
            return true;

        case prelude::Builtin::Mostran:
            emitPrintBuiltin(node.Args.get(), true);
            return true;

        case prelude::Builtin::Toma: {
            auto* ptrTy = llvm::PointerType::getUnqual(Context);
            llvm::Value* prompt = llvm::ConstantPointerNull::get(ptrTy);
            if (node.Args && !node.Args->Args.empty() && node.Args->Args[0]) {
                node.Args->Args[0]->accept(*this);
                prompt = LastValue ? LastValue : prompt;
            }

            // The runtime returns null and sets the message when the input ends
            // or cannot be read.
            auto* errorSlot = createEntryAlloca(CurrentFunction, "toma.error", ptrTy);
            Builder->CreateStore(llvm::ConstantPointerNull::get(ptrTy), errorSlot);
            llvm::Value* line = Builder->CreateCall(getOrDeclareRuntimeReadLine(*Mod, Context),
                                                    {prompt, errorSlot}, "toma");
            llvm::Value* message = Builder->CreateLoad(ptrTy, errorSlot, "toma.message");
            LastValue = makeFailableResult(Type::Text(), Builder->CreateIsNull(line, "toma.failed"),
                                           line, message);
            return true;
        }

        case prelude::Builtin::Sai: {
            auto* i32Ty = llvm::Type::getInt32Ty(Context);
            llvm::Value* code = llvm::ConstantInt::get(i32Ty, 0);
            if (node.Args && !node.Args->Args.empty() && node.Args->Args[0]) {
                node.Args->Args[0]->accept(*this);
                if (LastValue)
                    code = coerceToType(LastValue, node.Args->Args[0]->ResolvedType, Type::SignedInteger(32));
            }
            Builder->CreateCall(getOrDeclareExit(*Mod, Context), {code});
            Builder->CreateUnreachable();
            // Statements after sai() are unreachable; emit them into a fresh
            // dead block so the current block keeps a single terminator.
            startDeadBlock();
            LastValue = nullptr;
            return true;
        }

        case prelude::Builtin::Konfirma: {
            auto* i32Ty = llvm::Type::getInt32Ty(Context);
            llvm::Value* cond = llvm::ConstantInt::get(i32Ty, 1);
            if (node.Args && !node.Args->Args.empty() && node.Args->Args[0]) {
                node.Args->Args[0]->accept(*this);
                if (LastValue) {
                    llvm::Value* b = toBool(LastValue);
                    cond = Builder->CreateZExt(b, i32Ty, "konfirma_cond");
                }
            }
            llvm::Value* line = llvm::ConstantInt::get(i32Ty, node.LineNum);
            if (node.Args && node.Args->Args.size() > 1 && node.Args->Args[1]) {
                node.Args->Args[1]->accept(*this);
                if (LastValue) {
                    Builder->CreateCall(getOrDeclareRuntimeAssertMessage(*Mod, Context), {cond, line, LastValue});
                    LastValue = nullptr;
                    return true;
                }
            }
            Builder->CreateCall(getOrDeclareRuntimeAssert(*Mod, Context), {cond, line});
            LastValue = nullptr;
            return true;
        }

        case prelude::Builtin::Paniku: {
            auto* i32Ty = llvm::Type::getInt32Ty(Context);
            node.Args->Args[0]->accept(*this);
            llvm::Value* message = LastValue;
            if (!message)
                throw std::runtime_error("internal error: failed to generate the message of paniku");
            llvm::Value* line = llvm::ConstantInt::get(i32Ty, node.LineNum);
            Builder->CreateCall(getOrDeclarePanicAt(), {message, line});
            Builder->CreateUnreachable();
            // Like sai(): later statements go into a fresh dead block.
            startDeadBlock();
            LastValue = nullptr;
            return true;
        }

        case prelude::Builtin::None:
            return false;
    }

    return false;
}

void CodeGenVisitor::emitPrintBuiltin(ast::FuncCallArgs* argsNode, bool addNewline) {
    auto* ptrTy    = llvm::PointerType::getUnqual(Context);
    auto* i64Ty    = llvm::Type::getInt64Ty(Context);
    auto* doubleTy = llvm::Type::getDoubleTy(Context);
    auto* i32Ty    = llvm::Type::getInt32Ty(Context);
    auto* putcharFn = getOrDeclarePutchar(*Mod, Context);

    auto emitPrintValue = [&](llvm::Value* val, const Type& resolvedType, bool newline) {
        if (resolvedType.isInteger()) {
            Type printType = resolvedType.isSigned() ? Type::SignedInteger(64) : Type::UnsignedInteger(64);
            if (val->getType() != i64Ty) val = coerceToType(val, resolvedType, printType);
            Builder->CreateCall(getOrDeclareRuntimePrint(*Mod, Context, resolvedType.isSigned() ? "i64" : "u64", i64Ty, newline), {val});
        } else if (resolvedType.isFloat()) {
            if (val->getType() != doubleTy) val = coerceToType(val, resolvedType, Type::Float(64));
            Builder->CreateCall(getOrDeclareRuntimePrint(*Mod, Context, "f64", doubleTy, newline), {val});
        } else if (resolvedType == Type::Bool()) {
            llvm::Value* ext = val->getType()->isIntegerTy(1)
                ? Builder->CreateZExt(val, i32Ty, "bool_ext")
                : Builder->CreateTrunc(val, i32Ty, "bool_ext");
            Builder->CreateCall(getOrDeclareRuntimePrint(*Mod, Context, "bool", i32Ty, newline), {ext});
        } else {
            if (!val->getType()->isPointerTy()) val = Builder->CreateIntToPtr(val, ptrTy);
            Builder->CreateCall(getOrDeclareRuntimePrint(*Mod, Context, "string", ptrTy, newline), {val});
        }
    };

    if (!argsNode || argsNode->Args.empty()) {
        if (addNewline)
            Builder->CreateCall(putcharFn,
                                {llvm::ConstantInt::get(i32Ty, '\n')});
        LastValue = nullptr;
        return;
    }

    auto& args = argsNode->Args;
    size_t lastIdx = args.size() - 1;

    for (size_t i = 0; i < args.size(); ++i) {
        bool addNlHere = (i == lastIdx) && addNewline;
        auto& arg = args[i];

        if (arg->ResolvedType.isArray()) {
            LValue storage = resolveLValue(arg.get());
            if (!storage.Ptr)
                throw std::runtime_error("cannot print non-addressable array expression");

            llvm::Value* text = emitArrayToText(storage.Ptr, arg->ResolvedType);
            Builder->CreateCall(getOrDeclareRuntimePrint(*Mod, Context, "string", ptrTy, addNlHere), {text});
            continue;
        }

        arg->accept(*this);
        llvm::Value* v = LastValue;
        if (v) emitPrintValue(v, arg->ResolvedType, addNlHere);
    }

    LastValue = nullptr;
}

void CodeGenVisitor::visit(FStringExpr& node) {
    std::string fmtStr;
    std::vector<llvm::Value*> callArgs;

    for (auto& seg : node.Parts) {
        if (!seg.expr) {
            std::string text = processEscapes(seg.text);
            for (char c : text) {
                if (c == '%') fmtStr += '%';
                fmtStr += c;
            }
        } else {
            if (seg.expr->ResolvedType.isArray()) {
                LValue storage = resolveLValue(seg.expr.get());
                if (!storage.Ptr)
                    throw std::runtime_error("cannot interpolate non-addressable array expression");

                fmtStr += "%s";
                callArgs.push_back(emitArrayToText(storage.Ptr, seg.expr->ResolvedType));
                continue;
            }

            seg.expr->accept(*this);
            llvm::Value* val = LastValue;
            if (!val) continue;

            Type kriolType = seg.expr->ResolvedType;
            if (!kriolType.valid()) kriolType = llvmTypeToKriol(val->getType());

            if (kriolType == Type::Bool()) {
                auto* i32Ty = llvm::Type::getInt32Ty(Context);
                llvm::Value* ext = val->getType()->isIntegerTy(1)
                    ? Builder->CreateZExt(val, i32Ty) : val;
                val = Builder->CreateCall(
                    getOrDeclareRuntimeBoolToString(*Mod, Context), {ext}, "bool_str");
                fmtStr += "%s";
            } else {
                fmtStr += formatSpec(kriolType);
                if (kriolType.isInteger())
                    val = coerceToType(val, kriolType, kriolType.isSigned() ? Type::SignedInteger(64) : Type::UnsignedInteger(64));
                else if (kriolType.isFloat() && kriolType.bitWidth() < 64)
                    val = coerceToType(val, kriolType, Type::Float(64));
            }
            callArgs.push_back(val);
        }
    }

    auto* fmtGstr = Builder->CreateGlobalString(fmtStr, "fstr_fmt");
    callArgs.insert(callArgs.begin(), fmtGstr);
    auto* formatFn = getOrDeclareRuntimeFormat(*Mod, Context);
    LastValue = Builder->CreateCall(formatFn, callArgs, "fstr");
}

} // namespace ast
} // namespace kriol
