#include <llvm/IR/Verifier.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/MemoryBuffer.h>

#include <llvm/Support/Program.h>
#include <llvm/Support/Path.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/OptimizationLevel.h>

#if KRIOL_USE_EMBEDDED_LLD

#include <lld/Common/Driver.h>
LLD_HAS_DRIVER(wasm)

#endif // KRIOL_USE_EMBEDDED_LLD

#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <algorithm>

#include "../../include/kriol/codegen.hh"
#include "../../include/kriol/type_rules.hh"

using namespace kriol::ast;
using namespace kriol::typerules;

// NOTE: these includes below are generated and
// injected in compile time by the build system.
#include "kriol_runtime_native_gc.bc.h"
#include "libgc_native.h"

#if KRIOL_ENABLE_WASM

#if KRIOL_WASI_ENABLE_GC
#include "kriol_runtime_wasm32_wasi_gc.bc.h"
#include "libgc_wasm32_wasi.h"
#else // !KRIOL_WASI_ENABLE_GC
#include "kriol_runtime_wasm32_wasi_nogc.bc.h"
#endif // KRIOL_WASI_ENABLE_GC

#include "wasi_crt1_command.o.h"
#include "wasi_libc.a.h"
#include "wasi_libm.a.h"
#include "wasi_builtins.a.h"

#endif // KRIOL_ENABLE_WASM

#if KRIOL_ENABLE_WINDOWS_TARGET

struct KriolEmbeddedFile {
    const char* Name;
    const unsigned char* Data;
    unsigned int Len;
};

#include "kriol_runtime_x86_64_windows.bc.h"
#include "libgc_x86_64_windows.o.h"
#include "mingw_link_inputs.h"

#endif // KRIOL_ENABLE_WINDOWS_TARGET

#define __WASI_MAIN "__main_argc_argv"
#define KRIOL_MINGW_TRIPLE "x86_64-w64-windows-gnu"


// Interpret backslash escapes in a raw string (without surrounding quotes).
std::string kriol::ast::CodeGenVisitor::processEscapes(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            switch (raw[++i]) {
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                case 'r':  out += '\r'; break;
                case '\\': out += '\\'; break;
                case '"':  out += '"';  break;
                case '\'': out += '\''; break;
                case '0':  out += '\0'; break;
                case '{':  out += '{';  break;
                case '}':  out += '}';  break;
                default:   out += '\\'; out += raw[i]; break;
            }
        } else {
            out += raw[i];
        }
    }
    return out;
}

// Returns the printf format specifier for a Kriol type.
const char* kriol::ast::CodeGenVisitor::formatSpec(const kriol::Type& kriolType) {
    if (kriolType.isUnsignedInteger()) return "%llu";
    if (kriolType.isInteger()) return "%lld";
    if (kriolType.isFloat())  return "%g";
    return "%s"; // textu / fallback
}

// Reverse-map an LLVM type to the corresponding Kriol type string.
kriol::Type kriol::ast::CodeGenVisitor::llvmTypeToKriol(llvm::Type* ty) {
    if (ty->isIntegerTy(1))  return kriol::Type::Bool();
    if (ty->isIntegerTy())   return kriol::Type::SignedInteger(ty->getIntegerBitWidth());
    if (ty->isFloatTy())     return kriol::Type::Float(32);
    if (ty->isDoubleTy())    return kriol::Type::Float(64);
    return kriol::Type::Text(); // pointer / fallback
}

