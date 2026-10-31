#include "include/kriol/codegen.hh"

#include <llvm/IR/Verifier.h>

#include <cstdio>
#include <cstdlib>

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            std::exit(1); \
        } \
    } while (0)

namespace {

struct RuntimeStub {
    llvm::LLVMContext Context;
    llvm::Module Module{"stub", Context};
    llvm::Type* Void = llvm::Type::getVoidTy(Context);
    llvm::Type* I32 = llvm::Type::getInt32Ty(Context);
    llvm::Type* Ptr = llvm::PointerType::getUnqual(Context);

    llvm::Function* declare(const char* name, std::vector<llvm::Type*> params) {
        auto* fn = llvm::Function::Create(llvm::FunctionType::get(Void, params, false),
                                          llvm::Function::ExternalLinkage, name, Module);
        fn->addFnAttr(llvm::Attribute::NoReturn);
        return fn;
    }

    llvm::GlobalVariable* privateString(const char* name) {
        auto* text = llvm::ConstantDataArray::getString(Context, name);
        auto* global = new llvm::GlobalVariable(Module, text->getType(), true,
                                                llvm::GlobalValue::PrivateLinkage, text, name);
        return global;
    }

    llvm::Function* define(const char* name, std::vector<llvm::Type*> params,
                           llvm::function_ref<void(llvm::IRBuilder<>&, llvm::Function&)> body) {
        auto* fn = declare(name, params);
        llvm::IRBuilder<> builder(llvm::BasicBlock::Create(Context, "entry", fn));
        body(builder, *fn);
        builder.CreateUnreachable();
        return fn;
    }

    llvm::Function* build(bool withSetjmp, bool callsRaise = true) {
        auto* thrower = define("__kriol_spia_throw", {Ptr, Ptr, I32}, [](auto&, auto&) {});
        auto* panicAt = define("__kriol_panic_at", {Ptr, I32}, [](auto&, auto&) {});
        auto* reportEscaped = define("__kriol_report_escaped", {Ptr, Ptr, I32, I32}, [](auto&, auto&) {});
        auto* kindText = privateString("divizon_kind");
        privateString("unrelated_unused");

        auto* failAt = define("__kriol_fail_at", {Ptr, Ptr, I32}, [&](auto& builder, auto& fn) {
            builder.CreateCall(thrower, {kindText, fn.getArg(1), fn.getArg(2)});
        });
        auto* raise = define("__kriol_raise", {Ptr, Ptr, I32, I32}, [&](auto& builder, auto& fn) {
            builder.CreateCall(thrower, {fn.getArg(1), fn.getArg(0), fn.getArg(2)});
        });
        (void)panicAt;
        (void)reportEscaped;

        auto* caller = define("caller", {Ptr}, [&](auto& builder, auto& fn) {
            auto* line = llvm::ConstantInt::get(I32, 4);
            builder.CreateCall(failAt, {kindText, fn.getArg(0), line});
            if (callsRaise)
                builder.CreateCall(raise, {fn.getArg(0), fn.getArg(0), line, line});
        });
        if (withSetjmp)
            declare("setjmp", {Ptr});
        return caller;
    }
};

unsigned callsTo(llvm::Function& caller, const char* name) {
    unsigned count = 0;
    for (llvm::BasicBlock& block : caller)
        for (llvm::Instruction& inst : block)
            if (auto* call = llvm::dyn_cast<llvm::CallInst>(&inst))
                if (call->getCalledFunction() && call->getCalledFunction()->getName() == name)
                    ++count;
    return count;
}

void rewrites_calls_when_the_module_has_no_spia() {
    RuntimeStub stub;
    llvm::Function* caller = stub.build(false);

    kriol::ast::removeUnreachableSpiaThrow(stub.Module);

    CHECK(callsTo(*caller, "__kriol_fail_at") == 0);
    CHECK(callsTo(*caller, "__kriol_raise") == 0);
    CHECK(callsTo(*caller, "__kriol_panic_at") == 1);
    CHECK(callsTo(*caller, "__kriol_report_escaped") == 1);
    CHECK(!stub.Module.getFunction("__kriol_fail_at"));
    CHECK(!stub.Module.getFunction("__kriol_raise"));
    CHECK(!stub.Module.getFunction("__kriol_spia_throw"));
    CHECK(stub.Module.getFunction("__kriol_report_escaped"));
    CHECK(!stub.Module.getNamedGlobal("divizon_kind"));
    CHECK(stub.Module.getNamedGlobal("unrelated_unused"));
    CHECK(!llvm::verifyModule(stub.Module, &llvm::errs()));
}

void drops_the_reporter_when_nothing_raises() {
    RuntimeStub stub;
    llvm::Function* caller = stub.build(false, false);

    kriol::ast::removeUnreachableSpiaThrow(stub.Module);

    CHECK(callsTo(*caller, "__kriol_panic_at") == 1);
    CHECK(!stub.Module.getFunction("__kriol_raise"));
    CHECK(!stub.Module.getFunction("__kriol_report_escaped"));
    CHECK(!llvm::verifyModule(stub.Module, &llvm::errs()));
}

void leaves_the_module_alone_when_it_has_a_spia() {
    RuntimeStub stub;
    llvm::Function* caller = stub.build(true);

    kriol::ast::removeUnreachableSpiaThrow(stub.Module);

    CHECK(callsTo(*caller, "__kriol_fail_at") == 1);
    CHECK(callsTo(*caller, "__kriol_raise") == 1);
    CHECK(stub.Module.getFunction("__kriol_spia_throw"));
    CHECK(stub.Module.getNamedGlobal("divizon_kind"));
    CHECK(!llvm::verifyModule(stub.Module, &llvm::errs()));
}

} // namespace

int main() {
    rewrites_calls_when_the_module_has_no_spia();
    drops_the_reporter_when_nothing_raises();
    leaves_the_module_alone_when_it_has_a_spia();
    return 0;
}
