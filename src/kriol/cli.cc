#include "../../include/kriol/cli.hh"
#include "../../include/kriol/ast.hh"
#include "../../include/kriol/codegen.hh"
#include "../../include/kriol/sema.hh"
#include "../../include/kriol/constants.hh"

#include "../../include/external/argparse.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>

#include <llvm/Support/ManagedStatic.h>

namespace fs = std::filesystem;
namespace ap = argparse;

using namespace kriol;

extern FILE *yyin;

extern int yyparse(kriol::ast::BlockSttmt** Program);
extern int yylex_destroy(void);
extern int yylineno;
extern void kriol_scanner_reset_state(void);
extern void yyrestart(FILE* input_file);
struct yy_buffer_state;
typedef yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);

static std::string g_source_file;
static std::vector<std::string> g_parse_errors;

// Paths arrive as UTF-8 (main.cc converts the Windows command line), but
// std::filesystem and the CRT read narrow strings in the ANSI code page there.
static fs::path PathFromUtf8(const std::string& path)
{
    return fs::u8path(path);
}

static bool IsKnownTarget(const std::string& target)
{
    return target == "native" || target == "wasm32-wasi" || target == "x86_64-windows";
}

static ast::CodegenTarget ResolveTarget(const std::string& target)
{
    if (target == "wasm32-wasi")
        return ast::CodegenTarget::Wasm32Wasi;
    if (target == "x86_64-windows")
        return ast::CodegenTarget::X86_64Windows;
#if defined(_WIN32) && KRIOL_ENABLE_WINDOWS_TARGET
    // The bundled MinGW runtime needs no Visual Studio install, unlike the
    // host's default MSVC target.
    return ast::CodegenTarget::X86_64Windows;
#else
    return ast::CodegenTarget::Native;
#endif
}

static FILE* OpenFileForRead(const std::string& filename)
{
#ifdef _WIN32
    return _wfopen(PathFromUtf8(filename).c_str(), L"rb");
#else
    return fopen(filename.c_str(), "rb");
#endif
}

void cli::SetSourceFile(const std::string& filename) {
    g_source_file = filename;
}

const std::string& cli::GetSourceFile() {
    return g_source_file;
}

void cli::ReportParseError(int line, const std::string& message) {
    std::string location = g_source_file.empty() ? "" : g_source_file + ":" + std::to_string(line) + ": ";
    g_parse_errors.push_back(location + message);
}

void cli::PrintErr(std::string message)
{
    std::cerr << KR_STANDARD_COMPILER_NAME << ": err: " << message << std::endl;
}

void cli::PrintErr(std::string message, int exitNum)
{
    cli::PrintErr(message);
    throw cli::FatalError(message, exitNum);
}


void cli::Compiler::DefineArgs()
{
    Parser->add_description(
        "Compile one Kriol source file or inline source text."
    );
    Parser->add_epilog(
        "Inputs:\n"
        "  Provide exactly one of [file] or --text SOURCE.\n\n"
        "Outputs:\n"
        "  Native builds write ./" KR_DEFAULT_OUT_FILE " by default.\n"
        "  wasm32-wasi builds write ./" KR_DEFAULT_WASM_OUT_FILE " by default.\n"
        "  x86_64-windows builds write ./" KR_DEFAULT_WINDOWS_OUT_FILE " by default.\n"
        "  --emit-ir prints LLVM IR to stdout unless -o is provided.\n\n"
        "Examples:\n"
        "  kriol hello.kriol\n"
        "  kriol hello.kriol -o hello\n"
        "  kriol hello.kriol --target wasm32-wasi -o hello.wasm\n"
        "  kriol --text 'fn inisiu() { mostran(\"Oi\"); }' --emit-ir"
    );
    Parser->set_usage_max_line_width(100);

    Parser->add_argument("file")
        .help("Source file to compile (.kriol or .kr unless --ignore-extension is set).")
        .metavar("[file]")
        .nargs(ap::nargs_pattern::optional);

    Parser->add_argument("--text")
        .help("Compile inline source text instead of reading a file.")
        .metavar("SOURCE")
        .nargs(1);

    Parser->add_argument("-o", "--output")
        .help("Output path for the executable, wasm module, or emitted IR.")
        .metavar("FILE")
        .nargs(1);

    Parser->add_argument("--emit-ir")
        .help("Emit LLVM IR instead of producing native/wasm output.")
        .default_value(false)
        .implicit_value(true);

    // Only the targets this build supports are offered.
    auto& target = Parser->add_argument("--target")
        .help("Compilation target.")
        .metavar("TARGET")
        .default_value(std::string("native"))
        .nargs(1);
    target.add_choice("native");
#if KRIOL_ENABLE_WASM
    target.add_choice("wasm32-wasi");
#endif
#if KRIOL_ENABLE_WINDOWS_TARGET
    target.add_choice("x86_64-windows");
#endif

    Parser->add_argument("-O", "--opt-level")
        .help("Optimization level for the generated program, 0 (none) to 3.")
        .metavar("LEVEL")
        .default_value(std::string("2"))
        .nargs(1)
        .choices("0", "1", "2", "3");

    Parser->add_argument("--ignore-extension")
        .help("Accept file inputs without a ." +
              std::string(KR_STANDARD_FILE_EXTENSION) + " or ." +
              std::string(KR_ALTERNATIVE_FILE_EXTENSION) + " extension.")
        .default_value(false)
        .implicit_value(true);
}

