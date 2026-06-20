#ifndef KRIOL_TESTS_FUZZ_COMPILE_FUZZ_DRIVER_HH
#define KRIOL_TESTS_FUZZ_COMPILE_FUZZ_DRIVER_HH

#include "include/kriol/cli.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <streambuf>
#include <string>

extern int yylex_destroy(void);

namespace kriol::tests::fuzz
{
    constexpr std::size_t MaxInputBytes = 64 * 1024;

    class ScannerCleanup
    {
    public:
        ~ScannerCleanup() { yylex_destroy(); }
    };

    class NullBuffer final : public std::streambuf
    {
        int overflow(int ch) override { return ch; }
    };

    class CerrSilencer
    {
        inline static NullBuffer Null;
        std::streambuf* Previous;

    public:
        CerrSilencer() : Previous(std::cerr.rdbuf(&Null)) {}
        ~CerrSilencer() { std::cerr.rdbuf(Previous); }
    };

    inline int CompileOneInput(
        const std::uint8_t* data,
        std::size_t size,
        const std::string& sourceName = "<fuzz>"
    )
    {
        if (!data || size == 0)
            return 0;

        size = std::min(size, MaxInputBytes);
        std::string source(reinterpret_cast<const char*>(data), size);
        std::replace(source.begin(), source.end(), '\0', ' ');

        cli::CompileOptions options;
        options.inputKind = cli::CompileInputKind::SourceText;
        options.input = source;
        options.sourceName = sourceName;
        options.target = "native";
        options.emitIR = true;

        ScannerCleanup cleanup;
        CerrSilencer silence;
        try
        {
            (void)cli::Compile(options);
        }
        catch (const cli::FatalError&)
        {
            // Syntax and scanner diagnostics are expected for arbitrary bytes.
        }

        return 0;
    }
}

#endif // KRIOL_TESTS_FUZZ_COMPILE_FUZZ_DRIVER_HH
