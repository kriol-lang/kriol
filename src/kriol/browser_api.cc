#include "../../include/kriol/cli.hh"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <sstream>
#include <string>

namespace {

char* copyString(const std::string& text) {
    char* out = static_cast<char*>(std::malloc(text.size() + 1));
    if (!out)
        return nullptr;
    std::memcpy(out, text.data(), text.size());
    out[text.size()] = '\0';
    return out;
}

void setError(char** errorOut, const std::string& message) {
    if (errorOut)
        *errorOut = copyString(message);
}

std::string joinDiagnostics(const std::vector<std::string>& diagnostics) {
    std::ostringstream out;
    for (const auto& diagnostic : diagnostics)
        out << diagnostic << '\n';
    return out.str();
}

} // namespace

extern "C" {

int kriol_compile_source_to_wasm(const char* source,
                                 std::size_t sourceLen,
                                 unsigned char** wasmOut,
                                 std::size_t* wasmLenOut,
                                 char** errorOut) {
    if (wasmOut)
        *wasmOut = nullptr;
    if (wasmLenOut)
        *wasmLenOut = 0;
    if (errorOut)
        *errorOut = nullptr;

    if (!source || !wasmOut || !wasmLenOut) {
        setError(errorOut, "Invalid kriol_compile_source_to_wasm arguments.");
        return 1;
    }

    try {
        kriol::cli::CompileOptions options;
        options.inputKind = kriol::cli::CompileInputKind::SourceText;
        options.input.assign(source, sourceLen);
        options.sourceName = "<browser>";
        options.target = "wasm32-wasi";
        options.outputToMemory = true;

        kriol::cli::CompileResult result = kriol::cli::Compile(options);
        if (!result.diagnostics.empty()) {
            setError(errorOut, joinDiagnostics(result.diagnostics));
            return 2;
        }

        unsigned char* wasm = static_cast<unsigned char*>(std::malloc(result.outputBytes.size()));
        if (!wasm && !result.outputBytes.empty()) {
            setError(errorOut, "Failed to allocate wasm output buffer.");
            return 1;
        }

        if (!result.outputBytes.empty())
            std::memcpy(wasm, result.outputBytes.data(), result.outputBytes.size());
        *wasmOut = wasm;
        *wasmLenOut = result.outputBytes.size();
        return 0;
    } catch (const std::exception& err) {
        setError(errorOut, err.what());
        return 1;
    } catch (...) {
        setError(errorOut, "Unknown compiler failure.");
        return 1;
    }
}

void kriol_free(void* ptr) {
    std::free(ptr);
}

} // extern "C"
