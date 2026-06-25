#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/MemoryBuffer.h>

#include <llvm/Support/Program.h>
#include <llvm/Support/Path.h>

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
#include "../../include/kriol/type_utils.hh"

using namespace kriol::ast;

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

#define __WASI_MAIN "__main_argc_argv"


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
    throw std::runtime_error("The " + description + " could not be found in the system.");
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
        throw std::runtime_error("Failed to allocate temporary file: " + ec.message());
    }

    llvm::raw_fd_ostream out(tempPath, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open temporary file: " + ec.message());
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
        "WASI linker 'wasm-ld-20', 'wasm-ld-19', or 'wasm-ld'"
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
        throw std::runtime_error("The WASI linker fallback is not available.");
    runProgram(plan.LinkerPath, plan.Args, "Failure in the final WASI linkage: ");
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
        throw std::runtime_error("Failure in the embedded WASI linkage: " + detail);
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

static void removeWasiInputs(const WasiInputs& inputs) {
    if (!inputs.Crt1Command.empty()) llvm::sys::fs::remove(inputs.Crt1Command);
    if (!inputs.Libc.empty()) llvm::sys::fs::remove(inputs.Libc);
    if (!inputs.Libm.empty()) llvm::sys::fs::remove(inputs.Libm);
    if (!inputs.Builtins.empty()) llvm::sys::fs::remove(inputs.Builtins);
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
            throw std::runtime_error("This build of the compiler was built without the experimental 'wasm32-wasi' support.");
#endif
    }

    throw std::runtime_error("Unsupported codegen target.");
}

static void linkRuntimeBitcode(llvm::Module& module,
                               llvm::LLVMContext& context,
                               EmbeddedBlob runtimeBlob) {
    llvm::StringRef bcData(reinterpret_cast<const char*>(runtimeBlob.Data), runtimeBlob.Len);
    auto memBuf = llvm::MemoryBuffer::getMemBuffer(bcData, "kriol_runtime", false);

    auto expectedMod = llvm::parseBitcodeFile(*memBuf, context);
    if (!expectedMod) {
        std::string errDetail = llvm::toString(expectedMod.takeError());
        throw std::runtime_error("Failed to parse runtime bitcode: " + errDetail);
    }

    bool linkErr = llvm::Linker::linkModules(module, std::move(expectedMod.get()));
    if (linkErr)
        throw std::runtime_error("Failed to merge runtime bitcode into the main module.");
}