namespace {

using kriol::ast::CodegenTarget;

struct EmbeddedBlob {
    const unsigned char* Data;
    unsigned int Len;
};

struct TargetResources {
    std::string Triple;
    EmbeddedBlob Runtime;
    EmbeddedBlob GcArchive;
};

struct WasiLinkPlan {
    std::string LinkerPath;
    std::string OutputPath;
    std::vector<std::string> Args;
};

struct WasiInputs {
    std::string Crt1Command;
    std::string Libc;
    std::string Libm;
    std::string Builtins;
};

static void initializeTargets() {
    static bool initialized = false;
    if (initialized) return;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

#if KRIOL_ENABLE_WASM
    LLVMInitializeWebAssemblyTargetInfo();
    LLVMInitializeWebAssemblyTarget();
    LLVMInitializeWebAssemblyTargetMC();
    LLVMInitializeWebAssemblyAsmPrinter();
#endif

    initialized = true;
}

static std::string findProgram(std::initializer_list<const char*> names,
                               const std::string& description) {
    for (const char* name : names) {
        auto path = llvm::sys::findProgramByName(name);
        if (path) return *path;
    }
    throw std::runtime_error("não foi encontrado o " + description + ", necessário para gerar o executável; instala-o ou acrescenta-o ao PATH");
}

#if KRIOL_USE_EMBEDDED_LLD
static std::string findProgramOptional(std::initializer_list<const char*> names) {
    for (const char* name : names) {
        auto path = llvm::sys::findProgramByName(name);
        if (path) return *path;
    }
    return {};
}
#endif

static std::string writeTempBlob(const char* stem,
                                 const char* suffix,
                                 EmbeddedBlob blob) {
    llvm::SmallString<128> tempPath;
    std::error_code ec = llvm::sys::fs::createTemporaryFile(stem, suffix, tempPath);
    if (ec) {
        throw std::runtime_error("não foi possível criar um ficheiro temporário: " + ec.message());
    }

    llvm::raw_fd_ostream out(tempPath, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("não foi possível abrir um ficheiro temporário: " + ec.message());
    }

    out.write(reinterpret_cast<const char*>(blob.Data), blob.Len);
    out.close();
    return std::string(tempPath.str());
}

static void runProgram(const std::string& program,
                       const std::vector<std::string>& ownedArgs,
                       const std::string& failurePrefix) {
    std::vector<llvm::StringRef> args;
    args.reserve(ownedArgs.size());
    for (const auto& arg : ownedArgs)
        args.push_back(arg);

    std::string err;
    bool execFailed = false;
    int ret = llvm::sys::ExecuteAndWait(
        program, args, std::nullopt, {}, 0, 0, &err, &execFailed
    );

    if (execFailed || ret != 0)
        throw std::runtime_error(failurePrefix + err);
}

static WasiLinkPlan buildWasiLinkPlan(const std::string& objPath,
                                      const std::string& tempLibGc,
                                      const WasiInputs& wasiInputs,
                                      const std::string& outputPath) {
#if KRIOL_USE_EMBEDDED_LLD
    std::string wasmLdPath = findProgramOptional({"wasm-ld-20", "wasm-ld-19", "wasm-ld"});
    std::string linkerArg0 = wasmLdPath.empty() ? "wasm-ld" : wasmLdPath;
#else
    std::string wasmLdPath = findProgram(
        {"wasm-ld-20", "wasm-ld-19", "wasm-ld"},
        "linker de WASI 'wasm-ld-20', 'wasm-ld-19' ou 'wasm-ld'"
    );
    std::string linkerArg0 = wasmLdPath;
#endif
    WasiLinkPlan plan;
    plan.LinkerPath = wasmLdPath;
    plan.OutputPath = outputPath;
    plan.Args = {
        linkerArg0,
        "-m",
        "wasm32",
        // Stack first in memory, so an overflow traps instead of overwriting data.
        "-z",
        "stack-size=8388608",
        "--stack-first",
        "--export=" __WASI_MAIN,
        wasiInputs.Crt1Command,
        objPath,
    };
    if (!tempLibGc.empty())
        plan.Args.push_back(tempLibGc);
    plan.Args.insert(plan.Args.end(), {
        wasiInputs.Libm,
        wasiInputs.Libc,
        wasiInputs.Builtins,
        "-o",
        outputPath
    });

    return plan;
}

static void linkWasmWithWasmLd(const WasiLinkPlan& plan) {
    if (plan.LinkerPath.empty())
        throw std::runtime_error("o linker de WASI não está disponível");
    runProgram(plan.LinkerPath, plan.Args, "a ligação do módulo WASI falhou: ");
}

#if KRIOL_USE_EMBEDDED_LLD
static void linkWasmWithEmbeddedLld(const WasiLinkPlan& plan) {
    std::vector<const char*> args;
    args.reserve(plan.Args.size());
    for (const auto& arg : plan.Args)
        args.push_back(arg.c_str());

    std::string stdoutText;
    std::string stderrText;
    llvm::raw_string_ostream stdoutStream(stdoutText);
    llvm::raw_string_ostream stderrStream(stderrText);

    lld::DriverDef drivers[] = {
        {lld::Wasm, &lld::wasm::link}
    };
    lld::Result result = lld::lldMain(args, stdoutStream, stderrStream, drivers);
    stdoutStream.flush();
    stderrStream.flush();

    if (result.retCode != 0 || !result.canRunAgain) {
        std::string detail = stderrText.empty() ? stdoutText : stderrText;
        if (!result.canRunAgain && detail.empty())
            detail = "LLD reported that it cannot be called again.";
        throw std::runtime_error("a ligação do módulo WASI falhou: " + detail);
    }
}
#endif

static void linkWasm(const WasiLinkPlan& plan) {
#if KRIOL_USE_EMBEDDED_LLD
    try {
        linkWasmWithEmbeddedLld(plan);
        return;
    } catch (const std::exception&) {
        llvm::sys::fs::remove(plan.OutputPath);
    }
#endif
    linkWasmWithWasmLd(plan);
}

#if KRIOL_ENABLE_WASM
static WasiInputs writeWasiInputs() {
    return WasiInputs{
        writeTempBlob(
            "kriol_wasi_crt1_command",
            "o",
            EmbeddedBlob{wasi_crt1_command_o, wasi_crt1_command_o_len}
        ),
        writeTempBlob(
            "kriol_wasi_libc",
            "a",
            EmbeddedBlob{wasi_libc_a, wasi_libc_a_len}
        ),
        writeTempBlob(
            "kriol_wasi_libm",
            "a",
            EmbeddedBlob{wasi_libm_a, wasi_libm_a_len}
        ),
        writeTempBlob(
            "kriol_wasi_builtins",
            "a",
            EmbeddedBlob{wasi_builtins_a, wasi_builtins_a_len}
        )
    };
}
#endif

static TargetResources selectTargetResources(CodegenTarget target) {
    switch (target) {
        case CodegenTarget::Native:
            return TargetResources{
                llvm::sys::getDefaultTargetTriple(),
                EmbeddedBlob{kriol_runtime_native_gc_bc, kriol_runtime_native_gc_bc_len},
                EmbeddedBlob{libgc_native_a, libgc_native_a_len}
            };
        case CodegenTarget::Wasm32Wasi:
#if KRIOL_ENABLE_WASM
#if KRIOL_WASI_ENABLE_GC
            return TargetResources{
                KRIOL_WASI_TARGET,
                EmbeddedBlob{kriol_runtime_wasm32_wasi_gc_bc, kriol_runtime_wasm32_wasi_gc_bc_len},
                EmbeddedBlob{libgc_wasm32_wasi_a, libgc_wasm32_wasi_a_len}
            };
#else
            return TargetResources{
                KRIOL_WASI_TARGET,
                EmbeddedBlob{kriol_runtime_wasm32_wasi_nogc_bc, kriol_runtime_wasm32_wasi_nogc_bc_len},
                EmbeddedBlob{nullptr, 0}
            };
#endif
#else
            throw std::runtime_error("esta versão do compilador foi construída sem suporte para 'wasm32-wasi'");
#endif
        case CodegenTarget::X86_64Windows:
#if KRIOL_ENABLE_WINDOWS_TARGET
            return TargetResources{
                KRIOL_MINGW_TRIPLE,
                EmbeddedBlob{kriol_runtime_x86_64_windows_bc, kriol_runtime_x86_64_windows_bc_len},
                EmbeddedBlob{libgc_x86_64_windows_o, libgc_x86_64_windows_o_len}
            };
#else
            throw std::runtime_error("esta versão do compilador foi construída sem suporte para 'x86_64-windows'");
#endif
    }

    throw std::runtime_error("erro interno: alvo de compilação desconhecido");
}

static void linkRuntimeBitcode(llvm::Module& module,
                               llvm::LLVMContext& context,
                               EmbeddedBlob runtimeBlob) {
    llvm::StringRef bcData(reinterpret_cast<const char*>(runtimeBlob.Data), runtimeBlob.Len);
    auto memBuf = llvm::MemoryBuffer::getMemBuffer(bcData, "kriol_runtime", false);

    auto expectedMod = llvm::parseBitcodeFile(*memBuf, context);
    if (!expectedMod) {
        std::string errDetail = llvm::toString(expectedMod.takeError());
        throw std::runtime_error("erro interno: não foi possível ler o runtime: " + errDetail);
    }

    bool linkErr = llvm::Linker::linkModules(module, std::move(expectedMod.get()));
    if (linkErr)
        throw std::runtime_error("erro interno: não foi possível juntar o runtime ao programa");

    // clang tags the runtime with its baseline CPU attributes, which kriol's
    // functions lack, and LLVM never inlines across differing target
    // attributes. Dropping them lets hot helpers (bounds and division checks)
    // inline and fold; the target machine uses the same baseline CPU anyway.
    for (llvm::Function& fn : module) {
        fn.removeFnAttr("target-cpu");
        fn.removeFnAttr("target-features");
        fn.removeFnAttr("tune-cpu");
    }
}

static void optimizeModule(llvm::Module& module,
                           llvm::TargetMachine& targetMachine,
                           unsigned optLevel) {
    llvm::LoopAnalysisManager loopAM;
    llvm::FunctionAnalysisManager functionAM;
    llvm::CGSCCAnalysisManager cgsccAM;
    llvm::ModuleAnalysisManager moduleAM;

    llvm::PassBuilder passBuilder(&targetMachine);
    passBuilder.registerModuleAnalyses(moduleAM);
    passBuilder.registerCGSCCAnalyses(cgsccAM);
    passBuilder.registerFunctionAnalyses(functionAM);
    passBuilder.registerLoopAnalyses(loopAM);
    passBuilder.crossRegisterProxies(loopAM, functionAM, cgsccAM, moduleAM);

    const llvm::OptimizationLevel level = optLevel >= 3 ? llvm::OptimizationLevel::O3
                                        : optLevel == 2 ? llvm::OptimizationLevel::O2
                                        : llvm::OptimizationLevel::O1;
    passBuilder.buildPerModuleDefaultPipeline(level).run(module, moduleAM);
}

static void emitObjectFile(llvm::Module& module,
                           const std::string& targetTriple,
                           const std::string& objPath,
                           unsigned optLevel) {
    module.setTargetTriple(targetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);
    if (!Target)
        throw std::runtime_error("o LLVM não suporta o alvo de compilação: " + Error);

    auto CPU = "generic";
    auto Features = "";
    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    const llvm::CodeGenOptLevel codeGenLevel = optLevel == 0 ? llvm::CodeGenOptLevel::None
                                             : optLevel == 1 ? llvm::CodeGenOptLevel::Less
                                             : optLevel == 2 ? llvm::CodeGenOptLevel::Default
                                             : llvm::CodeGenOptLevel::Aggressive;
    std::unique_ptr<llvm::TargetMachine> TargetMachine(Target->createTargetMachine(
        targetTriple, CPU, Features, opt, RM, std::nullopt, codeGenLevel));

    module.setDataLayout(TargetMachine->createDataLayout());

    if (optLevel > 0)
        optimizeModule(module, *TargetMachine, optLevel);

    std::error_code EC;
    llvm::raw_fd_ostream dest(objPath, EC, llvm::sys::fs::OF_None);
    if (EC)
        throw std::runtime_error("não foi possível criar o ficheiro objeto: " + EC.message());

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::ObjectFile;
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
        throw std::runtime_error("erro interno: o LLVM não consegue gerar código objeto para este alvo");

    pass.run(module);
    dest.flush();
}

static bool isWindowsTriple(const std::string& triple) {
    return llvm::Triple(triple).isOSWindows();
}

static std::string writeGcArchive(CodegenTarget target,
                                  const std::string& targetTriple,
                                  EmbeddedBlob gcBlob) {
    if (!gcBlob.Data || gcBlob.Len == 0)
        return {};

    if (target == CodegenTarget::X86_64Windows)
        return writeTempBlob("embedded_libgc_x86_64_windows", "o", gcBlob);

    const char* stem = target == CodegenTarget::Wasm32Wasi
        ? "embedded_libgc_wasm32_wasi"
        : "embedded_libgc_native";
    return writeTempBlob(stem, isWindowsTriple(targetTriple) ? "lib" : "a", gcBlob);
}

static void linkNativeExecutable(const std::string& objPath,
                                 const std::string& tempLibGc,
                                 const std::string& outputPath,
                                 const std::string& targetTriple) {
    const bool windows = isWindowsTriple(targetTriple);
    std::string ccPath = findProgram({"clang", "cc"}, "linker 'clang' ou 'cc'");
    std::vector<std::string> linkArgs = {ccPath};
    if (!windows)
        linkArgs.push_back("-no-pie");
    linkArgs.push_back(objPath);
    if (!tempLibGc.empty())
        linkArgs.push_back(tempLibGc);
    linkArgs.push_back("-o");
    linkArgs.push_back(outputPath);
    if (!windows)
        linkArgs.push_back("-lm");
    runProgram(ccPath, linkArgs, "a ligação do executável falhou: ");
}

#if KRIOL_ENABLE_WINDOWS_TARGET
struct MingwInputs {
    std::vector<std::string> Startup;
    std::vector<std::string> Libraries;
};

static void removeMingwInputs(const MingwInputs& inputs) {
    for (const auto& path : inputs.Startup) llvm::sys::fs::remove(path);
    for (const auto& path : inputs.Libraries) llvm::sys::fs::remove(path);
}

template <size_t N>
static void writeMingwGroup(const KriolEmbeddedFile (&files)[N],
                            std::vector<std::string>& paths) {
    for (const auto& file : files) {
        std::string stem = "kriol_mingw_" + llvm::sys::path::stem(file.Name).str();
        std::string suffix = llvm::sys::path::extension(file.Name).drop_front().str();
        paths.push_back(writeTempBlob(stem.c_str(), suffix.c_str(), EmbeddedBlob{file.Data, file.Len}));
    }
}

static MingwInputs writeMingwInputs() {
    MingwInputs inputs;
    try {
        writeMingwGroup(kriol_mingw_startup_inputs, inputs.Startup);
        writeMingwGroup(kriol_mingw_library_inputs, inputs.Libraries);
    } catch (...) {
        removeMingwInputs(inputs);
        throw;
    }
    return inputs;
}

static std::string findMingwLinker() {
    // Release archives ship ld.lld next to kriol, so that copy wins over PATH.
    std::string self = llvm::sys::fs::getMainExecutable(
        nullptr, reinterpret_cast<void*>(&findMingwLinker));
    if (!self.empty()) {
        llvm::SmallString<256> bundled(llvm::sys::path::parent_path(self));
#ifdef _WIN32
        llvm::sys::path::append(bundled, "ld.lld.exe");
#else
        llvm::sys::path::append(bundled, "ld.lld");
#endif
        if (llvm::sys::fs::can_execute(bundled))
            return std::string(bundled.str());
    }

    return findProgram(
        {"ld.lld-20", "ld.lld-19", "ld.lld"},
        "linker 'ld.lld' (que deve estar ao lado do kriol)"
    );
}

static void linkMingwExecutable(const std::string& objPath,
                                const std::string& gcObject,
                                const std::string& outputPath) {
    std::string linkerPath = findMingwLinker();
    MingwInputs inputs = writeMingwInputs();

    // What the MinGW clang driver passes to ld.lld, plus Linux's 8 MiB stack.
    std::vector<std::string> linkArgs = {
        linkerPath, "-m", "i386pep", "--subsystem", "console",
        "--stack", "8388608", "-o", outputPath
    };
    linkArgs.insert(linkArgs.end(), inputs.Startup.begin(), inputs.Startup.end());
    linkArgs.push_back(objPath);
    if (!gcObject.empty())
        linkArgs.push_back(gcObject);
    linkArgs.insert(linkArgs.end(), inputs.Libraries.begin(), inputs.Libraries.end());

    try {
        runProgram(linkerPath, linkArgs, "a ligação do executável para Windows falhou: ");
    } catch (...) {
        removeMingwInputs(inputs);
        throw;
    }
    removeMingwInputs(inputs);
}
#endif // KRIOL_ENABLE_WINDOWS_TARGET

// Returns true if the block can be reached from its function's entry block.
static bool isReachableFromEntry(llvm::BasicBlock* target) {
    llvm::Function* fn = target->getParent();
    llvm::SmallPtrSet<llvm::BasicBlock*, 32> visited;
    llvm::SmallVector<llvm::BasicBlock*, 32> worklist;
    worklist.push_back(&fn->getEntryBlock());
    while (!worklist.empty()) {
        llvm::BasicBlock* bb = worklist.pop_back_val();
        if (!visited.insert(bb).second) continue;
        if (bb == target) return true;
        for (llvm::BasicBlock* succ : llvm::successors(bb))
            worklist.push_back(succ);
    }
    return false;
}

// User functions get a "kriol." prefix and globals a "kriol.g." prefix, both with
// internal linkage, so no Kriol name (exit, free, main, ...) can clash with a C
// library or runtime symbol. Only inisiu is exported, as the C entry point.
static std::string llvmFunctionName(const std::string& kriolName) {
    return kriolName == "inisiu" ? "main" : "kriol." + kriolName;
}

static std::string llvmGlobalName(const std::string& kriolName) {
    return "kriol.g." + kriolName;
}

static llvm::GlobalValue::LinkageTypes functionLinkage(const std::string& kriolName) {
    return kriolName == "inisiu"
        ? llvm::GlobalValue::ExternalLinkage
        : llvm::GlobalValue::InternalLinkage;
}

}