void cli::Compiler::ParseArgs(int argc, const char* const* argv)
{
    DefineArgs();

    // Accept the attached "-O2" spelling compilers use; argparse only
    // understands "-O 2".
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (i > 0 && argument.size() == 3 && argument.compare(0, 2, "-O") == 0)
        {
            arguments.push_back("-O");
            arguments.push_back(argument.substr(2));
        }
        else
        {
            arguments.push_back(argument);
        }
    }

    try
    {
        Parser->parse_args(arguments);
    }
    catch (const std::exception &e)
    {
        std::stringstream sstrm;

        sstrm << *(Parser);

        cli::PrintErr(std::string(e.what()) + "\n\n" + sstrm.str(), 1);
    }

    auto file = Parser->present<std::string>("file");
    auto text = Parser->present<std::string>("--text");
    if (file.has_value() == text.has_value())
        cli::PrintErr("Provide exactly one input: a source file or --text <source>.", 1);

    if (text)
    {
        Args.inputKind = CompileInputKind::SourceText;
        Args.input = std::move(*text);
        Args.sourceName = "<command-line>";
    }
    else
    {
        Args.inputKind = CompileInputKind::File;
        Args.input = std::move(*file);
        Args.sourceName = Args.input;
    }

    Args.outfile = Parser->present<std::string>("--output").value_or("");
    Args.target = Parser->get<std::string>("--target");
    Args.optLevel = static_cast<unsigned>(std::stoul(Parser->get<std::string>("--opt-level")));
    Args.emitIR = Parser->get<bool>("--emit-ir");
    Args.ignoreExtension = Parser->get<bool>("--ignore-extension");
}

ast::BlockSttmt* cli::KriolLangParserWrapper::ParseCode(
    const std::string& input,
    CompileInputKind inputKind
)
{
    ast::BlockSttmt* program = nullptr;
    if (inputKind == CompileInputKind::File)
        ParseFile(input, &program);
    else
        ParseText(input, &program);
    return program;
}

void cli::Compiler::SaveCodeToFile(const std::string& code, const std::string& filename)
{
    std::ofstream file(PathFromUtf8(filename), std::ios::binary);
    if (!file)
        cli::PrintErr("Couldn't create file '" + filename + "': " + std::strerror(errno), 1);

    file.write(code.data(), static_cast<std::streamsize>(code.size()));
    if (!file)
        cli::PrintErr("Couldn't write file '" + filename + "'.", 1);
}

void cli::KriolLangParserWrapper::ParseFile(
    const std::string& filename,
    ast::BlockSttmt** program
)
{
    const fs::path path = PathFromUtf8(filename);

    if (!fs::exists(path))
    {
        cli::PrintErr("File '" + filename + "' was not found!", 1);
    }

    if (!fs::is_regular_file(path))
    {
        cli::PrintErr("Input '" + filename + "' is not a regular file.", 1);
    }

    FILE *file = OpenFileForRead(filename);

    if (file == NULL)
    {
        cli::PrintErr("Couldn't open the file '" + filename + "': " + std::strerror(errno), 1);
    }

    // Skip a UTF-8 byte order mark, which Windows editors often write.
    unsigned char bom[3];
    if (fread(bom, 1, sizeof bom, file) != sizeof bom ||
        bom[0] != 0xEF || bom[1] != 0xBB || bom[2] != 0xBF)
        rewind(file);

    // A previous parse may have stopped before the end of its input (an aborted
    // parse or an exception), leaving buffered input and start conditions behind.
    yyin = file;
    yyrestart(file);
    kriol_scanner_reset_state();
    cli::SetSourceFile(filename);
    g_parse_errors.clear();
    yylineno = 1;
    try {
        yyparse(program);
    } catch (...) {
        fclose(file);
        throw;
    }
    fclose(file);
}

void cli::KriolLangParserWrapper::ParseText(
    const std::string& text,
    ast::BlockSttmt** program
)
{
    kriol_scanner_reset_state();
    YY_BUFFER_STATE buffer = yy_scan_string(text.c_str());
    if (!buffer)
        cli::PrintErr("Couldn't create scanner buffer for source text!", 1);

    g_parse_errors.clear();
    yylineno = 1;
    try {
        yyparse(program);
    } catch (...) {
        yy_delete_buffer(buffer);
        kriol_scanner_reset_state();
        throw;
    }
    yy_delete_buffer(buffer);
    kriol_scanner_reset_state();
}

