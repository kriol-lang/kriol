#include "../../include/kriol/codegen.hh"

#include <llvm/IR/Instructions.h>

#include <set>
#include <vector>

namespace kriol {
namespace ast {

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
