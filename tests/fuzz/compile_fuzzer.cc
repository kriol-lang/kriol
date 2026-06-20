#include "tests/fuzz/compile_fuzz_driver.hh"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    return kriol::tests::fuzz::CompileOneInput(data, size);
}