CodeGenVisitor::CodeGenVisitor(const std::string& moduleName)
    : Mod(std::make_unique<llvm::Module>(moduleName, Context)),
      Builder(std::make_unique<llvm::IRBuilder<>>(Context))
{
    initializeTargets();

    // The runtime names the source file in its error messages; its own
    // definition is weak, so this one wins when the runtime is linked in.
    auto* name = Builder->CreateGlobalString(moduleName, "kriol.source_name", 0, Mod.get());
    new llvm::GlobalVariable(*Mod, llvm::PointerType::getUnqual(Context), true,
                             llvm::GlobalValue::ExternalLinkage, name, "__kriol_source_name");
}

llvm::Type* CodeGenVisitor::mapType(const Type& t) {
    if (t.isArray())
        return llvm::ArrayType::get(mapType(t.elementType()), t.arraySize());

    if (t.isFloat()) {
        if (t.bitWidth() == 32) return llvm::Type::getFloatTy(Context);
        return llvm::Type::getDoubleTy(Context);
    }
    if (t.isInteger()) return llvm::IntegerType::get(Context, t.bitWidth());
    if (t == Type::Bool())    return llvm::Type::getInt1Ty(Context);
    if (t == Type::Text())    return llvm::PointerType::getUnqual(Context);
    if (t == Type::Void())    return llvm::Type::getVoidTy(Context);
    if (t.isNamed())          return getOrCreateRecordType(t.name());

    return llvm::PointerType::getUnqual(Context);
}

llvm::StructType* CodeGenVisitor::getOrCreateRecordType(const std::string& name) {
    auto it = Records.find(name);
    if (it == Records.end())
        throw std::runtime_error("erro interno: molde desconhecido '" + name + "'");

    RecordInfo& info = it->second;
    if (info.llvmType && !info.llvmType->isOpaque())
        return info.llvmType;

    if (!info.llvmType)
        info.llvmType = llvm::StructType::create(Context, "molda." + name);

    std::vector<llvm::Type*> fieldTypes;
    fieldTypes.reserve(info.fields.size());
    for (auto* field : info.fields)
        fieldTypes.push_back(mapType(field->Type));

    if (info.llvmType->isOpaque())
        info.llvmType->setBody(fieldTypes, false);

    return info.llvmType;
}

llvm::AllocaInst* CodeGenVisitor::createEntryAlloca(
        llvm::Function* fn, const std::string& name, llvm::Type* ty) {
    llvm::IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp.CreateAlloca(ty, nullptr, name);
}

llvm::AllocaInst* CodeGenVisitor::lookupVar(const std::string& name) {
    for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return nullptr;
}

llvm::GlobalVariable* CodeGenVisitor::lookupGlobal(const std::string& name) {
    auto it = GlobalVars.find(name);
    return it != GlobalVars.end() ? it->second : nullptr;
}

llvm::Value* CodeGenVisitor::coerce(llvm::Value* v, llvm::Type* targetTy) {
    llvm::Type* srcTy = v->getType();
    if (srcTy == targetTy) return v; // identity

    auto* doubleTy = llvm::Type::getDoubleTy(Context);
    auto* i64Ty    = llvm::Type::getInt64Ty(Context);
    auto* i1Ty     = llvm::Type::getInt1Ty(Context);

    // int (i64) -> num (double)
    if (srcTy == i64Ty && targetTy == doubleTy)
        return Builder->CreateSIToFP(v, doubleTy, "conv");
    // bool (i1) -> int (i64)
    if (srcTy == i1Ty && targetTy == i64Ty)
        return Builder->CreateZExt(v, i64Ty, "conv");
    // bool (i1) -> num (double)
    if (srcTy == i1Ty && targetTy == doubleTy)
        return Builder->CreateUIToFP(v, doubleTy, "conv");
    // num (double) -> int (i64)
    if (srcTy == doubleTy && targetTy == i64Ty)
        return Builder->CreateIntrinsic(llvm::Intrinsic::fptosi_sat, {i64Ty, doubleTy}, {v}, nullptr, "conv");
    // any integer widening (e.g. i1 -> i64 via general int path)
    if (srcTy->isIntegerTy() && targetTy->isIntegerTy())
        return Builder->CreateSExtOrTrunc(v, targetTy, "conv");

    throw std::runtime_error("erro interno: conversão implícita não suportada");
}

llvm::Value* CodeGenVisitor::coerceToType(llvm::Value* v,
                                          const Type& sourceType,
                                          const Type& targetType) {
    llvm::Type* targetTy = mapType(targetType);
    llvm::Type* sourceTy = v->getType();
    if (sourceTy == targetTy) return v;

    if (sourceType == Type::Bool() && targetType.isInteger())
        return Builder->CreateZExtOrTrunc(v, targetTy, "conv");
    if (sourceType == Type::Bool() && targetType.isFloat())
        return Builder->CreateUIToFP(v, targetTy, "conv");

    if (sourceType.isInteger() && targetType.isInteger()) {
        if (sourceType.isSigned())
            return Builder->CreateSExtOrTrunc(v, targetTy, "conv");
        return Builder->CreateZExtOrTrunc(v, targetTy, "conv");
    }

    if (sourceType.isInteger() && targetType.isFloat()) {
        if (sourceType.isSigned())
            return Builder->CreateSIToFP(v, targetTy, "conv");
        return Builder->CreateUIToFP(v, targetTy, "conv");
    }

    if (sourceType.isFloat() && targetType.isFloat()) {
        if (sourceType.bitWidth() < targetType.bitWidth())
            return Builder->CreateFPExt(v, targetTy, "conv");
        return Builder->CreateFPTrunc(v, targetTy, "conv");
    }

    if (sourceType.isFloat() && targetType.isInteger()) {
        // Truncates toward zero like C, but saturating: NaN becomes 0 and
        // out-of-range values clamp, so the result is always defined.
        const auto intrinsic = targetType.isSigned() ? llvm::Intrinsic::fptosi_sat : llvm::Intrinsic::fptoui_sat;
        return Builder->CreateIntrinsic(intrinsic, {targetTy, sourceTy}, {v}, nullptr, "conv");
    }

    return coerce(v, targetTy);
}

llvm::Value* CodeGenVisitor::toBool(llvm::Value* v) {
    if (v->getType()->isPointerTy())
        throw std::runtime_error("erro interno: condição sem valor lógico");
    if (v->getType()->isIntegerTy(1)) return v;
    if (v->getType()->isFloatingPointTy())
        return Builder->CreateFCmpONE(
            v, llvm::ConstantFP::get(v->getType(), 0.0), "booltmp");
    return Builder->CreateICmpNE(
        v, llvm::ConstantInt::get(v->getType(), 0), "booltmp");
}

std::string CodeGenVisitor::emitIR() {
    std::string buf;
    llvm::raw_string_ostream os(buf);
    Mod->print(os, nullptr);
    return buf;
}

std::vector<unsigned char> CodeGenVisitor::emitToMemory(const EmitOptions& options) {
    llvm::SmallString<128> outputPath;
    std::error_code ec = llvm::sys::fs::createTemporaryFile(
        options.Target == CodegenTarget::Wasm32Wasi ? "kriol_wasm_output" : "kriol_output",
        options.Target == CodegenTarget::Wasm32Wasi ? "wasm" : "out",
        outputPath
    );
    if (ec)
        throw std::runtime_error("não foi possível criar um ficheiro temporário: " + ec.message());

    std::string outputPathStr(outputPath.str());
    try {
        emit(outputPathStr, options);

        auto outputBuffer = llvm::MemoryBuffer::getFile(outputPathStr);
        if (!outputBuffer)
            throw std::runtime_error("não foi possível ler o resultado da compilação: " + outputBuffer.getError().message());

        llvm::StringRef bytes = outputBuffer.get()->getBuffer();
        std::vector<unsigned char> result(bytes.bytes_begin(), bytes.bytes_end());
        llvm::sys::fs::remove(outputPathStr);
        return result;
    } catch (...) {
        llvm::sys::fs::remove(outputPathStr);
        throw;
    }
}

void CodeGenVisitor::emit(const std::string& outputPath, const EmitOptions& options) {
    TargetResources resources = selectTargetResources(options.Target);

    std::vector<std::string> tempFiles;
    auto removeTempFiles = [&tempFiles] {
        for (const auto& path : tempFiles)
            llvm::sys::fs::remove(path);
    };

    try {
        llvm::SmallString<128> objPathBuf;
        std::error_code ec = llvm::sys::fs::createTemporaryFile(
            "kriol_object", isWindowsTriple(resources.Triple) ? "obj" : "o", objPathBuf);
        if (ec)
            throw std::runtime_error("não foi possível criar um ficheiro temporário: " + ec.message());
        const std::string objPath(objPathBuf.str());
        tempFiles.push_back(objPath);

        linkRuntimeBitcode(*Mod, Context, resources.Runtime);
        emitObjectFile(*Mod, resources.Triple, objPath, options.OptLevel);

        std::string tempLibGc = writeGcArchive(options.Target, resources.Triple, resources.GcArchive);
        if (!tempLibGc.empty())
            tempFiles.push_back(tempLibGc);

        if (options.Target == CodegenTarget::Wasm32Wasi) {
#if KRIOL_ENABLE_WASM
            WasiInputs wasiInputs = writeWasiInputs();
            tempFiles.insert(tempFiles.end(), {
                wasiInputs.Crt1Command, wasiInputs.Libc, wasiInputs.Libm, wasiInputs.Builtins
            });
            linkWasm(buildWasiLinkPlan(objPath, tempLibGc, wasiInputs, outputPath));
#else
            throw std::runtime_error("esta versão do compilador foi construída sem suporte para 'wasm32-wasi'");
#endif
        } else if (options.Target == CodegenTarget::X86_64Windows) {
#if KRIOL_ENABLE_WINDOWS_TARGET
            linkMingwExecutable(objPath, tempLibGc, outputPath);
#else
            throw std::runtime_error("esta versão do compilador foi construída sem suporte para 'x86_64-windows'");
#endif
        } else {
            linkNativeExecutable(objPath, tempLibGc, outputPath, resources.Triple);
        }
    } catch (...) {
        removeTempFiles();
        throw;
    }
    removeTempFiles();
}

