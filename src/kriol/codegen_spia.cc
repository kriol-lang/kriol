#include "../../include/kriol/codegen.hh"

#include <llvm/IR/Attributes.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>

#include <set>
#include <vector>

namespace kriol {
namespace ast {

// Pairs with the runtime's longjmp. Darwin, wasm32-wasi (with -wasm-enable-sjlj
// and libsetjmp.a, see emit): setjmp(buf). glibc: _setjmp(buf). MinGW:
// _setjmp(buf, NULL), so msvcrt's longjmp restores registers without SEH unwind.
llvm::Value* CodeGenVisitor::emitSetjmp(llvm::Value* frame)
{
    const llvm::Triple host(llvm::sys::getDefaultTargetTriple());
    const bool native = CurrentTarget == CodegenTarget::Native;
    const bool windows = CurrentTarget == CodegenTarget::X86_64Windows || (native && host.isOSWindows());
    const bool plainSetjmp = CurrentTarget == CodegenTarget::Wasm32Wasi || (native && host.isOSDarwin());
    const char* name = plainSetjmp ? "setjmp" : "_setjmp";

    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    std::vector<llvm::Type*> params = {ptrTy};
    std::vector<llvm::Value*> args = {frame};
    if (windows) {
        params.push_back(ptrTy);
        args.push_back(llvm::ConstantPointerNull::get(ptrTy));
    }

    auto* fn = Mod->getFunction(name);
    if (!fn) {
        fn = llvm::Function::Create(llvm::FunctionType::get(i32Ty, params, false),
                                    llvm::Function::ExternalLinkage, name, *Mod);
        fn->addFnAttr(llvm::Attribute::ReturnsTwice);
    }
    auto* call = Builder->CreateCall(fn, args, "setjmp");
    call->addFnAttr(llvm::Attribute::ReturnsTwice);
    return call;
}

void CodeGenVisitor::emitSpiaPop()
{
    auto* fn = Mod->getFunction("__kriol_spia_pop");
    if (!fn)
        fn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(Context), false),
                                    llvm::Function::ExternalLinkage, "__kriol_spia_pop", *Mod);
    Builder->CreateCall(fn);
}

// The frame's message is freed by the pop, so it is copied into Kriol text first.
void CodeGenVisitor::emitCaughtRuntimeError(const SpiaFrame& frame)
{
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    auto getter = [&](const char* name, llvm::Type* type) {
        return Builder->CreateCall(Mod->getOrInsertFunction(name, llvm::FunctionType::get(type, false)));
    };
    auto format = Mod->getOrInsertFunction("__kriol_format", llvm::FunctionType::get(ptrTy, {ptrTy}, true));
    auto* asText = Builder->CreateGlobalString("%s");
    auto* message = Builder->CreateCall(format, {asText, getter("__kriol_spia_message", ptrTy)}, "spia.message");
    auto* kind = Builder->CreateCall(format, {asText, getter("__kriol_spia_kind", ptrTy)}, "spia.kind");
    auto* line = getter("__kriol_spia_line", i32Ty);
    emitSpiaPop();
    Builder->CreateStore(makeErru(message, kind, Builder->CreateSExt(line, Builder->getInt64Ty())), frame.ErrSlot);
    Builder->CreateStore(line, frame.LineSlot);
    Builder->CreateBr(frame.CatchBB);
}

void CodeGenVisitor::emitFailureToSpia(llvm::Value* erruValue, llvm::Value* callLine)
{
    const SpiaFrame frame = SpiaFrames.back();
    if (!frame.InOops) {
        Builder->CreateStore(erruValue, frame.ErrSlot);
        Builder->CreateStore(callLine, frame.LineSlot);
        if (frame.RuntimeHandlerPushed)
            emitSpiaPop();
        Builder->CreateBr(frame.CatchBB);
        return;
    }

    SpiaFrames.pop_back();
    if (frame.RuntimeHandlerPushed)
        emitSpiaPop();
    emitSempriThenFail(frame.Sempri, erruValue, callLine);
    SpiaFrames.push_back(frame);
}

void CodeGenVisitor::emitSempriThenFail(ast::BlockSttmt* sempri, llvm::Value* erruValue, llvm::Value* callLine)
{
    if (sempri)
        sempri->accept(*this);
    if (Builder->GetInsertBlock()->getTerminator())
        startDeadBlock();
    emitFailure(erruValue, callLine);
}

void CodeGenVisitor::emitSpiaHandler(const SpiaFrame& catchTo)
{
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto push = Mod->getOrInsertFunction("__kriol_spia_push", llvm::FunctionType::get(
                                              llvm::PointerType::getUnqual(Context), false));
    auto* jumped = Builder->CreateICmpNE(emitSetjmp(Builder->CreateCall(push, {}, "spia.frame")),
                                         Builder->getInt32(0), "spia.jumped");
    auto* caughtBB = llvm::BasicBlock::Create(Context, "spia.caught", fn);
    auto* continueBB = llvm::BasicBlock::Create(Context, "spia.guarded", fn);
    Builder->CreateCondBr(jumped, caughtBB, continueBB);
    Builder->SetInsertPoint(caughtBB);
    emitCaughtRuntimeError(catchTo);
    Builder->SetInsertPoint(continueBB);
    CurrentHasSpia = true;
}

