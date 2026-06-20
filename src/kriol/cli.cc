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
struct yy_buffer_state;
typedef yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);

static std::string g_source_file;

void cli::SetSourceFile(const std::string& filename) {
    g_source_file = filename;
}

const std::string& cli::GetSourceFile() {
    return g_source_file;
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

void cli::PrintErr(const std::string& file, int line, const std::string& msg, int exitNum) {
    std::string location = file.empty() ? "" : file + ":" + std::to_string(line) + ": ";
    std::cerr << KR_STANDARD_COMPILER_NAME << ": err: " << location << msg << std::endl;
    if (exitNum >= 0) throw cli::FatalError(location + msg, exitNum);
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
        "  Native builds write ./a.out by default.\n"
        "  wasm32-wasi builds write ./a.wasm by default.\n"
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

    Parser->add_argument("--target")
        .help("Compilation target.")
        .metavar("TARGET")
        .default_value(std::string("native"))
        .nargs(1)
        .choices("native", "wasm32-wasi");

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

    try
    {
        Parser->parse_args(argc, argv);
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
    std::ofstream file(filename, std::ios::binary);
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
    if (!fs::exists(filename))
    {
        cli::PrintErr("File '" + filename + "' was not found!", 1);
    }

    if (!fs::is_regular_file(filename))
    {
        cli::PrintErr("Input '" + filename + "' is not a regular file.", 1);
    }

    FILE *file = fopen(filename.c_str(), "r");

    if (file == NULL)
    {
        cli::PrintErr("Couldn't open the file '" + filename + "': " + std::strerror(errno), 1);
    }

    yyin = file;
    cli::SetSourceFile(filename);
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
    if (options.target != "native" && options.target != "wasm32-wasi")
        throw std::invalid_argument("Unknown compilation target '" + options.target + "'.");
    if (options.emitIR && options.outputToMemory)
        throw std::invalid_argument("emitIR and outputToMemory cannot be used together.");
    if (options.outputToMemory && !options.outfile.empty())
        throw std::invalid_argument("outfile cannot be used with outputToMemory.");
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

    ast::CodegenTarget Target = options.target == "wasm32-wasi"
        ? ast::CodegenTarget::Wasm32Wasi
        : ast::CodegenTarget::Native;

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

    std::string defaultOutfile = Target == ast::CodegenTarget::Wasm32Wasi
        ? KR_DEFAULT_WASM_OUT_FILE
        : KR_DEFAULT_OUT_FILE;

    std::string outfile = options.outfile != "" ? options.outfile : defaultOutfile;

    ast::EmitOptions emitOptions = {.Target = Target};

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

    const std::string extension = fs::path(Args.input).extension().string();
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