void CodeGenVisitor::visit(VarDeclSttmt& node) {
    if (node.IsParam) return; // params are handled inside FuncDeclSttmt

    llvm::Type* ty = mapType(node.Type);

    // Module scope: CurrentFunction is null when we're outside any function
    if (!CurrentFunction) {
        llvm::Constant* init = llvm::Constant::getNullValue(ty);

        auto* gv = new llvm::GlobalVariable(
            *Mod,
            ty,
            /*isConstant=*/false,
            llvm::GlobalValue::InternalLinkage,
            init,
            llvmGlobalName(node.Name)
        );

        GlobalVars[node.Name] = gv;

        // Defer any initializer expression; emitted as stores at the top of inisiu
        if (node.Value)
            DeferredGlobalInits.push_back({gv, node.Value.get(), node.Type});

        LastValue = nullptr;
        return;
    }

    // Function scope
    llvm::AllocaInst* alloca = createEntryAlloca(CurrentFunction, node.Name, ty);
    declareVar(node.Name, alloca);

    if (isAggregate(node.Type)) {
        emitAggregateStore(alloca, node.Type, node.Value.get());
    } else {
        node.Value->accept(*this);
        if (LastValue) {
            LastValue = coerceToType(LastValue, node.Value->ResolvedType, node.Type);
            Builder->CreateStore(LastValue, alloca);
        }
    }
    LastValue = nullptr;
}

void CodeGenVisitor::emitArrayLoop(llvm::Value* storage,
                                   llvm::ArrayType* arrayTy,
                                   uint64_t first,
                                   llvm::function_ref<void(llvm::Value*)> body) {
    const uint64_t count = arrayTy->getNumElements();
    if (first >= count) return;

    // The range is known to be non-empty, so the body can run before the bound check.
    auto* i64Ty = llvm::Type::getInt64Ty(Context);
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* entryBB = Builder->GetInsertBlock();
    auto* loopBB = llvm::BasicBlock::Create(Context, "fill.loop", fn);
    auto* doneBB = llvm::BasicBlock::Create(Context, "fill.done", fn);
    Builder->CreateBr(loopBB);

    Builder->SetInsertPoint(loopBB);
    auto* index = Builder->CreatePHI(i64Ty, 2, "fill.index");
    index->addIncoming(llvm::ConstantInt::get(i64Ty, first), entryBB);
    body(createArrayElementPtr(storage, arrayTy, index));
    auto* next = Builder->CreateAdd(index, llvm::ConstantInt::get(i64Ty, 1), "fill.next");
    index->addIncoming(next, Builder->GetInsertBlock());
    auto* more = Builder->CreateICmpULT(next, llvm::ConstantInt::get(i64Ty, count), "fill.more");
    Builder->CreateCondBr(more, loopBB, doneBB);

    Builder->SetInsertPoint(doneBB);
}

void CodeGenVisitor::emitArrayInitializer(llvm::Value* storage,
                                          const Type& arrayType,
                                          ast::Expr* init) {
    auto* arrayTy = llvm::cast<llvm::ArrayType>(mapType(arrayType));
    llvm::Type* elemTy = arrayTy->getArrayElementType();
    const Type& elemType = arrayType.elementType();

    if (!dynamic_cast<ArrayLiteralExpr*>(init) && !dynamic_cast<ArrayRepeatExpr*>(init)) {
        emitAggregateCopy(storage, emitAggregateAddress(init, arrayType), arrayType);
        LastValue = nullptr;
        return;
    }

    auto* i64Ty = llvm::Type::getInt64Ty(Context);
    if (isAggregate(elemType)) {
        if (auto* initLit = dynamic_cast<ArrayLiteralExpr*>(init)) {
            for (size_t i = 0; i < initLit->Elements.size(); ++i)
                emitAggregateStore(createArrayElementPtr(storage, arrayTy, llvm::ConstantInt::get(i64Ty, i)),
                                   elemType, initLit->Elements[i].get());
        } else {
            // Build the first element, then copy it into the others.
            auto* initRep = static_cast<ArrayRepeatExpr*>(init);
            llvm::Value* first = createArrayElementPtr(storage, arrayTy, llvm::ConstantInt::get(i64Ty, 0));
            emitAggregateStore(first, elemType, initRep->Fill.get());
            emitArrayLoop(storage, arrayTy, 1, [&](llvm::Value* element) {
                emitAggregateCopy(element, first, elemType);
            });
        }
        LastValue = nullptr;
        return;
    }

    if (auto* initLit = dynamic_cast<ArrayLiteralExpr*>(init)) {
        for (size_t i = 0; i < initLit->Elements.size(); ++i) {
            initLit->Elements[i]->accept(*this);
            if (!LastValue) continue;
            llvm::Value* value = LastValue;
            if (value->getType() != elemTy)
                value = coerceToType(value, initLit->Elements[i]->ResolvedType, elemType);
            auto* index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), i);
            Builder->CreateStore(value, createArrayElementPtr(storage, arrayTy, index));
        }
    } else if (auto* initRep = dynamic_cast<ArrayRepeatExpr*>(init)) {
        initRep->Fill->accept(*this);
        llvm::Value* fill = LastValue
            ? coerceToType(LastValue, initRep->Fill->ResolvedType, elemType)
            : llvm::Constant::getNullValue(elemTy);
        emitArrayLoop(storage, arrayTy, 0, [&](llvm::Value* element) {
            Builder->CreateStore(fill, element);
        });
    } else {
        throw std::runtime_error("erro interno: o array precisa de uma lista ou de uma repetição");
    }
    LastValue = nullptr;
}

void CodeGenVisitor::visit(MoldaDeclSttmt&) {
    // Molda declarations are registered during the program-root prepass.
    // They define record types but do not emit runtime instructions.
}

