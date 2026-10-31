#include "../../include/kriol/codegen.hh"

#include <llvm/IR/Attributes.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <llvm/IR/Instructions.h>

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

void CodeGenVisitor::visit(SpiaSttmt& node)
{
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* erruTy = getOrCreateRecordType(prelude::ErrorTypeName);
    auto* errSlot = createEntryAlloca(fn, node.Oops ? node.ErrName : "spia.error", erruTy);
    auto* lineSlot = createEntryAlloca(fn, "spia.line", llvm::Type::getInt32Ty(Context));
    Builder->CreateStore(Builder->getInt32(node.LineNum), lineSlot);
    auto* catchBB = llvm::BasicBlock::Create(Context, node.Oops ? "spia.oops" : "spia.escape", fn);
    auto* endBB = llvm::BasicBlock::Create(Context, "spia.end", fn);
    const auto branchToEnd = [&] {
        if (!Builder->GetInsertBlock()->getTerminator())
            Builder->CreateBr(endBB);
    };

    SpiaFrames.push_back({catchBB, errSlot, lineSlot, node.Sempri.get()});
    node.Body->accept(*this);
    SpiaFrames.pop_back();
    branchToEnd();

    Builder->SetInsertPoint(catchBB);
    if (node.Oops) {
        SpiaFrame oops{catchBB, errSlot, lineSlot, node.Sempri.get()};
        oops.InOops = true;
        SpiaFrames.push_back(oops);
        pushScope();
        declareVar(node.ErrName, errSlot);
        node.Oops->accept(*this);
        popScope();
        SpiaFrames.pop_back();
        branchToEnd();
    } else {
        emitSempriThenFail(node.Sempri.get(), Builder->CreateLoad(erruTy, errSlot, "spia.error"),
                           Builder->CreateLoad(Builder->getInt32Ty(), lineSlot, "spia.line"));
    }

    Builder->SetInsertPoint(endBB);
    if (node.Sempri)
        node.Sempri->accept(*this);
    LastValue = nullptr;
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
