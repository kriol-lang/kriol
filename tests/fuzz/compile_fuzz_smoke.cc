#include "tests/fuzz/compile_fuzz_driver.hh"

#include <cstdint>
#include <string>
#include <vector>

int main()
{
    const std::vector<std::string> inputs = {
        "fn inisiu() { mostran(\"Kuale, Mundu!\"); }",
        "fn inisiu() { mostran(f\"unterminated { value \");",
        std::string("fn inisiu() {\0 mostran(\"nul\"); }", 29),
        "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{"
    };

    for (const auto& input : inputs)
    {
        kriol::tests::fuzz::CompileOneInput(
            reinterpret_cast<const std::uint8_t*>(input.data()),
            input.size()
        );
    }

    return 0;
}