llvm::Function* CodeGenVisitor::getOrDeclareKriolCheckBounds() {
    if (auto* fn = Mod->getFunction("__kriol_check_bounds")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i64Ty  = llvm::Type::getInt64Ty(Context);
    auto* i32Ty  = llvm::Type::getInt32Ty(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {i64Ty, i64Ty, i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_check_bounds", *Mod);
}

llvm::Function* CodeGenVisitor::getOrDeclareKriolCheckDiv() {
    if (auto* fn = Mod->getFunction("__kriol_check_div")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* i64Ty  = llvm::Type::getInt64Ty(Context);
    auto* i32Ty  = llvm::Type::getInt32Ty(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {i64Ty, i64Ty, i32Ty, i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_check_div", *Mod);
}

llvm::Function* CodeGenVisitor::getOrDeclareKriolCheckFloatDiv() {
    if (auto* fn = Mod->getFunction("__kriol_check_fdiv")) return fn;
    auto* voidTy   = llvm::Type::getVoidTy(Context);
    auto* doubleTy = llvm::Type::getDoubleTy(Context);
    auto* i32Ty    = llvm::Type::getInt32Ty(Context);
    auto* ftype    = llvm::FunctionType::get(voidTy, {doubleTy, i32Ty}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_check_fdiv", *Mod);
}

void CodeGenVisitor::startDeadBlock() {
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* dead = llvm::BasicBlock::Create(Context, "dead", fn);
    Builder->SetInsertPoint(dead);
}

void CodeGenVisitor::emitDivGuard(llvm::Value* lhs, llvm::Value* rhs,
                                  const Type& operandType, int lineNum) {
    auto* i64Ty = llvm::Type::getInt64Ty(Context);
    auto* i32Ty = llvm::Type::getInt32Ty(Context);

    if (operandType.isFloat()) {
        llvm::Value* divisor = Builder->CreateFPExt(rhs, llvm::Type::getDoubleTy(Context));
        Builder->CreateCall(getOrDeclareKriolCheckFloatDiv(), {divisor, llvm::ConstantInt::get(i32Ty, lineNum)});
        return;
    }

    llvm::Value* lhs64 = operandType.isSigned()
        ? Builder->CreateSExtOrTrunc(lhs, i64Ty)
        : Builder->CreateZExtOrTrunc(lhs, i64Ty);
    llvm::Value* rhs64 = operandType.isSigned()
        ? Builder->CreateSExtOrTrunc(rhs, i64Ty)
        : Builder->CreateZExtOrTrunc(rhs, i64Ty);

    // signedBits > 0 additionally enables the MIN / -1 overflow check.
    int signedBits = operandType.isSigned() ? static_cast<int>(operandType.bitWidth()) : 0;
    Builder->CreateCall(getOrDeclareKriolCheckDiv(), {
        lhs64,
        rhs64,
        llvm::ConstantInt::get(i32Ty, signedBits),
        llvm::ConstantInt::get(i32Ty, lineNum)
    });
}

llvm::Value* CodeGenVisitor::getVariableStorage(const std::string& name) {
    if (auto* alloca = lookupVar(name)) return alloca;
    if (auto* gv = lookupGlobal(name)) return gv;
    return nullptr;
}

llvm::Value* CodeGenVisitor::createArrayElementPtr(llvm::Value* storage,
                                                   llvm::Type* arrayTy,
                                                   llvm::Value* index) {
    auto* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), 0);
    return Builder->CreateGEP(arrayTy, storage, {zero, index}, "array.elem.ptr");
}

CodeGenVisitor::LValue CodeGenVisitor::resolveLValue(ast::Expr* expr, bool allowTemporary) {
    if (!expr) return {};

    if (auto* par = dynamic_cast<ParExpr*>(expr))
        return resolveLValue(par->Content.get(), allowTemporary);

    if (auto* ident = dynamic_cast<IdentExpr*>(expr)) {
        if (auto* alloca = lookupVar(ident->Name))
            return {alloca, alloca->getAllocatedType()};
        if (auto* gv = lookupGlobal(ident->Name))
            return {gv, gv->getValueType()};
        return {};
    }

    if (auto* arr = dynamic_cast<ArrayAccessExpr*>(expr)) {
        LValue base = resolveLValue(arr->Base.get(), allowTemporary);
        if (!base.Ptr || !base.Type || !base.Type->isArrayTy()) return {};

        if (!arr->Index) return {};
        arr->Index->accept(*this);
        if (!LastValue) return {};

        // An u8 holding 200 must stay 200.
        auto* i64Ty = llvm::Type::getInt64Ty(Context);
        const Type& indexType = arr->Index->ResolvedType;
        llvm::Value* idx = coerceToType(LastValue, indexType,
            indexType.isUnsignedInteger() ? Type::UnsignedInteger(64) : Type::SignedInteger(64));

        auto* sizeConst = llvm::ConstantInt::get(i64Ty, base.Type->getArrayNumElements());
        auto* lineConst = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), arr->LineNum);
        Builder->CreateCall(getOrDeclareKriolCheckBounds(), {idx, sizeConst, lineConst});

        llvm::Value* elemPtr = createArrayElementPtr(base.Ptr, base.Type, idx);
        return {elemPtr, base.Type->getArrayElementType()};
    }

    if (auto* member = dynamic_cast<MemberAccessExpr*>(expr)) {
        if (!member->Base || !member->Base->ResolvedType.isNamed()) return {};

        LValue base = resolveLValue(member->Base.get(), allowTemporary);
        if (!base.Ptr || !base.Type) return {};

        auto recordIt = Records.find(member->Base->ResolvedType.name());
        if (recordIt == Records.end()) return {};
        auto fieldIt = recordIt->second.fieldIndex.find(member->Member);
        if (fieldIt == recordIt->second.fieldIndex.end()) return {};

        unsigned fieldIndex = static_cast<unsigned>(fieldIt->second);
        llvm::StructType* structTy = getOrCreateRecordType(member->Base->ResolvedType.name());
        llvm::Value* fieldPtr = Builder->CreateStructGEP(
            structTy,
            base.Ptr,
            fieldIndex,
            "field.ptr"
        );
        return {fieldPtr, structTy->getElementType(fieldIndex)};
    }

    if (!allowTemporary) return {};
    expr->accept(*this);
    if (!LastValue) return {};
    // An aggregate value already evaluates to an address.
    if (isAggregate(expr->ResolvedType))
        return {LastValue, mapType(expr->ResolvedType)};
    auto* temporary = createEntryAlloca(CurrentFunction, "tmp", LastValue->getType());
    Builder->CreateStore(LastValue, temporary);
    return {temporary, LastValue->getType()};
}

void CodeGenVisitor::visit(ArrayAccessExpr& node) {
    LValue elem = resolveLValue(&node, true);
    if (!elem.Ptr || !elem.Type) { LastValue = nullptr; return; }
    // An aggregate element, such as a row, stays an address.
    LastValue = isAggregate(node.ResolvedType) ? elem.Ptr : Builder->CreateLoad(elem.Type, elem.Ptr, "array.elem");
}

void CodeGenVisitor::visit(MemberAccessExpr& node) {
    LValue field = resolveLValue(&node, true);
    if (!field.Ptr || !field.Type) { LastValue = nullptr; return; }
    LastValue = isAggregate(node.ResolvedType)
        ? field.Ptr
        : Builder->CreateLoad(field.Type, field.Ptr, "field." + node.Member);
}

void CodeGenVisitor::visit(QualifiedAccessExpr& node) {
    LastValue = nullptr;
}

void CodeGenVisitor::visit(ArrayLiteralExpr& node) {
    LastValue = node.ResolvedType.isArray() ? emitAggregateAddress(&node, node.ResolvedType) : nullptr;
}

void CodeGenVisitor::visit(ArrayRepeatExpr& node) {
    LastValue = node.ResolvedType.isArray() ? emitAggregateAddress(&node, node.ResolvedType) : nullptr;
}

void CodeGenVisitor::visit(RecordLiteralExpr& node) {
    LastValue = Records.count(node.TypeName) ? emitAggregateAddress(&node, Type::Named(node.TypeName)) : nullptr;
}

void CodeGenVisitor::emitRecordLiteral(llvm::Value* dest, RecordLiteralExpr& node) {
    auto recordIt = Records.find(node.TypeName);
    if (recordIt == Records.end()) return;

    llvm::StructType* structTy = getOrCreateRecordType(node.TypeName);
    std::vector<bool> initialized(recordIt->second.fields.size(), false);

    for (auto& field : node.Fields) {
        auto fieldIt = recordIt->second.fieldIndex.find(field.Name);
        if (fieldIt == recordIt->second.fieldIndex.end() || !field.Value) continue;

        const unsigned index = static_cast<unsigned>(fieldIt->second);
        llvm::Value* fieldPtr = Builder->CreateStructGEP(structTy, dest, index, "field.ptr");
        const Type& fieldType = recordIt->second.fields[index]->Type;
        initialized[index] = true;
        if (isAggregate(fieldType)) {
            emitAggregateStore(fieldPtr, fieldType, field.Value.get());
            continue;
        }
        field.Value->accept(*this);
        if (!LastValue) continue;
        Builder->CreateStore(coerceToType(LastValue, field.Value->ResolvedType, fieldType), fieldPtr);
    }

    for (std::size_t i = 0; i < initialized.size(); ++i) {
        if (initialized[i]) continue;
        llvm::Type* fieldTy = structTy->getElementType(static_cast<unsigned>(i));
        Builder->CreateStore(llvm::Constant::getNullValue(fieldTy),
                             Builder->CreateStructGEP(structTy, dest, static_cast<unsigned>(i), "field.ptr"));
    }
    LastValue = nullptr;
}

void CodeGenVisitor::registerRecord(ast::MoldaDeclSttmt& node) {
    if (Records.count(node.Name)) return;

    RecordInfo info;
    info.llvmType = llvm::StructType::create(Context, "molda." + node.Name);
    for (auto& field : node.Fields) {
        if (!field) continue;
        info.fieldIndex[field->Name] = info.fields.size();
        info.fields.push_back(field.get());
    }
    Records[node.Name] = std::move(info);
}

llvm::StructType* CodeGenVisitor::failableResultType(const Type& valueType) {
    std::vector<llvm::Type*> elements{llvm::Type::getInt1Ty(Context)};
    if (!valueType.isVoid())
        elements.push_back(abiType(valueType));
    elements.push_back(getOrCreateRecordType(prelude::ErrorTypeName));
    return llvm::StructType::get(Context, elements);
}

llvm::Type* CodeGenVisitor::functionReturnType(const ast::FuncDeclSttmt& node) {
    if (node.Name == "inisiu") return llvm::Type::getInt32Ty(Context);
    if (node.CanFail()) return failableResultType(node.Type);
    return abiType(node.Type);
}

llvm::Type* CodeGenVisitor::abiType(const Type& type) {
    return isAggregate(type) ? llvm::PointerType::getUnqual(Context) : mapType(type);
}

static bool isAggregateLiteral(Expr* expr) {
    return dynamic_cast<ArrayLiteralExpr*>(expr) || dynamic_cast<ArrayRepeatExpr*>(expr)
        || dynamic_cast<RecordLiteralExpr*>(expr);
}

llvm::Value* CodeGenVisitor::emitAggregateAddress(ast::Expr* expr, const Type& type) {
    if (isAggregateLiteral(expr)) {
        auto* temporary = createEntryAlloca(CurrentFunction, "aggregate.tmp", mapType(type));
        emitAggregateStore(temporary, type, expr);
        return temporary;
    }
    expr->accept(*this);
    if (!LastValue)
        throw std::runtime_error("erro interno: falhou a geração de um valor composto");
    return LastValue;
}

void CodeGenVisitor::emitAggregateStore(llvm::Value* dest, const Type& type, ast::Expr* expr) {
    if (auto* record = dynamic_cast<RecordLiteralExpr*>(expr))
        emitRecordLiteral(dest, *record);
    else if (type.isArray())
        emitArrayInitializer(dest, type, expr);
    else
        emitAggregateCopy(dest, emitAggregateAddress(expr, type), type);
    LastValue = nullptr;
}

void CodeGenVisitor::emitAggregateCopy(llvm::Value* dest, llvm::Value* src, const Type& type) {
    // memmove, because `a = a` or `m[0] = m[0]` may name the same storage.
    Builder->CreateMemMove(dest, llvm::MaybeAlign(), src, llvm::MaybeAlign(),
                           llvm::ConstantExpr::getSizeOf(mapType(type)));
}

llvm::Value* CodeGenVisitor::erruMessage(llvm::Value* erru) {
    const auto& fields = Records.at(prelude::ErrorTypeName).fieldIndex;
    const unsigned index = static_cast<unsigned>(fields.at(prelude::ErrorMessageField));
    return Builder->CreateExtractValue(erru, {index}, "error.message");
}

llvm::Value* CodeGenVisitor::makeFailableResult(const Type& valueType, llvm::Value* failed,
                                                llvm::Value* value, llvm::Value* message) {
    auto* resultTy = failableResultType(valueType);
    const unsigned erruIndex = resultTy->getNumElements() - 1;
    auto* erruTy = llvm::cast<llvm::StructType>(resultTy->getElementType(erruIndex));
    const auto& fields = Records.at(prelude::ErrorTypeName).fieldIndex;
    const unsigned messageIndex = static_cast<unsigned>(fields.at(prelude::ErrorMessageField));
    llvm::Value* erru = Builder->CreateInsertValue(llvm::ConstantAggregateZero::get(erruTy), message, {messageIndex});

    llvm::Value* result = llvm::ConstantAggregateZero::get(resultTy);
    result = Builder->CreateInsertValue(result, failed, {0u});
    if (value)
        result = Builder->CreateInsertValue(result, value, {1u});
    return Builder->CreateInsertValue(result, erru, {erruIndex}, "result");
}

static llvm::Function* getOrDeclareUnhandledError(llvm::Module& Mod, llvm::LLVMContext& Context) {
    if (auto* fn = Mod.getFunction("__kriol_unhandled_error")) return fn;
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(Context),
                                          {ptrTy, llvm::Type::getInt32Ty(Context)}, false);
    auto* fn = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage,
                                      "__kriol_unhandled_error", Mod);
    fn->addFnAttr(llvm::Attribute::NoReturn);
    return fn;
}

void CodeGenVisitor::emitFailure(llvm::Value* erruValue, int lineNum) {
    if (CurrentIsMain) {
        Builder->CreateCall(getOrDeclareUnhandledError(*Mod, Context),
                            {erruMessage(erruValue), llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), lineNum)});
        Builder->CreateUnreachable();
        return;
    }

    auto* resultTy = llvm::cast<llvm::StructType>(CurrentFunction->getReturnType());
    llvm::Value* result = llvm::ConstantAggregateZero::get(resultTy);
    result = Builder->CreateInsertValue(result, llvm::ConstantInt::getTrue(Context), {0u});
    result = Builder->CreateInsertValue(result, erruValue, {resultTy->getNumElements() - 1}, "failed");
    Builder->CreateRet(result);
}

void CodeGenVisitor::emitConversionCall(TypeCallExpr& node) {
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    const Type& target = node.OwnerType;

    node.Args->Args[0]->accept(*this);
    llvm::Value* text = LastValue;
    if (!text)
        throw std::runtime_error("erro interno: falhou a geração do texto a converter");

    auto declare = [&](const char* name, std::vector<llvm::Type*> params) {
        if (auto* fn = Mod->getFunction(name)) return fn;
        auto* ftype = llvm::FunctionType::get(i32Ty, params, false);
        return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, name, *Mod);
    };

    auto* errorSlot = createEntryAlloca(CurrentFunction, "konverti.error", ptrTy);
    Builder->CreateStore(llvm::ConstantPointerNull::get(ptrTy), errorSlot);
    auto* bits = llvm::ConstantInt::get(i32Ty, target.bitWidth());

    llvm::Type* slotTy;
    llvm::AllocaInst* slot;
    llvm::Value* ok;
    if (target.isInteger()) {
        slotTy = llvm::Type::getInt64Ty(Context);
        slot = createEntryAlloca(CurrentFunction, "konverti.value", slotTy);
        ok = Builder->CreateCall(declare("__kriol_convert_int", {ptrTy, ptrTy, i32Ty, i32Ty, ptrTy}),
                                 {text, slot, bits, llvm::ConstantInt::get(i32Ty, target.isSigned() ? 1 : 0),
                                  errorSlot});
    } else if (target.isFloat()) {
        slotTy = llvm::Type::getDoubleTy(Context);
        slot = createEntryAlloca(CurrentFunction, "konverti.value", slotTy);
        ok = Builder->CreateCall(declare("__kriol_convert_float", {ptrTy, ptrTy, i32Ty, ptrTy}),
                                 {text, slot, bits, errorSlot});
    } else {
        slotTy = i32Ty;
        slot = createEntryAlloca(CurrentFunction, "konverti.value", slotTy);
        ok = Builder->CreateCall(declare("__kriol_convert_bool", {ptrTy, ptrTy, ptrTy}),
                                 {text, slot, errorSlot});
    }

    llvm::Value* raw = Builder->CreateLoad(slotTy, slot, "konverti.raw");
    llvm::Value* value;
    if (target.isInteger())
        value = Builder->CreateTrunc(raw, mapType(target), "konverti.int");  // no-op for 64 bits
    else if (target.isFloat())
        value = Builder->CreateFPTrunc(raw, mapType(target), "konverti.float");
    else
        value = Builder->CreateICmpNE(raw, llvm::ConstantInt::get(i32Ty, 0), "konverti.bool");

    llvm::Value* message = Builder->CreateLoad(ptrTy, errorSlot, "konverti.message");
    LastValue = makeFailableResult(target, Builder->CreateIsNull(ok, "konverti.failed"), value, message);
}