static void emitObjectFile(llvm::Module& module,
                           const std::string& targetTriple,
                           const std::string& objPath) {
    module.setTargetTriple(targetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);
    if (!Target)
        throw std::runtime_error("Failed to find Target: " + Error);

    auto CPU = "generic";
    auto Features = "";
    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(targetTriple, CPU, Features, opt, RM);

    module.setDataLayout(TargetMachine->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(objPath, EC, llvm::sys::fs::OF_None);
    if (EC)
        throw std::runtime_error("The object file could not be opened: " + EC.message());

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::ObjectFile;
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
        throw std::runtime_error("The TargetMachine cannot output a file of such type.");

    pass.run(module);
    dest.flush();
}

static std::string writeGcArchive(CodegenTarget target, EmbeddedBlob gcBlob) {
    if (!gcBlob.Data || gcBlob.Len == 0)
        return {};

    const char* stem = target == CodegenTarget::Wasm32Wasi
        ? "embedded_libgc_wasm32_wasi"
        : "embedded_libgc_native";
    return writeTempBlob(stem, "a", gcBlob);
}

static void linkNativeExecutable(const std::string& objPath,
                                 const std::string& tempLibGc,
                                 const std::string& outputPath) {
    std::string ccPath = findProgram({"clang", "cc"}, "linker 'clang' or 'cc'");
    std::vector<std::string> linkArgs = {
        ccPath,
        "-no-pie",
        objPath
    };
    if (!tempLibGc.empty())
        linkArgs.push_back(tempLibGc);
    linkArgs.push_back("-o");
    linkArgs.push_back(outputPath);
    linkArgs.push_back("-lm");
    runProgram(ccPath, linkArgs, "Failure in the final linkage: ");
}

static kriol::Type promotedNumericType(const kriol::Type& lhs, const kriol::Type& rhs) {
    if (!lhs.valid()) return rhs;
    if (!rhs.valid()) return lhs;
    if (lhs.isFloat() || rhs.isFloat()) {
        unsigned bits = std::max(lhs.isFloat() ? lhs.bitWidth() : 0,
                                 rhs.isFloat() ? rhs.bitWidth() : 0);
        return kriol::Type::Float(bits == 32 ? 32 : 64);
    }
    if (lhs.isInteger() && rhs.isInteger()) {
        unsigned bits = std::max(lhs.bitWidth(), rhs.bitWidth());
        bool isSigned = lhs.isSigned() || rhs.isSigned();
        if (lhs.isSigned() != rhs.isSigned())
            bits = std::max(bits, std::min(64u, bits * 2));
        return isSigned ? kriol::Type::SignedInteger(bits) : kriol::Type::UnsignedInteger(bits);
    }
    return lhs.valid() ? lhs : rhs;
}

static const LiteralExpr* integerLiteralExpr(const Expr* expr, bool& negative) {
    if (!expr) return nullptr;
    if (auto* par = dynamic_cast<const ParExpr*>(expr))
        return integerLiteralExpr(par->Content.get(), negative);
    if (auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        if (unary->Op != "-") return nullptr;
        negative = !negative;
        return integerLiteralExpr(unary->Operand.get(), negative);
    }
    auto* lit = dynamic_cast<const LiteralExpr*>(expr);
    return lit && lit->Type.isInteger() ? lit : nullptr;
}

static bool integerLiteralFitsType(const Expr* expr, const kriol::Type& to) {
    if (!to.isInteger()) return false;

    bool negative = false;
    auto* lit = integerLiteralExpr(expr, negative);
    if (!lit) return false;

    long long value = 0;
    try {
        value = std::stoll(lit->Value);
    } catch (...) {
        return false;
    }
    if (negative) value = -value;

    const unsigned bits = to.bitWidth();
    if (bits == 0 || bits > 64) return false;

    if (to.isSigned()) {
        if (bits == 64) return true;
        const long long min = -(1LL << (bits - 1));
        const long long max = (1LL << (bits - 1)) - 1;
        return value >= min && value <= max;
    }

    if (value < 0) return false;
    if (bits == 64) return true;
    const unsigned long long max = (1ULL << bits) - 1;
    return static_cast<unsigned long long>(value) <= max;
}

static kriol::Type promotedNumericTypeForExpr(const Expr* lhsExpr,
                                              const Expr* rhsExpr,
                                              const kriol::Type& lhs,
                                              const kriol::Type& rhs) {
    if (lhs.isInteger() && rhs.isInteger()) {
        bool ignored = false;
        const bool lhsLiteral = integerLiteralExpr(lhsExpr, ignored) != nullptr;
        ignored = false;
        const bool rhsLiteral = integerLiteralExpr(rhsExpr, ignored) != nullptr;

        if (lhsLiteral && !rhsLiteral && integerLiteralFitsType(lhsExpr, rhs))
            return rhs;
        if (rhsLiteral && !lhsLiteral && integerLiteralFitsType(rhsExpr, lhs))
            return lhs;
    }

    return promotedNumericType(lhs, rhs);
}

}

CodeGenVisitor::CodeGenVisitor(const std::string& moduleName)
    : Mod(std::make_unique<llvm::Module>(moduleName, Context)),
      Builder(std::make_unique<llvm::IRBuilder<>>(Context))
{
    initializeTargets();
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
        throw std::runtime_error("unknown molda type '" + name + "'");

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

llvm::Function* CodeGenVisitor::getOrDeclarePrintf() {
    if (auto* fn = Mod->getFunction("printf")) return fn;
    auto* ptrTy  = llvm::PointerType::getUnqual(Context);
    auto* ftype  = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(Context), {ptrTy}, /*isVarArg=*/true);
    return llvm::Function::Create(
        ftype, llvm::Function::ExternalLinkage, "printf", *Mod);
}

