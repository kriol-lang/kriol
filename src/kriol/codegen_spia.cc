#include "../../include/kriol/codegen.hh"

#include <llvm/IR/Attributes.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

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

} // namespace ast
} // namespace kriol