void CodeGenVisitor::visit(TypeCallExpr& node) {
    emitConversionCall(node);
    emitUnhandledFailureCheck(node);
}

void CodeGenVisitor::emitUnhandledFailureCheck(FunCallExpr& node) {
    if (!node.Fallible || node.ErrorHandled || !LastValue) return;

    llvm::Value* result = LastValue;
    auto* resultTy = llvm::cast<llvm::StructType>(result->getType());
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* failBB = llvm::BasicBlock::Create(Context, "unhandled.fail", fn);
    auto* okBB = llvm::BasicBlock::Create(Context, "unhandled.ok", fn);
    Builder->CreateCondBr(Builder->CreateExtractValue(result, {0u}, "failed"), failBB, okBB);

    Builder->SetInsertPoint(failBB);
    llvm::Value* error = Builder->CreateExtractValue(result, {resultTy->getNumElements() - 1}, "error");
    Builder->CreateCall(getOrDeclareUnhandledError(*Mod, Context),
                        {erruMessage(error), llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), node.LineNum)});
    Builder->CreateUnreachable();

    Builder->SetInsertPoint(okBB);
    LastValue = node.ResolvedType.isVoid() ? nullptr : Builder->CreateExtractValue(result, {1u}, "value");
}

void CodeGenVisitor::emitTenta(UnaryExpr& node) {
    node.Operand->accept(*this);
    llvm::Value* result = LastValue;
    if (!result)
        throw std::runtime_error("erro interno: falhou a geração da chamada depois de 'tenta'");

    auto* resultTy = llvm::cast<llvm::StructType>(result->getType());
    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* failBB = llvm::BasicBlock::Create(Context, "tenta.fail", fn);
    auto* okBB = llvm::BasicBlock::Create(Context, "tenta.ok", fn);
    llvm::Value* failed = Builder->CreateExtractValue(result, {0u}, "failed");
    Builder->CreateCondBr(failed, failBB, okBB);

    Builder->SetInsertPoint(failBB);
    emitFailure(Builder->CreateExtractValue(result, {resultTy->getNumElements() - 1}, "error"), node.LineNum);

    Builder->SetInsertPoint(okBB);
    LastValue = node.ResolvedType.isVoid() ? nullptr : Builder->CreateExtractValue(result, {1u}, "value");
}

void CodeGenVisitor::emitSinon(BinExpr& node) {
    node.LHS->accept(*this);
    llvm::Value* result = LastValue;
    if (!result)
        throw std::runtime_error("erro interno: falhou a geração da chamada antes de 'sinon'");

    auto* fn = Builder->GetInsertBlock()->getParent();
    auto* fallbackBB = llvm::BasicBlock::Create(Context, "sinon.fallback", fn);
    auto* mergeBB = llvm::BasicBlock::Create(Context, "sinon.merge", fn);
    llvm::Value* failed = Builder->CreateExtractValue(result, {0u}, "failed");
    llvm::Value* value = Builder->CreateExtractValue(result, {1u}, "value");
    llvm::BasicBlock* okEndBB = Builder->GetInsertBlock();
    Builder->CreateCondBr(failed, fallbackBB, mergeBB);

    // The fallback is only evaluated when the call failed.
    Builder->SetInsertPoint(fallbackBB);
    node.RHS->accept(*this);
    if (!LastValue)
        throw std::runtime_error("erro interno: falhou a geração do valor depois de 'sinon'");
    llvm::Value* fallback = isAggregate(node.ResolvedType)
        ? LastValue
        : coerceToType(LastValue, node.RHS->ResolvedType, node.ResolvedType);
    llvm::BasicBlock* fallbackEndBB = Builder->GetInsertBlock();
    Builder->CreateBr(mergeBB);

    Builder->SetInsertPoint(mergeBB);
    auto* phi = Builder->CreatePHI(value->getType(), 2, "sinon.value");
    phi->addIncoming(value, okEndBB);
    phi->addIncoming(fallback, fallbackEndBB);
    LastValue = phi;
}

CodeGenVisitor::FuncSig CodeGenVisitor::makeFuncSig(const ast::FuncDeclSttmt& node) const {
    FuncSig sig;
    sig.retType = node.Name == "inisiu" ? Type::SignedInteger(32) : node.Type;
    if (node.Args)
        for (auto& arg : node.Args->Args)
            sig.paramTypes.push_back(arg->Type);
    return sig;
}

llvm::Function* CodeGenVisitor::declareFunction(const ast::FuncDeclSttmt& node) {
    const std::string name = llvmFunctionName(node.Name);
    if (auto* fn = Mod->getFunction(name)) return fn;

    std::vector<llvm::Type*> paramTypes;
    if (isAggregate(node.Type))
        paramTypes.push_back(llvm::PointerType::getUnqual(Context));
    if (node.Args)
        for (auto& arg : node.Args->Args)
            paramTypes.push_back(abiType(arg->Type));

    auto* ftype = llvm::FunctionType::get(functionReturnType(node), paramTypes, false);
    return llvm::Function::Create(ftype, functionLinkage(node.Name), name, *Mod);
}

void CodeGenVisitor::forwardDeclareFunc(ast::FuncDeclSttmt& node) {
    FunctionSigs[llvmFunctionName(node.Name)] = makeFuncSig(node);
    declareFunction(node);
}

void CodeGenVisitor::visit(BlockSttmt& node) {
    // At program root forward-declare all user functions
    // so that forward calls and mutual recursion resolve in codegen.
    if (!CurrentFunction) {
        registerRecord(*ErrorTypeDecl);
        for (auto& s : node.SttmtList)
            if (auto* rec = dynamic_cast<MoldaDeclSttmt*>(s.get()))
                registerRecord(*rec);

        for (auto& s : node.SttmtList)
            if (auto* fn = dynamic_cast<FuncDeclSttmt*>(s.get()))
                forwardDeclareFunc(*fn);

        // Also visit all top-level variable declarations first so deferred
        // global initializers are complete before main is emitted.
        for (auto& s : node.SttmtList)
            if (auto* v = dynamic_cast<VarDeclSttmt*>(s.get()))
                v->accept(*this);
    }

    pushScope();
    for (auto& s : node.SttmtList) {
        if (!s) continue;
        // At the program root only function bodies are emitted; molda and
        // variable declarations were handled in the prepass above, and sema
        // rejects any other top-level statement before codegen runs.
        if (!CurrentFunction && !dynamic_cast<FuncDeclSttmt*>(s.get())) continue;
        s->accept(*this);
    }
    popScope();
}

void CodeGenVisitor::visit(FuncArgs& node) {
    // Handled inside FuncDeclSttmt
}

static llvm::Function* getOrDeclareKriolGcInit(llvm::Module& Mod, llvm::LLVMContext& Context)
{
    if (auto* fn = Mod.getFunction("__kriol_gc_init")) return fn;
    auto* voidTy = llvm::Type::getVoidTy(Context);
    auto* ftype  = llvm::FunctionType::get(voidTy, {}, false);
    return llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__kriol_gc_init", Mod);
}

static llvm::Function* emitWasiMainWrapper(
    llvm::Module& Mod,
    llvm::LLVMContext& Context,
    llvm::IRBuilder<>& Builder,
    llvm::Function* mainFn
) {
    if (auto* wrapper = Mod.getFunction(__WASI_MAIN)) return wrapper;

    auto* i32Ty = llvm::Type::getInt32Ty(Context);
    auto* ptrTy = llvm::PointerType::getUnqual(Context);
    auto* wrapperTy = llvm::FunctionType::get(i32Ty, {i32Ty, ptrTy}, false);
    auto* wrapper = llvm::Function::Create(
        wrapperTy,
        llvm::Function::ExternalLinkage,
        __WASI_MAIN,
        Mod
    );

    auto arg = wrapper->arg_begin();
    arg->setName("argc");
    (++arg)->setName("argv");

    auto* entry = llvm::BasicBlock::Create(Context, "entry", wrapper);
    Builder.SetInsertPoint(entry);
    Builder.CreateRet(Builder.CreateCall(mainFn, {}));
    return wrapper;
}

