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

    void PrintErr(std::string message);
    void PrintErr(std::string message, int exitNum);
    void PrintErr(
        const std::string& file,
        int line,
        const std::string& message,
        int exitNum = -1
    );

    void SetSourceFile(const std::string& filename);
    const std::string& GetSourceFile();

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
        bool emitIR = false;
        bool outputToMemory = false;
    };

    struct CompileResult
    {
        std::string ir;
        std::string outputPath;
        std::vector<unsigned char> outputBytes;
        std::vector<std::string> diagnostics;
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
            bool emitIR = false;
            bool ignoreExtension = false;
        };

        std::unique_ptr<ap::ArgumentParser> Parser;
        CommandLineOptions Args;

    public:
        Compiler(std::string name, std::string version)
            : Parser(std::make_unique<ap::ArgumentParser>(
                std::move(name),
                std::move(version)
            )) {}

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