cli::CompileResult cli::Compile(const cli::CompileOptions& options)
{
    if (options.input.empty())
        throw std::invalid_argument("Compilation input cannot be empty.");
    if (!IsKnownTarget(options.target))
        throw std::invalid_argument("Unknown compilation target '" + options.target + "'.");
    if (options.emitIR && options.outputToMemory)
        throw std::invalid_argument("emitIR and outputToMemory cannot be used together.");
    if (options.outputToMemory && !options.outfile.empty())
        throw std::invalid_argument("outfile cannot be used with outputToMemory.");
    if (options.optLevel > 3)
        throw std::invalid_argument("Optimization level must be between 0 and 3.");
    if (options.outputToMemory && options.target != "wasm32-wasi")
        throw std::invalid_argument("In-memory output is currently supported only for wasm32-wasi.");

    CompileResult result;
    std::string sourceName = options.sourceName.empty()
        ? (options.inputKind == CompileInputKind::File ? options.input : "<memory>")
        : options.sourceName;
    cli::SetSourceFile(sourceName);

    ast::BlockSttmt *ProgramAST = KriolLangParserWrapper::ParseCode(
        options.input,
        options.inputKind
    );
    std::unique_ptr<ast::BlockSttmt> ProgramNode(ProgramAST);

    if (!g_parse_errors.empty())
    {
        result.diagnostics = std::move(g_parse_errors);
        g_parse_errors.clear();
        return result;
    }

    if (ProgramNode)
    {
        kriol::sema::SemanticAnalyzer sema;
        sema.SetSourceFile(sourceName);
        sema.Check(ProgramNode.get());
        if (sema.HasErrors())
        {
            result.diagnostics = sema.GetErrors();
            return result;
        }
    }

    ast::CodegenTarget Target = ResolveTarget(options.target);

    ast::CodeGenVisitor codegenVisitor(sourceName);
    codegenVisitor.CurrentTarget = Target;
    if (ProgramNode) ProgramNode->accept(codegenVisitor);

    if (options.emitIR)
    {
        result.ir = codegenVisitor.emitIR();
        if (options.outfile != "")
        {
            Compiler::SaveCodeToFile(result.ir, options.outfile);
            result.outputPath = options.outfile;
        }
        return result;
    }

    bool windowsExecutable = Target == ast::CodegenTarget::X86_64Windows;
#ifdef _WIN32
    windowsExecutable = windowsExecutable || Target == ast::CodegenTarget::Native;
#endif

    std::string defaultOutfile = Target == ast::CodegenTarget::Wasm32Wasi
        ? KR_DEFAULT_WASM_OUT_FILE
        : windowsExecutable ? KR_DEFAULT_WINDOWS_OUT_FILE : KR_DEFAULT_OUT_FILE;

    std::string outfile = options.outfile != "" ? options.outfile : defaultOutfile;
    // Windows only runs programs that carry an .exe extension.
    if (windowsExecutable && !PathFromUtf8(outfile).has_extension())
        outfile += ".exe";

    ast::EmitOptions emitOptions = {.Target = Target, .OptLevel = options.optLevel};

    if (options.outputToMemory)
    {
        result.outputBytes = codegenVisitor.emitToMemory(emitOptions);
    }
    else
    {
        codegenVisitor.emit(outfile, emitOptions);
        result.outputPath = outfile;
    }
    return result;
}

void cli::Compiler::ValidateInput() const
{
    if (Args.inputKind != CompileInputKind::File || Args.ignoreExtension)
        return;

    const std::string extension = PathFromUtf8(Args.input).extension().u8string();
    const bool supported = extension == "." + std::string(KR_STANDARD_FILE_EXTENSION) ||
                           extension == "." + std::string(KR_ALTERNATIVE_FILE_EXTENSION);
    if (!supported)
        cli::PrintErr(
            "File format not recognized. Expected a ." +
            std::string(KR_STANDARD_FILE_EXTENSION) + " or ." +
            std::string(KR_ALTERNATIVE_FILE_EXTENSION) + " source file.",
            1
        );
}

cli::CompileOptions cli::Compiler::MakeCompileOptions() const
{
    CompileOptions options;
    options.inputKind = Args.inputKind;
    options.input = Args.input;
    options.sourceName = Args.sourceName;
    options.outfile = Args.outfile;
    options.target = Args.target;
    options.optLevel = Args.optLevel;
    options.emitIR = Args.emitIR;
    return options;
}

void cli::Compiler::Run(int argc, const char* const* argv)
{
    ParseArgs(argc, argv);
    ValidateInput();

    try
    {
        CompileResult result = Compile(MakeCompileOptions());
        if (!result.diagnostics.empty())
        {
            for (const auto& err : result.diagnostics)
                cli::PrintErr(err);
            throw cli::FatalError("semantic errors", 1);
        }

        if (Args.emitIR && Args.outfile.empty())
            std::cout << result.ir;
    }
    catch (std::exception &err)
    {
        // Re-throw FatalError so main() catches it and returns the right exit code
        if (dynamic_cast<cli::FatalError*>(&err))
            throw;
        cli::PrintErr(err.what(), 1);
    }
}

void cli::Compiler::Cleanup()
{
    yylex_destroy();
    llvm::llvm_shutdown();
}
