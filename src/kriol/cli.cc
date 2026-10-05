#include "../../include/kriol/cli.hh"
#include "../../include/kriol/ast.hh"
#include "../../include/kriol/codegen.hh"
#include "../../include/kriol/sema.hh"
#include "../../include/kriol/constants.hh"
#include "../../include/kriol/diagnostic.hh"

#include "../../include/external/argparse.hpp"

#include <algorithm>
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

// The targets this build of the compiler can produce.
static std::vector<std::string> SupportedTargets()
{
    std::vector<std::string> targets = {"native"};
#if KRIOL_ENABLE_WASM
    targets.push_back("wasm32-wasi");
#endif
#if KRIOL_ENABLE_WINDOWS_TARGET
    targets.push_back("x86_64-windows");
#endif
    return targets;
}

static std::string JoinWords(const std::vector<std::string>& words)
{
    std::string joined;
    for (std::size_t i = 0; i < words.size(); ++i)
        joined += (i == 0 ? "" : i + 1 == words.size() ? " ou " : ", ") + words[i];
    return joined;
}

static std::string HelpText()
{
    return
        "Utilização: kriol [opções] ficheiro.kriol\n"
        "            kriol [opções] --text 'código'\n\n"
        "Compila um programa Kriol num executável.\n\n"
        "Opções:\n"
        "  -o, --output FICHEIRO  Onde escrever o resultado (por omissão, ./" KR_DEFAULT_OUT_FILE
        ", ou ./" KR_DEFAULT_WASM_OUT_FILE " para wasm32-wasi)\n"
        "  --text CÓDIGO          Compila o código indicado em vez de um ficheiro\n"
        "  --target ALVO          Plataforma de destino: " + JoinWords(SupportedTargets()) +
        " (por omissão, native)\n"
        "  --opt-lvl N            Nível de otimização, de 0 (nenhuma) a 3 (por omissão, 2)\n"
        "  --emit-ir              Escreve o IR do LLVM em vez do executável, no ecrã ou no ficheiro de -o\n"
        "  --strict               Trata os avisos como erros\n"
        "  --ignore-extension     Aceita ficheiros sem a extensão ." KR_STANDARD_FILE_EXTENSION
        " ou ." KR_ALTERNATIVE_FILE_EXTENSION "\n"
        "  -h, --help             Mostra esta ajuda\n"
        "  -v, --version          Mostra a versão do compilador\n\n"
        "Exemplos:\n"
        "  kriol ola.kriol\n"
        "  kriol ola.kriol -o ola\n"
        "  kriol ola.kriol --target wasm32-wasi -o ola.wasm\n"
        "  kriol --text 'fn inisiu() { mostran(\"Oi\"); }' --emit-ir\n";
}

// Rewrites the argument parser's own errors in Portuguese.
static std::string ArgumentError(const std::string& error)
{
    const auto quoted = [&](std::size_t from) {
        std::string value = error.substr(from);
        if (!value.empty() && value.back() == '.') value.pop_back();
        if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'')
            value = value.substr(1, value.size() - 2);
        return "'" + value + "'";
    };

    const std::string unknown = "Unknown argument: ";
    const std::string tooFew = "Too few arguments for ";
    const std::string tooMany = "failed to parse ";
    if (error.rfind(unknown, 0) == 0)
        return "a opção " + quoted(unknown.size()) + " não existe";
    if (error.rfind(tooFew, 0) == 0)
        return "falta o valor da opção " + quoted(tooFew.size());
    if (const auto at = error.find(tooMany); at != std::string::npos)
        return "só se pode compilar um ficheiro de cada vez, e " + quoted(at + tooMany.size()) + " está a mais";
    return "argumentos inválidos (" + error + ")";
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
    g_parse_errors.push_back(DiagnosticPrefix(g_source_file, line, "erro") + message);
}

bool cli::HasParseErrors() {
    return !g_parse_errors.empty();
}

void cli::PrintErr(std::string message)
{
    std::cerr << KR_STANDARD_COMPILER_NAME << ": erro: " << message << std::endl;
}

void cli::PrintErr(std::string message, int exitNum)
{
    cli::PrintErr(message);
    throw cli::FatalError(message, exitNum);
}