void CodeGenVisitor::visit(SpiaSttmt& node)
{
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* erruTy = getOrCreateRecordType(prelude::ErrorTypeName);
    auto* errSlot = createEntryAlloca(fn, node.Oops ? node.ErrName : "spia.error", erruTy);
    auto* lineSlot = createEntryAlloca(fn, "spia.line", llvm::Type::getInt32Ty(Context));
    Builder->CreateStore(Builder->getInt32(node.LineNum), lineSlot);
    auto* catchBB = llvm::BasicBlock::Create(Context, node.Oops ? "spia.oops" : "spia.escape", fn);
    auto* endBB = llvm::BasicBlock::Create(Context, "spia.end", fn);
    const auto leaveGuarded = [&](const SpiaFrame& frame) {
        if (Builder->GetInsertBlock()->getTerminator())
            return;
        if (frame.RuntimeHandlerPushed)
            emitSpiaPop();
        Builder->CreateBr(endBB);
    };

    SpiaFrame body{catchBB, errSlot, lineSlot, node.Sempri.get()};
    body.RuntimeHandlerPushed = true;
    emitSpiaHandler(body);
    SpiaFrames.push_back(body);
    node.Body->accept(*this);
    SpiaFrames.pop_back();
    leaveGuarded(body);

    Builder->SetInsertPoint(catchBB);
    if (node.Oops) {
        SpiaFrame oops = body;
        oops.InOops = true;
        oops.RuntimeHandlerPushed = node.Sempri != nullptr;
        // A runtime error in an oops with a sempri must run the sempri before it goes on up.
        llvm::BasicBlock* oopsEscapeBB = nullptr;
        if (oops.RuntimeHandlerPushed) {
            oopsEscapeBB = llvm::BasicBlock::Create(Context, "spia.oops.escape", fn);
            emitSpiaHandler({oopsEscapeBB, errSlot, lineSlot, nullptr});
        }
        SpiaFrames.push_back(oops);
        pushScope();
        declareVar(node.ErrName, errSlot);
        node.Oops->accept(*this);
        popScope();
        SpiaFrames.pop_back();
        leaveGuarded(oops);
        if (oopsEscapeBB)
            Builder->SetInsertPoint(oopsEscapeBB);
    }
    if (!node.Oops || node.Sempri)
        emitSempriThenFail(node.Sempri.get(), Builder->CreateLoad(erruTy, errSlot, "spia.error"),
                           Builder->CreateLoad(Builder->getInt32Ty(), lineSlot, "spia.line"));

    Builder->SetInsertPoint(endBB);
    if (node.Sempri)
        node.Sempri->accept(*this);
    LastValue = nullptr;
}

void CodeGenVisitor::markLocalAccessesVolatile(llvm::Function& fn)
{
    const auto onLocal = [](llvm::Value* pointer) {
        return llvm::isa<llvm::AllocaInst>(llvm::getUnderlyingObject(pointer, 0));
    };
    for (llvm::BasicBlock& block : fn)
        for (llvm::Instruction& inst : block) {
            if (auto* load = llvm::dyn_cast<llvm::LoadInst>(&inst); load && onLocal(load->getPointerOperand()))
                load->setVolatile(true);
            if (auto* store = llvm::dyn_cast<llvm::StoreInst>(&inst); store && onLocal(store->getPointerOperand()))
                store->setVolatile(true);
            if (auto* memory = llvm::dyn_cast<llvm::MemIntrinsic>(&inst))
                memory->setVolatile(Builder->getTrue());
        }
}

namespace {

void redirectCalls(llvm::Function& from, llvm::Function& to, bool dropKind)
{
    std::vector<llvm::CallInst*> calls;
    for (llvm::User* user : from.users())
        if (auto* call = llvm::dyn_cast<llvm::CallInst>(user))
            calls.push_back(call);
    for (llvm::CallInst* call : calls) {
        std::vector<llvm::Value*> args(call->arg_begin(), call->arg_end());
        if (dropKind)
            args.erase(args.begin());
        llvm::IRBuilder<> builder(call);
        builder.CreateCall(&to, args)->setDebugLoc(call->getDebugLoc());
        call->eraseFromParent();
    }
}

} // namespace

// Without a spia block the handler stack is always empty, so every runtime
// error terminates: route them to the non-jumping functions, as before spia
// existed, and drop the throw path (and with it longjmp) from the program.
void removeUnreachableSpiaThrow(llvm::Module& module)
{
    if (module.getFunction("setjmp") || module.getFunction("_setjmp"))
        return;
    auto* failAt = module.getFunction("__kriol_fail_at");
    auto* panicAt = module.getFunction("__kriol_panic_at");
    auto* raise = module.getFunction("__kriol_raise");
    auto* reportEscaped = module.getFunction("__kriol_report_escaped");
    if (!failAt || !panicAt || !raise || !reportEscaped)
        return;

    redirectCalls(*failAt, *panicAt, true);
    redirectCalls(*raise, *reportEscaped, false);

    std::vector<llvm::Function*> doomed;
    for (const char* name : {"__kriol_fail_at", "__kriol_raise", "__kriol_spia_throw"})
        if (auto* fn = module.getFunction(name))
            doomed.push_back(fn);

    std::set<llvm::GlobalVariable*> orphanCandidates;
    std::vector<llvm::Function*> mayBecomeDead = doomed;
    mayBecomeDead.push_back(reportEscaped);
    for (llvm::Function* fn : mayBecomeDead)
        for (llvm::BasicBlock& block : *fn)
            for (llvm::Instruction& inst : block)
                for (llvm::Value* operand : inst.operands())
                    if (auto* global = llvm::dyn_cast<llvm::GlobalVariable>(operand))
                        orphanCandidates.insert(global);

    for (llvm::Function* fn : doomed)
        fn->deleteBody();
    if (reportEscaped->use_empty())
        doomed.push_back(reportEscaped);
    for (llvm::Function* fn : doomed) {
        fn->deleteBody();
        if (fn->use_empty())
            fn->eraseFromParent();
    }
    for (llvm::GlobalVariable* global : orphanCandidates)
        if (global->hasPrivateLinkage() && global->use_empty())
            global->eraseFromParent();
}

} // namespace ast
} // namespace kriol
