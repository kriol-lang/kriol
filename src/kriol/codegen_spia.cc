#include "../../include/kriol/codegen.hh"
#include "../../include/kriol/constants.hh"

#include <llvm/IR/Attributes.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>

#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kriol {
namespace ast {

namespace {

using InstructionSet = std::unordered_set<const llvm::Instruction*>;

bool callsFunction(const llvm::Instruction& inst, llvm::StringRef name)
{
    const auto* call = llvm::dyn_cast<llvm::CallBase>(&inst);
    return call && call->getCalledFunction() && call->getCalledFunction()->getName() == name;
}

// The instructions that run while one of the function's spia handlers is pushed,
// between a setjmp and any longjmp back to it. Returns false when the pushes and
// pops do not nest the same way on every path.
bool collectGuardedInstructions(const llvm::Function& fn, InstructionSet& guarded)
{
    std::unordered_map<const llvm::BasicBlock*, int> depthOnEntry{{&fn.getEntryBlock(), 0}};
    std::vector<const llvm::BasicBlock*> pending{&fn.getEntryBlock()};
    while (!pending.empty()) {
        const llvm::BasicBlock* block = pending.back();
        pending.pop_back();
        int depth = depthOnEntry.at(block);
        for (const llvm::Instruction& inst : *block) {
            if (depth > 0)
                guarded.insert(&inst);
            if (callsFunction(inst, "__kriol_spia_push"))
                ++depth;
            else if (callsFunction(inst, "__kriol_spia_pop") && --depth < 0)
                return false;
        }
        for (const llvm::BasicBlock* next : llvm::successors(block)) {
            const auto [known, isNew] = depthOnEntry.emplace(next, depth);
            if (isNew)
                pending.push_back(next);
            else if (known->second != depth)
                return false;
        }
    }
    return true;
}

enum class LocalUse { Read, Write, Escape };

// How `user` uses `address`, an address inside a local.
LocalUse classifyUse(const llvm::User& user, const llvm::Value& address)
{
    if (llvm::isa<llvm::LoadInst>(user))
        return LocalUse::Read;
    if (const auto* store = llvm::dyn_cast<llvm::StoreInst>(&user))
        return store->getPointerOperand() == &address ? LocalUse::Write : LocalUse::Escape;
    if (const auto* memory = llvm::dyn_cast<llvm::MemIntrinsic>(&user))
        return memory->getRawDest() == &address ? LocalUse::Write : LocalUse::Read;
    return LocalUse::Escape;
}

// C keeps across a longjmp the locals left unchanged since the setjmp (C17 7.13.2.1),
// so only a local written under a handler, or changed out of sight through an
// escaped address, has to stay in memory.
bool mustStayInMemory(const llvm::AllocaInst& local, const InstructionSet& guarded)
{
    std::vector<const llvm::Value*> addresses{&local};
    while (!addresses.empty()) {
        const llvm::Value* address = addresses.back();
        addresses.pop_back();
        for (const llvm::User* user : address->users()) {
            if (llvm::isa<llvm::GetElementPtrInst, llvm::BitCastInst, llvm::AddrSpaceCastInst>(user)) {
                addresses.push_back(user);
                continue;
            }
            const LocalUse use = classifyUse(*user, *address);
            if (use == LocalUse::Escape
                    || (use == LocalUse::Write && guarded.count(llvm::cast<llvm::Instruction>(user))))
                return true;
        }
    }
    return false;
}

std::unordered_set<const llvm::Value*> localsToKeepInMemory(llvm::Function& fn)
{
    InstructionSet guarded;
    const bool nested = collectGuardedInstructions(fn, guarded);
    std::unordered_set<const llvm::Value*> kept;
    for (const llvm::BasicBlock& block : fn)
        for (const llvm::Instruction& inst : block)
            if (const auto* local = llvm::dyn_cast<llvm::AllocaInst>(&inst);
                    local && (!nested || mustStayInMemory(*local, guarded)))
                kept.insert(local);
    return kept;
}

} // namespace

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
    auto* callLine = getter("__kriol_spia_call_line", i32Ty);
    emitSpiaPop();
    if (frame.RuntimeSlot)
        Builder->CreateStore(Builder->getTrue(), frame.RuntimeSlot);
    Builder->CreateStore(makeErru(message, kind, Builder->CreateSExt(line, Builder->getInt64Ty())), frame.ErrSlot);
    Builder->CreateStore(callLine, frame.LineSlot);
    Builder->CreateBr(frame.CatchBB);
}