llvm::Value* CodeGenVisitor::coerce(llvm::Value* v, llvm::Type* targetTy) {
    llvm::Type* srcTy = v->getType();
    if (srcTy == targetTy) return v; // identity

    auto* doubleTy = llvm::Type::getDoubleTy(Context);
    auto* i64Ty    = llvm::Type::getInt64Ty(Context);
    auto* i1Ty     = llvm::Type::getInt1Ty(Context);

    // nter (i64) -> num (double)
    if (srcTy == i64Ty && targetTy == doubleTy)
        return Builder->CreateSIToFP(v, doubleTy, "conv");
    // bool (i1) -> nter (i64)
    if (srcTy == i1Ty && targetTy == i64Ty)
        return Builder->CreateZExt(v, i64Ty, "conv");
    // bool (i1) -> num (double)
    if (srcTy == i1Ty && targetTy == doubleTy)
        return Builder->CreateUIToFP(v, doubleTy, "conv");
    // num (double) -> nter (i64)
    if (srcTy == doubleTy && targetTy == i64Ty)
        return Builder->CreateFPToSI(v, i64Ty, "conv");
    // any integer widening (e.g. i1 -> i64 via general int path)
    if (srcTy->isIntegerTy() && targetTy->isIntegerTy())
        return Builder->CreateSExtOrTrunc(v, targetTy, "conv");

    throw std::runtime_error("Unsupported implicit type conversion");
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
        if (targetType.isSigned())
            return Builder->CreateFPToSI(v, targetTy, "conv");
        return Builder->CreateFPToUI(v, targetTy, "conv");
    }

    return coerce(v, targetTy);
}