void CodeGenVisitor::visit(FuncDeclSttmt& node) {
    bool isMain = (node.Name == "inisiu");
    std::string name = llvmFunctionName(node.Name);

    FuncSig sig = makeFuncSig(node);
    FunctionSigs[name] = sig;

    auto* fn = declareFunction(node);
    llvm::Type* retTy = fn->getReturnType();

    const unsigned firstParam = isAggregate(node.Type) ? 1 : 0;
    if (firstParam) fn->getArg(0)->setName("return.slot");
    if (node.Args)
        for (size_t i = 0; i < node.Args->Args.size(); ++i)
            fn->getArg(firstParam + i)->setName(node.Args->Args[i]->Name);

    auto* entry = llvm::BasicBlock::Create(Context, "entry", fn);
    Builder->SetInsertPoint(entry);
    CurrentFunction = fn;
    CurrentReturnType = sig.retType;
    CurrentCanFail = node.CanFail();
    CurrentIsMain = isMain;
    CurrentReturnSlot = firstParam ? fn->getArg(0) : nullptr;

    pushScope();
    if (node.Args)
        for (size_t i = 0; i < node.Args->Args.size(); ++i) {
            auto& p = node.Args->Args[i];
            llvm::Argument* llvmArg = fn->getArg(firstParam + i);
            auto* a = createEntryAlloca(fn, p->Name, mapType(p->Type));
            // An aggregate argument is the caller's address; the parameter is a copy.
            if (isAggregate(p->Type))
                emitAggregateCopy(a, llvmArg, p->Type);
            else
                Builder->CreateStore(llvmArg, a);
            declareVar(p->Name, a);
        }

    // Initialise the Boehm GC as the very first thing in main.
    if (isMain) {
        auto* gcInit = getOrDeclareKriolGcInit(*Mod, Context);
        Builder->CreateCall(gcInit, {});
    }

    // Emit deferred global initializers at the top of inisiu (main)
    if (isMain && !DeferredGlobalInits.empty()) {
        for (auto& di : DeferredGlobalInits) {
            if (isAggregate(di.TargetType)) {
                emitAggregateStore(di.Var, di.TargetType, di.InitExpr);
                continue;
            }
            di.InitExpr->accept(*this);
            if (LastValue) {
                LastValue = coerceToType(LastValue, di.InitExpr->ResolvedType, di.TargetType);
                Builder->CreateStore(LastValue, di.Var);
            }
        }
        DeferredGlobalInits.clear();
    }

    if (node.Body) node.Body->accept(*this);
    popScope();

    // Auto-return if the current block has no terminator
    if (!Builder->GetInsertBlock()->getTerminator()) {
        llvm::BasicBlock* cur = Builder->GetInsertBlock();
        // If every branch already returned, the final block is dead code
        // (unreachable from entry) but still needs a valid LLVM terminator.
        if (!isReachableFromEntry(cur))
            Builder->CreateUnreachable();
        else if (isMain)
            Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0));
        else if (retTy->isVoidTy())
            Builder->CreateRetVoid();
        else if (node.CanFail() && node.Type.isVoid())
            Builder->CreateRet(llvm::ConstantAggregateZero::get(llvm::cast<llvm::StructType>(retTy)));
        else
            throw std::runtime_error("erro interno: a função '" + node.Name + "' não tem 'divolvi'");
    }

    auto verificationFailed = llvm::verifyFunction(*fn);

    if (verificationFailed) {
        fn->print(llvm::errs());
        throw std::runtime_error("erro interno: o código gerado para a função '" + node.Name + "' é inválido");
    }

    if (isMain && CurrentTarget == CodegenTarget::Wasm32Wasi) {
        auto* wrapper = emitWasiMainWrapper(*Mod, Context, *Builder, fn);
        if (llvm::verifyFunction(*wrapper)) {
            wrapper->print(llvm::errs());
            throw std::runtime_error("erro interno: o código gerado para o início do módulo WASI é inválido");
        }
    }

    CurrentFunction = nullptr;
    CurrentReturnType = Type::Invalid();
    CurrentCanFail = false;
    CurrentIsMain = false;
    CurrentReturnSlot = nullptr;
}

void CodeGenVisitor::visit(IfSttmt& node) {
    if (node.Init) {
        pushScope();
        node.Init->accept(*this);
    }

    node.Cond->accept(*this);
    llvm::Value* cond = toBool(LastValue);

    auto* fn      = Builder->GetInsertBlock()->getParent();
    auto* thenBB  = llvm::BasicBlock::Create(Context, "then",   fn);
    // A block never inserted into a function has no owner and would leak.
    auto* elseBB  = node.Else ? llvm::BasicBlock::Create(Context, "else") : nullptr;
    auto* mergeBB = llvm::BasicBlock::Create(Context, "ifcont");

    Builder->CreateCondBr(cond, thenBB, elseBB ? elseBB : mergeBB);

    // then
    Builder->SetInsertPoint(thenBB);
    if (node.Then) node.Then->accept(*this);
    if (!Builder->GetInsertBlock()->getTerminator())
        Builder->CreateBr(mergeBB);

    // else
    if (node.Else) {
        fn->insert(fn->end(), elseBB);
        Builder->SetInsertPoint(elseBB);
        node.Else->accept(*this);
        if (!Builder->GetInsertBlock()->getTerminator())
            Builder->CreateBr(mergeBB);
    }

    fn->insert(fn->end(), mergeBB);
    Builder->SetInsertPoint(mergeBB);
    if (node.Init) popScope();
    LastValue = nullptr;
}

void CodeGenVisitor::visit(WhileSttmt& node) {
    auto* fn     = Builder->GetInsertBlock()->getParent();
    auto* condBB = llvm::BasicBlock::Create(Context, "while.cond", fn);
    auto* bodyBB = llvm::BasicBlock::Create(Context, "while.body", fn);
    auto* exitBB = llvm::BasicBlock::Create(Context, "while.exit", fn);

    auto* savedExit = LoopExit;
    auto* savedCont = LoopContinue;
    LoopExit = exitBB; LoopContinue = condBB;

    Builder->CreateBr(condBB);
    Builder->SetInsertPoint(condBB);
    node.Cond->accept(*this);
    Builder->CreateCondBr(toBool(LastValue), bodyBB, exitBB);

    Builder->SetInsertPoint(bodyBB);
    if (node.Do) node.Do->accept(*this);
    if (!Builder->GetInsertBlock()->getTerminator())
        Builder->CreateBr(condBB);

    Builder->SetInsertPoint(exitBB);
    LoopExit = savedExit; LoopContinue = savedCont;
    LastValue = nullptr;
}

void CodeGenVisitor::visit(JumpSttmt& node) {
    if (node.Name == "kebra" && LoopExit) {
        Builder->CreateBr(LoopExit);
        startDeadBlock();
    } else if (node.Name == "kontinua" && LoopContinue) {
        Builder->CreateBr(LoopContinue);
        startDeadBlock();
    }
}

void CodeGenVisitor::visit(ReturnSttmt& node) {
    llvm::Type* retTy = Builder->GetInsertBlock()->getParent()->getReturnType();
    if (node.Throws) {
        llvm::Value* erru = emitAggregateAddress(node.ReturnValue.get(), node.ReturnValue->ResolvedType);
        emitFailure(Builder->CreateLoad(getOrCreateRecordType(prelude::ErrorTypeName), erru, "erru"), node.LineNum);
    } else if (isAggregate(CurrentReturnType)) {
        emitAggregateStore(CurrentReturnSlot, CurrentReturnType, node.ReturnValue.get());
        if (CurrentCanFail) {
            auto* resultTy = llvm::cast<llvm::StructType>(retTy);
            Builder->CreateRet(Builder->CreateInsertValue(llvm::ConstantAggregateZero::get(resultTy),
                                                          CurrentReturnSlot, {1u}));
        } else {
            Builder->CreateRet(CurrentReturnSlot);
        }
    } else if (CurrentCanFail && !CurrentIsMain) {
        auto* resultTy = llvm::cast<llvm::StructType>(retTy);
        llvm::Value* result = llvm::ConstantAggregateZero::get(resultTy);
        if (node.ReturnValue) {
            node.ReturnValue->accept(*this);
            llvm::Value* value = coerceToType(LastValue, node.ReturnValue->ResolvedType, CurrentReturnType);
            result = Builder->CreateInsertValue(result, value, {1u});
        }
        Builder->CreateRet(result);
    } else if (node.ReturnValue) {
        node.ReturnValue->accept(*this);
        // Coerce to the function's declared return type (e.g. int -> num widening)
        if (LastValue && LastValue->getType() != retTy)
            LastValue = coerceToType(LastValue, node.ReturnValue->ResolvedType, CurrentReturnType);
        Builder->CreateRet(LastValue);
    } else if (retTy->isVoidTy()) {
        Builder->CreateRetVoid();
    } else {
        // A bare 'divolvi;' inside inisiu: main returns i32, so return 0
        // instead of emitting an invalid 'ret void'.
        Builder->CreateRet(llvm::Constant::getNullValue(retTy));
    }
    // Anything else in this source block is unreachable; give it a fresh
    // block so it still produces structurally valid IR.
    startDeadBlock();
    LastValue = nullptr;
}

void CodeGenVisitor::visit(FuncCallArgs& node) {
    // Handled inside FunCallExpr
}

void CodeGenVisitor::visit(FunCallExpr& node) {
    auto* callee = unwrapIdentExpr(node.Callee.get());
    if (!callee) { LastValue = nullptr; return; }
    if (emitPreludeCall(node, callee->Name)) {
        emitUnhandledFailureCheck(node);
        return;
    }

    const std::string name = llvmFunctionName(callee->Name);
    auto* fn = Mod->getFunction(name);
    if (!fn) { LastValue = nullptr; return; }
    auto sigIt = FunctionSigs.find(name);

    std::vector<llvm::Value*> callArgs;
    if (isAggregate(node.ResolvedType))
        callArgs.push_back(createEntryAlloca(CurrentFunction, "call.result", mapType(node.ResolvedType)));
    const size_t firstParam = callArgs.size();
    if (node.Args) {
        size_t i = 0;
        for (auto& arg : node.Args->Args) {
            const bool arrayParam = sigIt != FunctionSigs.end() && i < sigIt->second.paramTypes.size()
                && isAggregate(sigIt->second.paramTypes[i]);
            if (arrayParam) {
                callArgs.push_back(emitAggregateAddress(arg.get(), sigIt->second.paramTypes[i]));
                ++i;
                continue;
            }
            arg->accept(*this);
            if (!LastValue)
                throw std::runtime_error("erro interno: falhou a geração do argumento "
                                         + std::to_string(i + 1) + " da chamada a '" + callee->Name + "'");
            if (firstParam + i < fn->arg_size()) {
                // Coerce argument to the declared parameter type when they differ
                llvm::Type* paramTy = fn->getArg(firstParam + i)->getType();
                if (LastValue->getType() != paramTy) {
                    if (sigIt != FunctionSigs.end() && i < sigIt->second.paramTypes.size())
                        LastValue = coerceToType(LastValue, arg->ResolvedType, sigIt->second.paramTypes[i]);
                    else
                        LastValue = coerce(LastValue, paramTy);
                }
            }
            callArgs.push_back(LastValue);
            ++i;
        }
    }

    LastValue = Builder->CreateCall(fn, callArgs);
    emitUnhandledFailureCheck(node);
}

void CodeGenVisitor::emitShortCircuit(BinExpr& node) {
    const bool isAnd = (node.Op == "&&");
    auto* i1Ty = llvm::Type::getInt1Ty(Context);

    node.LHS->accept(*this);
    if (!LastValue) { LastValue = nullptr; return; }
    llvm::Value* lhs = toBool(LastValue);

    auto* fn      = Builder->GetInsertBlock()->getParent();
    auto* rhsBB   = llvm::BasicBlock::Create(Context, isAnd ? "and.rhs" : "or.rhs", fn);
    auto* mergeBB = llvm::BasicBlock::Create(Context, isAnd ? "and.merge" : "or.merge", fn);
    llvm::BasicBlock* lhsEndBB = Builder->GetInsertBlock();

    // '&&' only evaluates the RHS when the LHS is true;
    // '||' only evaluates the RHS when the LHS is false.
    if (isAnd)
        Builder->CreateCondBr(lhs, rhsBB, mergeBB);
    else
        Builder->CreateCondBr(lhs, mergeBB, rhsBB);

    Builder->SetInsertPoint(rhsBB);
    node.RHS->accept(*this);
    if (!LastValue)
        throw std::runtime_error("erro interno: falhou a geração do operando direito de '" + node.Op + "'");
    llvm::Value* rhs = toBool(LastValue);
    llvm::BasicBlock* rhsEndBB = Builder->GetInsertBlock();
    Builder->CreateBr(mergeBB);

    Builder->SetInsertPoint(mergeBB);
    auto* phi = Builder->CreatePHI(i1Ty, 2, isAnd ? "andtmp" : "ortmp");
    phi->addIncoming(llvm::ConstantInt::get(i1Ty, isAnd ? 0 : 1), lhsEndBB);
    phi->addIncoming(rhs, rhsEndBB);
    LastValue = phi;
}

