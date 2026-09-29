#include "include/kriol/cli.hh"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

static fs::path writeSource(const std::string& name, const std::string& source) {
    fs::path path = fs::temp_directory_path() / name;
    std::ofstream(path, std::ios::binary) << source;
    return path;
}

// A file whose parse stops at a syntax error must not leave buffered input
// behind for the next file compiled in the same process.
static int checkFileCompileAfterSyntaxError() {
    fs::path bad = writeSource("kriol_api_bad.kriol",
        "fn inisiu() {\n"
        "    nter x = ;\n"
        "    mostran(\"text left in the scanner buffer\");\n"
        "}\n");
    fs::path good = writeSource("kriol_api_good.kriol",
        "fn inisiu() {\n"
        "    mostran(\"Kuale, Mundu!\");\n"
        "}\n");

    int status = 0;
    kriol::cli::CompileOptions options;
    options.emitIR = true;

    options.input = bad.string();
    auto badResult = kriol::cli::Compile(options);
    if (badResult.diagnostics.empty()
            || badResult.diagnostics[0].find(":2: syntax error") == std::string::npos) {
        std::cerr << "expected a syntax error diagnostic on line 2\n";
        status = 1;
    }

    options.input = good.string();
    try {
        if (kriol::cli::Compile(options).ir.empty()) {
            std::cerr << "expected the valid file to produce LLVM IR\n";
            status = 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "valid file failed after a syntax error: " << e.what() << '\n';
        status = 1;
    }

    fs::remove(bad);
    fs::remove(good);
    return status;
}

int main() {
    if (checkFileCompileAfterSyntaxError() != 0)
        return 1;

#if !KRIOL_ENABLE_WASM
    // In-memory output is wasm32-wasi only, which this build leaves out.
    return 0;
#endif

    kriol::cli::CompileOptions options;
    options.inputKind = kriol::cli::CompileInputKind::SourceText;
    options.input =
        "fn inisiu() {\n"
        "    mostran(\"Kuale, Mundu!\");\n"
        "}\n";
    options.sourceName = "<api-memory-test>";
    options.target = "wasm32-wasi";

    options.emitIR = true;
    kriol::cli::CompileResult irResult = kriol::cli::Compile(options);
    if (irResult.ir.find("define i32 @main()") == std::string::npos) {
        std::cerr << "expected source-text compile to emit main in LLVM IR\n";
        return 1;
    }

    options.emitIR = false;
    options.outputToMemory = true;

    kriol::cli::CompileResult result = kriol::cli::Compile(options);
    if (!result.diagnostics.empty()) {
        for (const auto& diagnostic : result.diagnostics)
            std::cerr << diagnostic << '\n';
        return 1;
    }

    if (!result.outputPath.empty()) {
        std::cerr << "expected in-memory compile to leave outputPath empty\n";
        return 1;
    }

    const auto& bytes = result.outputBytes;
    if (bytes.size() < 4 ||
        bytes[0] != 0x00 ||
        bytes[1] != 0x61 ||
        bytes[2] != 0x73 ||
        bytes[3] != 0x6d) {
        std::cerr << "expected in-memory compile to return a WebAssembly module\n";
        return 1;
    }

    return 0;
}