llvm::Value* CodeGenVisitor::toBool(llvm::Value* v) {
    if (v->getType()->isPointerTy())
        throw std::runtime_error("Cannot use non truthy value as a condition");
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

void CodeGenVisitor::emitNative(const std::string& outputPath) {
    emit(outputPath, {});
}

std::vector<unsigned char> CodeGenVisitor::emitToMemory(const EmitOptions& options) {
    llvm::SmallString<128> outputPath;
    std::error_code ec = llvm::sys::fs::createTemporaryFile(
        options.Target == CodegenTarget::Wasm32Wasi ? "kriol_wasm_output" : "kriol_output",
        options.Target == CodegenTarget::Wasm32Wasi ? "wasm" : "out",
        outputPath
    );
    if (ec)
        throw std::runtime_error("Failed to allocate temporary output file: " + ec.message());

    std::string outputPathStr(outputPath.str());
    try {
        emit(outputPathStr, options);

        auto outputBuffer = llvm::MemoryBuffer::getFile(outputPathStr);
        if (!outputBuffer)
            throw std::runtime_error("Failed to read compiler output: " + outputBuffer.getError().message());

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
    std::string objPath = outputPath + ".o";
    TargetResources resources = selectTargetResources(options.Target);

    linkRuntimeBitcode(*Mod, Context, resources.Runtime);
    emitObjectFile(*Mod, resources.Triple, objPath);
    std::string tempLibGc = writeGcArchive(options.Target, resources.GcArchive);
#if KRIOL_ENABLE_WASM
    WasiInputs wasiInputs;
    if (options.Target == CodegenTarget::Wasm32Wasi)
        wasiInputs = writeWasiInputs();
#endif

    try {
        if (options.Target == CodegenTarget::Wasm32Wasi) {
#if KRIOL_ENABLE_WASM
            linkWasm(buildWasiLinkPlan(objPath, tempLibGc, wasiInputs, outputPath));
#else
            throw std::runtime_error("kriol was built without wasm32-wasi support.");
#endif
        } else {
            linkNativeExecutable(objPath, tempLibGc, outputPath);
        }
    } catch (...) {
        std::remove(objPath.c_str());
        if (!tempLibGc.empty()) llvm::sys::fs::remove(tempLibGc);
#if KRIOL_ENABLE_WASM
        if (options.Target == CodegenTarget::Wasm32Wasi)
            removeWasiInputs(wasiInputs);
#endif
        throw;
    }

    std::remove(objPath.c_str());
    if (!tempLibGc.empty()) llvm::sys::fs::remove(tempLibGc);
#if KRIOL_ENABLE_WASM
    if (options.Target == CodegenTarget::Wasm32Wasi)
        removeWasiInputs(wasiInputs);
#endif
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
            node.Name
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

    if (node.IsArray) {
        if (node.Value) {
            auto* arrTy = llvm::dyn_cast<llvm::ArrayType>(ty);
            if (!arrTy) throw std::runtime_error("internal error: array variable '" + node.Name + "' has non-array LLVM type");
            auto* elemTy = arrTy->getArrayElementType();

            if (auto* initLit = dynamic_cast<ArrayLiteralExpr*>(node.Value.get())) {
                for (size_t i = 0; i < initLit->Elements.size(); ++i) {
                    initLit->Elements[i]->accept(*this);
                    if (!LastValue) continue;
                    llvm::Value* value = LastValue;
                    Type elemKriolTy = node.Type.elementType();
                    if (value->getType() != elemTy)
                        value = coerceToType(value, initLit->Elements[i]->ResolvedType, elemKriolTy);
                    auto* index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), i);
                    llvm::Value* elemPtr = createArrayElementPtr(alloca, ty, index);
                    Builder->CreateStore(value, elemPtr);
                }
            } else if (auto* initRep = dynamic_cast<ArrayRepeatExpr*>(node.Value.get())) {
                initRep->Fill->accept(*this);
                llvm::Value* fillVal = LastValue
                    ? coerceToType(LastValue, initRep->Fill->ResolvedType, node.Type.elementType())
                    : llvm::Constant::getNullValue(elemTy);
                for (size_t i = 0; i < initRep->Count; ++i) {
                    auto* index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), i);
                    llvm::Value* elemPtr = createArrayElementPtr(alloca, ty, index);
                    Builder->CreateStore(fillVal, elemPtr);
                }
            } else {
                throw std::runtime_error("array variable '" + node.Name + "' requires array literal or repeat initializer");
            }
        }
    } else if (node.Value) {
        node.Value->accept(*this);
        if (LastValue) {
            LastValue = coerceToType(LastValue, node.Value->ResolvedType, node.Type);
            Builder->CreateStore(LastValue, alloca);
        }
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

llvm::Value* CodeGenVisitor::getArrayStorage(const std::string& name) {
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

CodeGenVisitor::LValue CodeGenVisitor::resolveLValue(ast::Expr* expr) {
    if (!expr) return {};

    if (auto* par = dynamic_cast<ParExpr*>(expr))
        return resolveLValue(par->Content.get());

    if (auto* ident = dynamic_cast<IdentExpr*>(expr)) {
        if (auto* alloca = lookupVar(ident->Name))
            return {alloca, alloca->getAllocatedType()};
        if (auto* gv = lookupGlobal(ident->Name))
            return {gv, gv->getValueType()};
        return {};
    }

    if (auto* arr = dynamic_cast<ArrayAccessExpr*>(expr)) {
        LValue base = resolveLValue(arr->Base.get());
        if (!base.Ptr || !base.Type || !base.Type->isArrayTy()) return {};

        if (arr->Index) arr->Index->accept(*this);
        if (!LastValue) return {};

        auto* i64Ty = llvm::Type::getInt64Ty(Context);
        llvm::Value* idx = LastValue->getType() == i64Ty ? LastValue : coerce(LastValue, i64Ty);

        auto* sizeConst = llvm::ConstantInt::get(i64Ty, base.Type->getArrayNumElements());
        auto* lineConst = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), arr->LineNum);
        Builder->CreateCall(getOrDeclareKriolCheckBounds(), {idx, sizeConst, lineConst});

        llvm::Value* elemPtr = createArrayElementPtr(base.Ptr, base.Type, idx);
        return {elemPtr, base.Type->getArrayElementType()};
    }

    if (auto* member = dynamic_cast<MemberAccessExpr*>(expr)) {
        if (!member->Base || !member->Base->ResolvedType.isNamed()) return {};

        LValue base = resolveLValue(member->Base.get());
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

    return {};
}

void CodeGenVisitor::visit(ArrayAccessExpr& node) {
    LValue elem = resolveLValue(&node);
    if (!elem.Ptr || !elem.Type) { LastValue = nullptr; return; }
    LastValue = Builder->CreateLoad(elem.Type, elem.Ptr, "array.elem");
}

void CodeGenVisitor::visit(MemberAccessExpr& node) {
    if (!node.Base || !node.Base->ResolvedType.isNamed()) {
        LastValue = nullptr;
        return;
    }

    auto recordIt = Records.find(node.Base->ResolvedType.name());
    if (recordIt == Records.end()) {
        LastValue = nullptr;
        return;
    }

    auto fieldIt = recordIt->second.fieldIndex.find(node.Member);
    if (fieldIt == recordIt->second.fieldIndex.end()) {
        LastValue = nullptr;
        return;
    }

    std::size_t fieldIndex = fieldIt->second;

    LValue field = resolveLValue(&node);
    if (field.Ptr && field.Type) {
        LastValue = Builder->CreateLoad(field.Type, field.Ptr, "field." + node.Member);
        return;
    }

    node.Base->accept(*this);
    if (!LastValue) return;
    LastValue = Builder->CreateExtractValue(
        LastValue,
        {static_cast<unsigned>(fieldIndex)},
        "field." + node.Member
    );
}

void CodeGenVisitor::visit(QualifiedAccessExpr& node) {
    LastValue = nullptr;
}

void CodeGenVisitor::visit(ArrayLiteralExpr& node) {
    if (!node.ResolvedType.isArray()) {
        LastValue = nullptr;
        return;
    }

    auto* arrayTy = llvm::dyn_cast<llvm::ArrayType>(mapType(node.ResolvedType));
    if (!arrayTy) {
        LastValue = nullptr;
        return;
    }

    llvm::Value* array = llvm::UndefValue::get(arrayTy);
    llvm::Type* elemTy = arrayTy->getArrayElementType();
    for (std::size_t i = 0; i < node.Elements.size(); ++i) {
        node.Elements[i]->accept(*this);
        llvm::Value* elem = LastValue;
        if (!elem) elem = llvm::Constant::getNullValue(elemTy);
        Type elemKriolTy = node.ResolvedType.elementType();
        if (elem->getType() != elemTy)
            elem = coerceToType(elem, node.Elements[i]->ResolvedType, elemKriolTy);
        array = Builder->CreateInsertValue(
            array,
            elem,
            {static_cast<unsigned>(i)},
            "array.literal.elem"
        );
    }

    LastValue = array;
}

void CodeGenVisitor::visit(ArrayRepeatExpr& node) {
    if (!node.ResolvedType.isArray()) {
        LastValue = nullptr;
        return;
    }

    auto* arrayTy = llvm::dyn_cast<llvm::ArrayType>(mapType(node.ResolvedType));
    if (!arrayTy) {
        LastValue = nullptr;
        return;
    }

    llvm::Type* elemTy = arrayTy->getArrayElementType();
    if (node.Fill) node.Fill->accept(*this);
    llvm::Value* fill = LastValue ? LastValue : llvm::Constant::getNullValue(elemTy);
    if (fill->getType() != elemTy)
        fill = coerceToType(fill, node.Fill->ResolvedType, node.ResolvedType.elementType());

    llvm::Value* array = llvm::UndefValue::get(arrayTy);
    for (uint64_t i = 0; i < arrayTy->getNumElements(); ++i) {
        array = Builder->CreateInsertValue(
            array,
            fill,
            {static_cast<unsigned>(i)},
            "array.repeat.elem"
        );
    }

    LastValue = array;
}

void CodeGenVisitor::visit(RecordLiteralExpr& node) {
    auto recordIt = Records.find(node.TypeName);
    if (recordIt == Records.end()) {
        LastValue = nullptr;
        return;
    }

    llvm::StructType* structTy = getOrCreateRecordType(node.TypeName);
    llvm::Value* record = llvm::UndefValue::get(structTy);
    std::vector<bool> initialized(recordIt->second.fields.size(), false);

    for (auto& field : node.Fields) {
        auto fieldIt = recordIt->second.fieldIndex.find(field.Name);
        if (fieldIt == recordIt->second.fieldIndex.end()) continue;

        std::size_t index = fieldIt->second;
        if (field.Value) field.Value->accept(*this);
        llvm::Value* value = LastValue;
        llvm::Type* fieldTy = structTy->getElementType(static_cast<unsigned>(index));
        if (!value) value = llvm::Constant::getNullValue(fieldTy);
        if (value->getType() != fieldTy)
            value = coerceToType(value, field.Value->ResolvedType, recordIt->second.fields[index]->Type);
        record = Builder->CreateInsertValue(
            record,
            value,
            {static_cast<unsigned>(index)},
            "record.field"
        );
        initialized[index] = true;
    }

    for (std::size_t i = 0; i < initialized.size(); ++i) {
        if (initialized[i]) continue;
        llvm::Type* fieldTy = structTy->getElementType(static_cast<unsigned>(i));
        record = Builder->CreateInsertValue(
            record,
            llvm::Constant::getNullValue(fieldTy),
            {static_cast<unsigned>(i)},
            "record.field.default"
        );
    }

    LastValue = record;
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

void CodeGenVisitor::forwardDeclareFunc(ast::FuncDeclSttmt& node) {
    bool isMain = (node.Name == "inisiu");
    std::string name = isMain ? "main" : node.Name;

    FuncSig sig;
    sig.retType = isMain ? Type::SignedInteger(32) : node.Type;
    if (node.Args) {
        for (auto& arg : node.Args->Args)
            sig.paramTypes.push_back(arg->Type);
    }
    FunctionSigs[name] = std::move(sig);

    // already declared
    if (Mod->getFunction(name)) return;

    std::vector<llvm::Type*> paramTypes;
    if (node.Args) {
        for (auto& arg : node.Args->Args)
            paramTypes.push_back(mapType(arg->Type));
    }

    llvm::Type* retTy = isMain
        ? llvm::Type::getInt32Ty(Context)
        : mapType(node.Type);

    auto* ftype = llvm::FunctionType::get(retTy, paramTypes, false);
    llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, name, *Mod);
}

void CodeGenVisitor::visit(BlockSttmt& node) {
    // At program root forward-declare all user functions
    // so that forward calls and mutual recursion resolve in codegen.
    if (!CurrentFunction) {
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
        if (!CurrentFunction && dynamic_cast<MoldaDeclSttmt*>(s.get())) continue;
        if (!CurrentFunction && dynamic_cast<VarDeclSttmt*>(s.get())) continue;
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
    std::string name = isMain ? "main" : node.Name;

    FuncSig sig;
    sig.retType = isMain ? Type::SignedInteger(32) : node.Type;
    if (node.Args) {
        for (auto& arg : node.Args->Args)
            sig.paramTypes.push_back(arg->Type);
    }
    FunctionSigs[name] = sig;

    std::vector<llvm::Type*> paramTypes;
    if (node.Args) {
        for (auto& arg : node.Args->Args)
            paramTypes.push_back(mapType(arg->Type));
    }

    llvm::Type* retTy = isMain
        ? llvm::Type::getInt32Ty(Context)
        : mapType(node.Type);

    // Get the function if it was already forward-declared, otherwise create it now.
    auto* fn = Mod->getFunction(name);

    if (!fn) {
        auto* ftype = llvm::FunctionType::get(retTy, paramTypes, false);
        fn = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, name, *Mod);
    }

    if (node.Args) {
        size_t i = 0;
        for (auto& llvmArg : fn->args())
            llvmArg.setName(node.Args->Args[i++]->Name);
    }

    auto* entry = llvm::BasicBlock::Create(Context, "entry", fn);
    Builder->SetInsertPoint(entry);
    CurrentFunction = fn;
    CurrentReturnType = sig.retType;

    pushScope();
    size_t nodeArgCount = (node.Args ? node.Args->Args.size() : 0);
    if (nodeArgCount > 0) {
        size_t i = 0;
        for (auto& llvmArg : fn->args()) {
            auto& p = node.Args->Args[i++];
            auto* a = createEntryAlloca(fn, p->Name, llvmArg.getType());
            Builder->CreateStore(&llvmArg, a);
            declareVar(p->Name, a);
        }
    }

    // Initialise the Boehm GC as the very first thing in main.
    if (isMain) {
        auto* gcInit = getOrDeclareKriolGcInit(*Mod, Context);
        Builder->CreateCall(gcInit, {});
    }

    // Emit deferred global initializers at the top of inisiu (main)
    if (isMain && !DeferredGlobalInits.empty()) {
        for (auto& di : DeferredGlobalInits) {
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
        if (isMain)
            Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0));
        else if (retTy->isVoidTy())
            Builder->CreateRetVoid();
        else {
            // If every branch already returned, the current block is dead code
            // with no predecessors (but it still needs a valid LLVM terminator).
            llvm::BasicBlock* cur = Builder->GetInsertBlock();
            bool isDeadBlock = (cur != &fn->getEntryBlock()) && cur->hasNPredecessors(0);
            if (isDeadBlock)
                Builder->CreateUnreachable();
            else
                throw std::runtime_error("Non-void function '" + node.Name + "' has no return statement");
        }
    }

    auto verificationFailed = llvm::verifyFunction(*fn);

    if (verificationFailed) {
        fn->print(llvm::errs());
        throw std::runtime_error("Function verification failed for '" + node.Name + "'");
    }

    if (isMain && CurrentTarget == CodegenTarget::Wasm32Wasi) {
        auto* wrapper = emitWasiMainWrapper(*Mod, Context, *Builder, fn);
        if (llvm::verifyFunction(*wrapper)) {
            wrapper->print(llvm::errs());
            throw std::runtime_error("WASI main wrapper verification failed");
        }
    }

    CurrentFunction = nullptr;
    CurrentReturnType = Type::Invalid();
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
    auto* elseBB  = llvm::BasicBlock::Create(Context, "else");
    auto* mergeBB = llvm::BasicBlock::Create(Context, "ifcont");

    Builder->CreateCondBr(cond, thenBB, node.Else ? elseBB : mergeBB);

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
    if (node.Name == "break" && LoopExit)
        Builder->CreateBr(LoopExit);
    else if (node.Name == "continue" && LoopContinue)
        Builder->CreateBr(LoopContinue);
}