void CodeGenVisitor::emitFailureToSpia(llvm::Value* erruValue, llvm::Value* callLine, llvm::Value* cameThroughRuntime)
{
    const SpiaFrame frame = SpiaFrames.back();
    if (!frame.InOops) {
        Builder->CreateStore(erruValue, frame.ErrSlot);
        Builder->CreateStore(callLine, frame.LineSlot);
        if (cameThroughRuntime && frame.RuntimeSlot)
            Builder->CreateStore(cameThroughRuntime, frame.RuntimeSlot);
        if (frame.RuntimeHandlerPushed)
            emitSpiaPop();
        Builder->CreateBr(frame.CatchBB);
        return;
    }

    SpiaFrames.pop_back();
    if (frame.RuntimeHandlerPushed)
        emitSpiaPop();
    emitSempriThenFail(frame, erruValue, callLine, cameThroughRuntime);
    SpiaFrames.push_back(frame);
}

void CodeGenVisitor::emitSempriThenFail(const SpiaFrame& frame, llvm::Value* erruValue, llvm::Value* callLine,
                                        llvm::Value* cameThroughRuntime)
{
    emitSempri(frame);
    if (!cameThroughRuntime) {
        emitFailure(erruValue, callLine);
        return;
    }
    if (!SpiaFrames.empty()) {
        emitFailureToSpia(erruValue, callLine, cameThroughRuntime);
        return;
    }
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* raiseBB = llvm::BasicBlock::Create(Context, "spia.raise", fn);
    auto* failBB = llvm::BasicBlock::Create(Context, "spia.fail", fn);
    Builder->CreateCondBr(cameThroughRuntime, raiseBB, failBB);
    Builder->SetInsertPoint(raiseBB);
    emitRaise(erruValue, callLine);
    Builder->SetInsertPoint(failBB);
    emitFailure(erruValue, callLine);
}

// Emitted where the spia is left, so the names declared inside it are hidden first.
void CodeGenVisitor::emitSempri(const SpiaFrame& frame)
{
    if (frame.Sempri) {
        std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> inner(
            std::make_move_iterator(Scopes.begin() + frame.ScopeDepth), std::make_move_iterator(Scopes.end()));
        Scopes.resize(frame.ScopeDepth);
        emitSempriCopy(*frame.Sempri);
        Scopes.insert(Scopes.end(), std::make_move_iterator(inner.begin()), std::make_move_iterator(inner.end()));
    }
    if (Builder->GetInsertBlock()->getTerminator())
        startDeadBlock();
}

void CodeGenVisitor::emitSempriCopy(ast::BlockSttmt& sempri)
{
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto enter = Mod->getOrInsertFunction("__kriol_sempri_enter", llvm::FunctionType::get(ptrTy, false));
    auto leave = Mod->getOrInsertFunction("__kriol_sempri_leave",
                                          llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {ptrTy}, false));
    auto* floor = Builder->CreateCall(enter, {}, "sempri.floor");

    std::vector<SpiaFrame> enclosing;
    enclosing.swap(SpiaFrames);
    ++SempriDepth;
    SempriCopies.emplace_back(&sempri, 0);
    sempri.accept(*this);
    SempriCopies.pop_back();
    --SempriDepth;
    SpiaFrames.swap(enclosing);

    if (!Builder->GetInsertBlock()->getTerminator())
        Builder->CreateCall(leave, {floor});
}