// The options are documented by HelpText(), which must follow any change here.
void cli::Compiler::DefineArgs()
{
    Parser->add_argument("file").nargs(ap::nargs_pattern::optional);
    Parser->add_argument("--text").nargs(1);
    Parser->add_argument("-o", "--output").nargs(1);
    Parser->add_argument("--emit-ir").flag();
    Parser->add_argument("--target").default_value(std::string("native")).nargs(1);
    Parser->add_argument("--opt-lvl").default_value(std::string("2")).nargs(1);
    Parser->add_argument("--strict").flag();
    Parser->add_argument("--ignore-extension").flag();
    Parser->add_argument("-h", "--help").flag();
    Parser->add_argument("-v", "--version").flag();
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
        cli::PrintErr(ArgumentError(e.what()) + "; usa 'kriol --help' para ver as opções", 1);
    }

    if (Parser->get<bool>("--help"))
    {
        std::cout << HelpText();
        throw cli::FatalError("help", 0);
    }
    if (Parser->get<bool>("--version"))
    {
        std::cout << Version << std::endl;
        throw cli::FatalError("version", 0);
    }

    const auto targets = SupportedTargets();
    const auto target = Parser->get<std::string>("--target");
    if (std::find(targets.begin(), targets.end(), target) == targets.end())
        cli::PrintErr("o alvo '" + target + "' não é suportado; os alvos possíveis são " + JoinWords(targets), 1);

    const auto optLevel = Parser->get<std::string>("--opt-lvl");
    if (optLevel.size() != 1 || optLevel[0] < '0' || optLevel[0] > '3')
        cli::PrintErr("o nível de otimização tem de ser 0, 1, 2 ou 3, mas é '" + optLevel + "'", 1);

    auto file = Parser->present<std::string>("file");
    auto text = Parser->present<std::string>("--text");
    if (file.has_value() == text.has_value())
        cli::PrintErr(file ? "indica o programa só de uma forma: um ficheiro ou --text 'código', não os dois"
                           : "falta o programa a compilar; indica um ficheiro, por exemplo 'kriol ola.kriol', "
                             "ou usa 'kriol --help' para ver as opções", 1);

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
    Args.optLevel = static_cast<unsigned>(std::stoul(Parser->get<std::string>("--opt-lvl")));
    Args.emitIR = Parser->get<bool>("--emit-ir");
    Args.strict = Parser->get<bool>("--strict");
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
        cli::PrintErr("não foi possível criar o ficheiro '" + filename + "': " + std::strerror(errno), 1);

    file.write(code.data(), static_cast<std::streamsize>(code.size()));
    if (!file)
        cli::PrintErr("não foi possível escrever o ficheiro '" + filename + "'", 1);
}

void cli::KriolLangParserWrapper::ParseFile(
    const std::string& filename,
    ast::BlockSttmt** program
)
{
    const fs::path path = PathFromUtf8(filename);

    if (!fs::exists(path))
    {
        cli::PrintErr("o ficheiro '" + filename + "' não existe", 1);
    }

    if (!fs::is_regular_file(path))
    {
        cli::PrintErr("'" + filename + "' não é um ficheiro", 1);
    }

    FILE *file = OpenFileForRead(filename);

    if (file == NULL)
    {
        cli::PrintErr("não foi possível abrir o ficheiro '" + filename + "': " + std::strerror(errno), 1);
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
        cli::PrintErr("não foi possível ler o código indicado", 1);

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
        throw std::invalid_argument("o programa a compilar está vazio");
    if (!IsKnownTarget(options.target))
        throw std::invalid_argument("o alvo '" + options.target + "' não existe");
    if (options.emitIR && options.outputToMemory)
        throw std::invalid_argument("emitIR e outputToMemory não podem ser usados ao mesmo tempo");
    if (options.outputToMemory && !options.outfile.empty())
        throw std::invalid_argument("outfile não pode ser usado com outputToMemory");
    if (options.optLevel > 3)
        throw std::invalid_argument("o nível de otimização tem de estar entre 0 e 3");
    if (options.outputToMemory && options.target != "wasm32-wasi")
        throw std::invalid_argument("a compilação para memória só é suportada para wasm32-wasi");

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
        if (options.strict && !sema.GetWarnings().empty())
        {
            result.diagnostics = sema.GetWarnings();
            return result;
        }
        result.warnings = sema.GetWarnings();
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
            "'" + Args.input + "' não tem a extensão de um programa Kriol (." +
            std::string(KR_STANDARD_FILE_EXTENSION) + " ou ." +
            std::string(KR_ALTERNATIVE_FILE_EXTENSION) + "); para o compilar na mesma, usa --ignore-extension",
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
    options.strict = Args.strict;
    return options;
}

void cli::Compiler::Run(int argc, const char* const* argv)
{
    ParseArgs(argc, argv);
    ValidateInput();

    try
    {
        // Diagnostics already carry their "file:line: erro: " prefix.
        CompileResult result = Compile(MakeCompileOptions());
        for (const auto& warning : result.warnings)
            std::cerr << warning << std::endl;
        if (!result.diagnostics.empty())
        {
            for (const auto& diagnostic : result.diagnostics)
                std::cerr << diagnostic << std::endl;
            throw cli::FatalError("o programa tem erros", 1);
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