void CodeGenVisitor::visit(ReturnSttmt& node) {
    if (node.ReturnValue) {
        node.ReturnValue->accept(*this);
        // Coerce to the function's declared return type (e.g. nter -> num widening)
        llvm::Type* retTy = Builder->GetInsertBlock()->getParent()->getReturnType();
        if (LastValue && LastValue->getType() != retTy)
            LastValue = coerceToType(LastValue, node.ReturnValue->ResolvedType, CurrentReturnType);
        Builder->CreateRet(LastValue);
    } else {
        Builder->CreateRetVoid();
    }
    LastValue = nullptr;
}

void CodeGenVisitor::visit(FuncCallArgs& node) {
    // Handled inside FunCallExpr
}

void CodeGenVisitor::visit(FunCallExpr& node) {
    auto* callee = unwrapIdentExpr(node.Callee.get());
    if (!callee) { LastValue = nullptr; return; }
    if (emitPreludeCall(node, callee->Name)) return;

    auto* fn = Mod->getFunction(callee->Name);
    if (!fn) { LastValue = nullptr; return; }
    auto sigIt = FunctionSigs.find(callee->Name);

    std::vector<llvm::Value*> callArgs;
    if (node.Args) {
        size_t i = 0;
        for (auto& arg : node.Args->Args) {
            arg->accept(*this);
            if (LastValue) {
                if (i < fn->arg_size()) {
                    // Coerce argument to the declared parameter type when they differ
                    llvm::Type* paramTy = (fn->arg_begin() + i)->getType();
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
    }

    LastValue = Builder->CreateCall(fn, callArgs);
}

void CodeGenVisitor::visit(BinExpr& node) {
    node.LHS->accept(*this);
    llvm::Value* lhs = LastValue;
    node.RHS->accept(*this);
    llvm::Value* rhs = LastValue;
    if (!lhs || !rhs) { LastValue = nullptr; return; }

    const auto& op = node.Op;
    if (op == "&&") { LastValue = Builder->CreateAnd(toBool(lhs), toBool(rhs)); return; }
    if (op == "||") { LastValue = Builder->CreateOr(toBool(lhs), toBool(rhs)); return; }

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

    if      (op == "+")  LastValue = isFloat ? Builder->CreateFAdd(lhs, rhs) : Builder->CreateAdd(lhs, rhs);
    else if (op == "-")  LastValue = isFloat ? Builder->CreateFSub(lhs, rhs) : Builder->CreateSub(lhs, rhs);
    else if (op == "*")  LastValue = isFloat ? Builder->CreateFMul(lhs, rhs) : Builder->CreateMul(lhs, rhs);
    else if (op == "/")  LastValue = isFloat ? Builder->CreateFDiv(lhs, rhs) : (isUnsigned ? Builder->CreateUDiv(lhs, rhs) : Builder->CreateSDiv(lhs, rhs));
    else if (op == "%")  LastValue = isFloat ? Builder->CreateFRem(lhs, rhs) : (isUnsigned ? Builder->CreateURem(lhs, rhs) : Builder->CreateSRem(lhs, rhs));
    else if (op == "<")  LastValue = isFloat ? Builder->CreateFCmpOLT(lhs, rhs) : (isUnsigned ? Builder->CreateICmpULT(lhs, rhs) : Builder->CreateICmpSLT(lhs, rhs));
    else if (op == ">")  LastValue = isFloat ? Builder->CreateFCmpOGT(lhs, rhs) : (isUnsigned ? Builder->CreateICmpUGT(lhs, rhs) : Builder->CreateICmpSGT(lhs, rhs));
    else if (op == "<=") LastValue = isFloat ? Builder->CreateFCmpOLE(lhs, rhs) : (isUnsigned ? Builder->CreateICmpULE(lhs, rhs) : Builder->CreateICmpSLE(lhs, rhs));
    else if (op == ">=") LastValue = isFloat ? Builder->CreateFCmpOGE(lhs, rhs) : (isUnsigned ? Builder->CreateICmpUGE(lhs, rhs) : Builder->CreateICmpSGE(lhs, rhs));
    else if (op == "==") LastValue = isFloat ? Builder->CreateFCmpOEQ(lhs, rhs) : Builder->CreateICmpEQ(lhs, rhs);
    else if (op == "!=") LastValue = isFloat ? Builder->CreateFCmpONE(lhs, rhs) : Builder->CreateICmpNE(lhs, rhs);
    else LastValue = nullptr;
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
    if (node.Assigned) node.Assigned->accept(*this);
    llvm::Value* val = LastValue;
    if (!val) { LastValue = nullptr; return; }

    LValue dest = resolveLValue(node.Assignee.get());
    if (!dest.Ptr || !dest.Type) { LastValue = nullptr; return; }

    if (node.AssignOp != "=") {
        llvm::Value* cur = Builder->CreateLoad(dest.Type, dest.Ptr);
        Type targetType = node.Assignee->ResolvedType;
        llvm::Value* rhs = coerceToType(val, node.Assigned->ResolvedType, targetType);
        bool isFloat = targetType.isFloat();
        bool isUnsigned = targetType.isUnsignedInteger();
        if      (node.AssignOp == "+=") val = isFloat ? Builder->CreateFAdd(cur, rhs) : Builder->CreateAdd(cur, rhs);
        else if (node.AssignOp == "-=") val = isFloat ? Builder->CreateFSub(cur, rhs) : Builder->CreateSub(cur, rhs);
        else if (node.AssignOp == "*=") val = isFloat ? Builder->CreateFMul(cur, rhs) : Builder->CreateMul(cur, rhs);
        else if (node.AssignOp == "/=") val = isFloat ? Builder->CreateFDiv(cur, rhs) : (isUnsigned ? Builder->CreateUDiv(cur, rhs) : Builder->CreateSDiv(cur, rhs));
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

void CodeGenVisitor::visit(UnaryExpr& node) {
    if (!node.Operand) { LastValue = nullptr; return; }
    node.Operand->accept(*this);

    llvm::Value* v = LastValue;
    if (!v) { LastValue = nullptr; return; }

    if (node.Op == "!") {
        LastValue = Builder->CreateNot(toBool(v), "nottmp");
    } else { // "-"
        if (v->getType()->isFloatingPointTy())
            LastValue = Builder->CreateFNeg(v, "negtmp");
        else
            LastValue = Builder->CreateNeg(v, "negtmp");
    }
}
