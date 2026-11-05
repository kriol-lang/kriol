#include "include/kriol/cli.hh"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            std::exit(1); \
        } \
    } while (0)

namespace {

// Locals are emitted as allocas named after the variable; reading them back by name
// keeps these checks on the source program instead of the generated temporaries.
std::string compileToIR(const std::string& source) {
    kriol::cli::CompileOptions options;
    options.inputKind = kriol::cli::CompileInputKind::SourceText;
    options.input = source;
    options.sourceName = "spia_volatile_locals.kr";
    options.emitIR = true;
    kriol::cli::CompileResult result = kriol::cli::Compile(options);
    for (const std::string& diagnostic : result.diagnostics)
        std::fprintf(stderr, "%s\n", diagnostic.c_str());
    CHECK(result.diagnostics.empty());
    return result.ir;
}

struct Accesses {
    unsigned total = 0;
    unsigned isVolatile = 0;
};

Accesses accessesTo(const std::string& ir, const std::string& local) {
    Accesses found;
    const std::string operand = "ptr %" + local + ", align";
    std::istringstream lines(ir);
    for (std::string line; std::getline(lines, line);) {
        const bool memoryAccess = line.find(" load ") != std::string::npos
            || line.find("store ") != std::string::npos;
        if (!memoryAccess || line.find(operand) == std::string::npos)
            continue;
        ++found.total;
        if (line.find(" volatile ") != std::string::npos)
            ++found.isVolatile;
    }
    return found;
}

void expectRegisterFriendly(const std::string& ir, const std::string& local) {
    const Accesses found = accessesTo(ir, local);
    if (found.total == 0 || found.isVolatile != 0)
        std::fprintf(stderr, "'%s': %u of %u accesses volatile, expected none\n",
                     local.c_str(), found.isVolatile, found.total);
    CHECK(found.total > 0 && found.isVolatile == 0);
}

void expectKeptInMemory(const std::string& ir, const std::string& local) {
    const Accesses found = accessesTo(ir, local);
    if (found.total == 0 || found.isVolatile != found.total)
        std::fprintf(stderr, "'%s': %u of %u accesses volatile, expected all\n",
                     local.c_str(), found.isVolatile, found.total);
    CHECK(found.total > 0 && found.isVolatile == found.total);
}

const char* const Program = R"(
fn zero() int {
    divolvi 0;
}

fn livri() int {
    int simplis = 1;
    simplis += 1;
    divolvi simplis;
}

fn inisiu() {
    int antis = 0;
    pa int i = 0; i < 3; i += 1 {
        antis += i;
    }
    int lidu = 2;
    int mudadu = 1;
    spia {
        mudadu = antis + lidu / zero();
    } oops (e) {
        mostran(f"{mudadu}");
    }
    int dipos = mudadu;
    dipos += 1;
    mostran(f"{dipos}");

    int soma = 0;
    pa int k = 0; k < 3; k += 1 {
        spia {
            soma += k / zero();
        } oops (e) {
            mostran(f"{k}");
        }
    }
    mostran(f"{soma + livri()}");

    int naOops = 0;
    spia {
        lansa Erru{mensagem = "x"};
    } oops (e) {
        naOops = 1;
        int y = 1 / zero();
    } sempri {
        mostran(f"{naOops}");
    }
}
)";

void only_locals_written_under_a_handler_stay_in_memory() {
    const std::string ir = compileToIR(Program);

    expectRegisterFriendly(ir, "antis");
    expectRegisterFriendly(ir, "i");
    expectRegisterFriendly(ir, "lidu");
    expectRegisterFriendly(ir, "dipos");
    expectRegisterFriendly(ir, "k");
    expectRegisterFriendly(ir, "simplis");

    expectKeptInMemory(ir, "mudadu");
    expectKeptInMemory(ir, "soma");
    expectKeptInMemory(ir, "naOops");
}

} // namespace

int main() {
    only_locals_written_under_a_handler_stay_in_memory();
    return 0;
}
