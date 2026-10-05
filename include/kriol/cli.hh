#ifndef _KRIOL_CLI_HEADER
#define _KRIOL_CLI_HEADER

#include "ast.hh"

#include "../external/argparse.hpp"

#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ap = argparse;

namespace kriol::cli
{
    class FatalError : public std::exception
    {
        std::string Message;
        int ExitCode;

    public:
        FatalError(std::string message, int exitCode)
            : Message(std::move(message)), ExitCode(exitCode) {}

        const char* what() const noexcept override { return Message.c_str(); }
        int exitCode() const noexcept { return ExitCode; }
    };

    // Writes "kriol: erro: <message>", for errors without a source location.
    void PrintErr(std::string message);
    void PrintErr(std::string message, int exitNum);

    void SetSourceFile(const std::string& filename);
    const std::string& GetSourceFile();

    // Compile() returns the recorded errors as diagnostics after parsing.
    void ReportParseError(int line, const std::string& message);
    bool HasParseErrors();

    enum class CompileInputKind
    {
        File,
        SourceText
    };

    struct CompileOptions
    {
        CompileInputKind inputKind = CompileInputKind::File;
        std::string input;
        std::string sourceName;
        std::string outfile;
        std::string target = "native";
        unsigned optLevel = 2;
        bool emitIR = false;
        bool outputToMemory = false;
        // Treat warnings as errors.
        bool strict = false;
    };

    struct CompileResult
    {
        std::string ir;
        std::string outputPath;
        std::vector<unsigned char> outputBytes;
        std::vector<std::string> diagnostics;
        std::vector<std::string> warnings;
    };

    CompileResult Compile(const CompileOptions& options);

    class KriolLangParserWrapper
    {
        static void ParseFile(const std::string& filename, ast::BlockSttmt** program);
        static void ParseText(const std::string& text, ast::BlockSttmt** program);

    public:
        static ast::BlockSttmt* ParseCode(
            const std::string& input,
            CompileInputKind inputKind
        );
    };

    class Compiler
    {
        struct CommandLineOptions
        {
            CompileInputKind inputKind = CompileInputKind::File;
            std::string input;
            std::string sourceName;
            std::string outfile;
            std::string target = "native";
            unsigned optLevel = 2;
            bool emitIR = false;
            bool strict = false;
            bool ignoreExtension = false;
        };

        std::unique_ptr<ap::ArgumentParser> Parser;
        std::string Version;
        CommandLineOptions Args;

    public:
        // The help and version options are defined by DefineArgs, so that
        // their text is in Portuguese like the rest of the compiler.
        Compiler(std::string name, std::string version)
            : Parser(std::make_unique<ap::ArgumentParser>(
                std::move(name),
                version,
                ap::default_arguments::none
            )),
              Version(std::move(version)) {}

        void Run(int argc, const char* const* argv);
        static void Cleanup();
        static void SaveCodeToFile(const std::string& code, const std::string& filename);

    private:
        void DefineArgs();
        void ParseArgs(int argc, const char* const* argv);
        void ValidateInput() const;
        CompileOptions MakeCompileOptions() const;
    };
}

#endif // _KRIOL_CLI_HEADER