void CodeGenVisitor::visit(BinExpr& node) {
    const auto& op = node.Op;
    if (op == "&&" || op == "||") { emitShortCircuit(node); return; }
    if (op == "sinon") { emitSinon(node); return; }

    node.LHS->accept(*this);
    llvm::Value* lhs = LastValue;
    node.RHS->accept(*this);
    llvm::Value* rhs = LastValue;
    if (!lhs || !rhs) { LastValue = nullptr; return; }

    Type operandType = promotedNumericTypeForExpr(node.LHS.get(), node.RHS.get(),
                                                  node.LHS->ResolvedType,
                                                  node.RHS->ResolvedType);
    llvm::Type* operandLlvmTy = mapType(operandType);
    bool isFloat = operandType.isFloat();
    bool isUnsigned = operandType.isUnsignedInteger();
    if (lhs->getType() != operandLlvmTy)
        lhs = coerceToType(lhs, node.LHS->ResolvedType, operandType);
    if (rhs->getType() != operandLlvmTy)
        rhs = coerceToType(rhs, node.RHS->ResolvedType, operandType);

    if      (op == "<")  LastValue = isFloat ? Builder->CreateFCmpOLT(lhs, rhs) : (isUnsigned ? Builder->CreateICmpULT(lhs, rhs) : Builder->CreateICmpSLT(lhs, rhs));
    else if (op == ">")  LastValue = isFloat ? Builder->CreateFCmpOGT(lhs, rhs) : (isUnsigned ? Builder->CreateICmpUGT(lhs, rhs) : Builder->CreateICmpSGT(lhs, rhs));
    else if (op == "<=") LastValue = isFloat ? Builder->CreateFCmpOLE(lhs, rhs) : (isUnsigned ? Builder->CreateICmpULE(lhs, rhs) : Builder->CreateICmpSLE(lhs, rhs));
    else if (op == ">=") LastValue = isFloat ? Builder->CreateFCmpOGE(lhs, rhs) : (isUnsigned ? Builder->CreateICmpUGE(lhs, rhs) : Builder->CreateICmpSGE(lhs, rhs));
    else if (op == "==") LastValue = isFloat ? Builder->CreateFCmpOEQ(lhs, rhs) : Builder->CreateICmpEQ(lhs, rhs);
    else if (op == "!=") LastValue = isFloat ? Builder->CreateFCmpONE(lhs, rhs) : Builder->CreateICmpNE(lhs, rhs);
    else LastValue = emitArithmetic(op, lhs, rhs, operandType, node.LineNum);
}

llvm::Value* CodeGenVisitor::emitArithmetic(const std::string& op, llvm::Value* lhs, llvm::Value* rhs,
                                            const Type& operandType, int lineNum) {
    const bool isFloat = operandType.isFloat();
    const bool isUnsigned = operandType.isUnsignedInteger();
    if (op == "/" || op == "%")
        emitDivGuard(lhs, rhs, operandType, lineNum);

    if (op == "+") return isFloat ? Builder->CreateFAdd(lhs, rhs) : Builder->CreateAdd(lhs, rhs);
    if (op == "-") return isFloat ? Builder->CreateFSub(lhs, rhs) : Builder->CreateSub(lhs, rhs);
    if (op == "*") return isFloat ? Builder->CreateFMul(lhs, rhs) : Builder->CreateMul(lhs, rhs);
    if (op == "/") return isFloat ? Builder->CreateFDiv(lhs, rhs) : (isUnsigned ? Builder->CreateUDiv(lhs, rhs) : Builder->CreateSDiv(lhs, rhs));
    if (op == "%") return isFloat ? Builder->CreateFRem(lhs, rhs) : (isUnsigned ? Builder->CreateURem(lhs, rhs) : Builder->CreateSRem(lhs, rhs));
    if (op == "&") return Builder->CreateAnd(lhs, rhs);
    if (op == "|") return Builder->CreateOr(lhs, rhs);
    if (op == "^") return Builder->CreateXor(lhs, rhs);
    throw std::runtime_error("erro interno: operador aritmético desconhecido '" + op + "'");
}

void CodeGenVisitor::visit(LiteralExpr& node) {
    const auto& t = node.Type;
    const auto& v = node.Value;

    if (t.isFloat()) {
        if (t.bitWidth() == 32)
            LastValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), std::stod(v));
        else
            LastValue = llvm::ConstantFP::get(Context, llvm::APFloat(std::stod(v)));
    } else if (t.isInteger()) {
        LastValue = llvm::ConstantInt::get(mapType(t), std::stoll(v), t.isSigned());
    } else if (t == Type::Bool()) {
        LastValue = llvm::ConstantInt::get(llvm::Type::getInt1Ty(Context),
                                           std::stoi(v) != 0);
    } else if (t == Type::Text()) {
        std::string s = v;
        if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
            s = s.substr(1, s.size() - 2);
        LastValue = Builder->CreateGlobalString(processEscapes(s));
    } else {
        LastValue = nullptr;
    }
}

void CodeGenVisitor::visit(ExprSttmt& node) {
    if (node.Expression) node.Expression->accept(*this);
    LastValue = nullptr;
}

void CodeGenVisitor::visit(IdentExpr& node) {
    if (isAggregate(node.ResolvedType)) {
        LastValue = getVariableStorage(node.Name);
        return;
    }
    auto* alloca = lookupVar(node.Name);
    if (alloca) {
        LastValue = Builder->CreateLoad(alloca->getAllocatedType(), alloca, node.Name);
        return;
    }
    auto* gv = lookupGlobal(node.Name);
    if (gv) {
        LastValue = Builder->CreateLoad(gv->getValueType(), gv, node.Name);
        return;
    }
    LastValue = nullptr;
}

void CodeGenVisitor::visit(ParExpr& node) {
    if (node.Content) node.Content->accept(*this);
}

void CodeGenVisitor::visit(AssignExpr& node) {
    const Type& assigneeType = node.Assignee->ResolvedType;
    if (isAggregate(assigneeType)) {
        // The value is built first, so `a = [a[1], a[0]]` reads the old elements.
        llvm::Value* src = emitAggregateAddress(node.Assigned.get(), assigneeType);
        LValue dest = resolveLValue(node.Assignee.get());
        if (!dest.Ptr) { LastValue = nullptr; return; }
        emitAggregateCopy(dest.Ptr, src, assigneeType);
        LastValue = dest.Ptr;
        return;
    }

    if (node.Assigned) node.Assigned->accept(*this);
    llvm::Value* val = LastValue;
    if (!val) { LastValue = nullptr; return; }

    LValue dest = resolveLValue(node.Assignee.get());
    if (!dest.Ptr || !dest.Type) { LastValue = nullptr; return; }

    if (node.AssignOp != "=") {
        // Like C, `x op= v` computes `x op v` in the promoted type of both
        // operands and converts the result back to the type of x.
        const Type& targetType = node.Assignee->ResolvedType;
        const Type& valueType = node.Assigned->ResolvedType;
        const Type operandType = promotedNumericTypeForExpr(node.Assignee.get(), node.Assigned.get(),
                                                            targetType, valueType);
        llvm::Value* cur = coerceToType(Builder->CreateLoad(dest.Type, dest.Ptr), targetType, operandType);
        llvm::Value* rhs = coerceToType(val, valueType, operandType);
        const std::string op = node.AssignOp.substr(0, node.AssignOp.size() - 1);
        val = coerceToType(emitArithmetic(op, cur, rhs, operandType, node.LineNum), operandType, targetType);
    } else {
        val = coerceToType(val, node.Assigned->ResolvedType, node.Assignee->ResolvedType);
    }
    Builder->CreateStore(val, dest.Ptr);
    LastValue = val;
    return;
}

void CodeGenVisitor::visit(ForSttmt& node) {
    auto* fn      = Builder->GetInsertBlock()->getParent();
    auto* condBB  = llvm::BasicBlock::Create(Context, "for.cond",  fn);
    auto* bodyBB  = llvm::BasicBlock::Create(Context, "for.body",  fn);
    auto* afterBB = llvm::BasicBlock::Create(Context, "for.after", fn);
    auto* exitBB  = llvm::BasicBlock::Create(Context, "for.exit",  fn);

    auto* savedExit = LoopExit;
    auto* savedCont = LoopContinue;
    LoopExit = exitBB; LoopContinue = afterBB;

    pushScope();
    if (node.Start) node.Start->accept(*this);
    Builder->CreateBr(condBB);

    Builder->SetInsertPoint(condBB);
    if (node.Cond) {
        node.Cond->accept(*this);
        Builder->CreateCondBr(toBool(LastValue), bodyBB, exitBB);
    } else {
        Builder->CreateBr(bodyBB);
    }

    Builder->SetInsertPoint(bodyBB);
    if (node.Then) node.Then->accept(*this);
    if (!Builder->GetInsertBlock()->getTerminator())
        Builder->CreateBr(afterBB);

    Builder->SetInsertPoint(afterBB);
    if (node.After) node.After->accept(*this);
    Builder->CreateBr(condBB);

    Builder->SetInsertPoint(exitBB);
    popScope();
    LoopExit = savedExit; LoopContinue = savedCont;
    LastValue = nullptr;
}

void CodeGenVisitor::visit(ImportSttmt&) {
    // TODO: Implement module imports
}

void CodeGenVisitor::visit(CastExpr& node) {
    node.Operand->accept(*this);
    llvm::Value* value = LastValue;
    if (!value) { LastValue = nullptr; return; }

    const Type& from = node.Operand->ResolvedType;
    const Type& to = node.Target;

    LastValue = to == Type::Bool() ? toBool(value) : coerceToType(value, from, to);
}

void CodeGenVisitor::visit(UnaryExpr& node) {
    if (!node.Operand) { LastValue = nullptr; return; }
    if (node.Op == "tenta") { emitTenta(node); return; }
    node.Operand->accept(*this);

    llvm::Value* v = LastValue;
    if (!v) { LastValue = nullptr; return; }

    if (node.Op == "!") {
        LastValue = Builder->CreateNot(toBool(v), "nottmp");
    } else if (node.Op == "~") {
        LastValue = Builder->CreateNot(v, "bitnottmp");
    } else { // "-"
        if (v->getType()->isFloatingPointTy())
            LastValue = Builder->CreateFNeg(v, "negtmp");
        else
            LastValue = Builder->CreateNeg(v, "negtmp");
    }
}