void CodeGenVisitor::emitSpiaExits(size_t keep)
{
    const std::vector<SpiaFrame> enclosing = SpiaFrames;
    while (SpiaFrames.size() > keep) {
        const SpiaFrame frame = SpiaFrames.back();
        SpiaFrames.pop_back();
        if (frame.RuntimeHandlerPushed)
            emitSpiaPop();
        emitSempri(frame);
    }
    SpiaFrames = enclosing;
}

void CodeGenVisitor::emitReturn(llvm::Value* value)
{
    emitSpiaExits(0);
    if (value)
        Builder->CreateRet(value);
    else
        Builder->CreateRetVoid();
}

void CodeGenVisitor::emitSpiaHandler(const SpiaFrame& catchTo)
{
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto push = Mod->getOrInsertFunction("__kriol_spia_push", llvm::FunctionType::get(ptrTy, {ptrTy}, false));
    auto* storage = createEntryAlloca(fn, "spia.frame",
                                      llvm::ArrayType::get(Builder->getInt8Ty(), KR_SPIA_FRAME_BYTES));
    storage->setAlignment(llvm::Align(KR_SPIA_FRAME_ALIGN));
    auto* frame = Builder->CreateCall(push, {storage}, "spia.frame");
    auto* jumped = Builder->CreateICmpNE(emitSetjmp(frame), Builder->getInt32(0), "spia.jumped");
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
    llvm::AllocaInst* runtimeSlot = nullptr;
    if (!node.Oops || node.Sempri) {
        runtimeSlot = createEntryAlloca(fn, "spia.runtime", Builder->getInt1Ty());
        Builder->CreateStore(Builder->getFalse(), runtimeSlot);
    }
    auto* catchBB = llvm::BasicBlock::Create(Context, node.Oops ? "spia.oops" : "spia.escape", fn);
    auto* endBB = llvm::BasicBlock::Create(Context, "spia.end", fn);
    const auto leaveGuarded = [&](const SpiaFrame& frame) {
        if (Builder->GetInsertBlock()->getTerminator())
            return;
        if (frame.RuntimeHandlerPushed)
            emitSpiaPop();
        Builder->CreateBr(endBB);
    };

    SpiaFrame body{catchBB, errSlot, lineSlot, node.Sempri.get(), runtimeSlot};
    body.RuntimeHandlerPushed = true;
    body.ScopeDepth = Scopes.size();
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
            emitSpiaHandler({oopsEscapeBB, errSlot, lineSlot, nullptr, runtimeSlot});
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
        emitSempriThenFail(body, Builder->CreateLoad(erruTy, errSlot, "spia.error"),
                           Builder->CreateLoad(Builder->getInt32Ty(), lineSlot, "spia.line"),
                           Builder->CreateLoad(Builder->getInt1Ty(), runtimeSlot, "spia.runtime"));

    Builder->SetInsertPoint(endBB);
    if (node.Sempri)
        emitSempriCopy(*node.Sempri);
    LastValue = nullptr;
}

void CodeGenVisitor::markLocalAccessesVolatile(llvm::Function& fn)
{
    const std::unordered_set<const llvm::Value*> kept = localsToKeepInMemory(fn);
    const auto onKeptLocal = [&](const llvm::Value* pointer) {
        return kept.count(llvm::getUnderlyingObject(pointer, 0)) != 0;
    };
    for (llvm::BasicBlock& block : fn)
        for (llvm::Instruction& inst : block) {
            if (auto* load = llvm::dyn_cast<llvm::LoadInst>(&inst); load && onKeptLocal(load->getPointerOperand()))
                load->setVolatile(true);
            if (auto* store = llvm::dyn_cast<llvm::StoreInst>(&inst); store && onKeptLocal(store->getPointerOperand()))
                store->setVolatile(true);
            auto* transfer = llvm::dyn_cast<llvm::MemTransferInst>(&inst);
            if (auto* memory = llvm::dyn_cast<llvm::MemIntrinsic>(&inst);
                    memory && (onKeptLocal(memory->getRawDest())
                               || (transfer && onKeptLocal(transfer->getRawSource()))))
                memory->setVolatile(Builder->getTrue());
        }
}

} // namespace ast
} // namespace kriol
