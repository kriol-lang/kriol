#include "include/kriol/constants.hh"
#include "include/kriol/cli.hh"

#ifdef _WIN32
#include <llvm/Support/InitLLVM.h>
#endif


int main(int argc, const char** argv) {
#ifdef _WIN32
    // Replaces argv with the UTF-8 command line; the one the CRT hands us is
    // in the ANSI code page and mangles non-ASCII paths.
    llvm::InitLLVM initLLVM(argc, argv);
#endif
    try {
        kriol::cli::Compiler Comp(KR_STANDARD_COMPILER_NAME, KR_VERSION_STRING);
        Comp.Run(argc, argv);
        kriol::cli::Compiler::Cleanup();
        return 0;
    } catch (const kriol::cli::FatalError& e) {
        kriol::cli::Compiler::Cleanup();
        return e.exitCode();
    } catch (...) {
        kriol::cli::Compiler::Cleanup();
        throw;
    }
}
