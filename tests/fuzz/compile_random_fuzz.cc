#include "tests/fuzz/compile_fuzz_driver.hh"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::uint64_t readU64Env(const char* name, std::uint64_t fallback)
    {
        const char* raw = std::getenv(name);
        if (!raw || *raw == '\0')
            return fallback;
        try
        {
            return std::stoull(raw);
        }
        catch (...)
        {
            return fallback;
        }
    }

    int randomInt(std::mt19937_64& rng, int min, int max)
    {
        return std::uniform_int_distribution<int>(min, max)(rng);
    }

    bool chance(std::mt19937_64& rng, int percent)
    {
        return randomInt(rng, 1, 100) <= percent;
    }

    const std::string& pick(std::mt19937_64& rng, const std::vector<std::string>& values)
    {
        return values[static_cast<std::size_t>(randomInt(rng, 0, static_cast<int>(values.size() - 1)))];
    }

    std::string ident(std::mt19937_64& rng)
    {
        static const std::vector<std::string> names = {
            "a", "b", "c", "x", "y", "z", "valor", "kontador", "arr", "obj", "_tmp"
        };
        return pick(rng, names) + std::to_string(randomInt(rng, 0, 9));
    }

    std::string typeName(std::mt19937_64& rng)
    {
        static const std::vector<std::string> types = {
            "nter", "num", "bool", "textu", "i8", "i16", "i32", "i64",
            "u8", "u16", "u32", "u64", "f32", "f64", "isize", "usize"
        };
        return pick(rng, types);
    }

    std::string literal(std::mt19937_64& rng)
    {
        switch (randomInt(rng, 0, 7))
        {
            case 0:
                return std::to_string(randomInt(rng, -100000, 100000));
            case 1:
                return std::to_string(randomInt(rng, -1000, 1000)) + "." +
                       std::to_string(randomInt(rng, 0, 999999));
            case 2:
                return chance(rng, 50) ? "sin" : "nau";
            case 3:
                return "\"textu " + std::to_string(randomInt(rng, 0, 999)) + "\"";
            case 4:
                return "'linha\\n" + std::to_string(randomInt(rng, 0, 999)) + "'";
            case 5:
                return "f\"valor { " + ident(rng) + " }\"";
            case 6:
                return "[ " + std::to_string(randomInt(rng, -32, 32)) + ", " +
                       std::to_string(randomInt(rng, -32, 32)) + " ]";
            default:
                return "f\"unterminated { " + ident(rng);
        }
    }

    std::string expr(std::mt19937_64& rng, int depth)
    {
        if (depth <= 0 || chance(rng, 35))
            return chance(rng, 50) ? literal(rng) : ident(rng);

        switch (randomInt(rng, 0, 8))
        {
            case 0:
                return "(" + expr(rng, depth - 1) + ")";
            case 1:
                return expr(rng, depth - 1) + " + " + expr(rng, depth - 1);
            case 2:
                return expr(rng, depth - 1) + " * " + expr(rng, depth - 1);
            case 3:
                return expr(rng, depth - 1) + " == " + expr(rng, depth - 1);
            case 4:
                return "!" + expr(rng, depth - 1);
            case 5:
                return "-" + expr(rng, depth - 1);
            case 6:
                return ident(rng) + "(" + expr(rng, depth - 1) + ")";
            case 7:
                return ident(rng) + "[" + expr(rng, depth - 1) + "]";
            default:
                return ident(rng) + "." + ident(rng);
        }
    }

    std::string statement(std::mt19937_64& rng, int depth)
    {
        switch (randomInt(rng, 0, 9))
        {
            case 0:
                return typeName(rng) + " " + ident(rng) + " = " + expr(rng, depth) + ";";
            case 1:
                return "dipoz " + typeName(rng) + " " + ident(rng) + ";";
            case 2:
                return "[" + std::to_string(randomInt(rng, 0, 64)) + "] " + ident(rng) +
                       " = [ " + expr(rng, 1) + " ];";
            case 3:
                return ident(rng) + " = " + expr(rng, depth) + ";";
            case 4:
                return "mostran(" + expr(rng, depth) + ");";
            case 5:
                return "si " + expr(rng, depth) + " { " + statement(rng, depth - 1) + " }";
            case 6:
                return "nkuantu " + expr(rng, depth) + " { " + statement(rng, depth - 1) + " }";
            case 7:
                return "para " + statement(rng, 1) + " " + expr(rng, 1) + "; " +
                       ident(rng) + " += 1 { " + statement(rng, depth - 1) + " }";
            case 8:
                return "divolvi " + expr(rng, depth) + ";";
            default:
                return expr(rng, depth) + ";";
        }
    }

    std::string molda(std::mt19937_64& rng)
    {
        std::ostringstream out;
        out << "molda T" << randomInt(rng, 0, 999) << " { ";
        const int fields = randomInt(rng, 0, 8);
        for (int i = 0; i < fields; ++i)
            out << typeName(rng) << ' ' << ident(rng) << "; ";
        out << "}\n";
        return out.str();
    }

    std::string function(std::mt19937_64& rng, bool entry)
    {
        std::ostringstream out;
        out << "fn " << (entry ? "inisiu" : ident(rng)) << "(";
        const int params = randomInt(rng, 0, 5);
        for (int i = 0; i < params; ++i)
        {
            if (i > 0)
                out << ", ";
            out << typeName(rng) << ' ' << ident(rng);
        }
        out << ")";
        if (!entry && chance(rng, 40))
            out << ' ' << typeName(rng);
        out << " {\n";
        const int statements = randomInt(rng, 0, 24);
        for (int i = 0; i < statements; ++i)
            out << "  " << statement(rng, 4) << "\n";
        out << "}\n";
        return out.str();
    }

    std::string program(std::mt19937_64& rng)
    {
        std::ostringstream out;
        const int declarations = randomInt(rng, 0, 6);
        for (int i = 0; i < declarations; ++i)
        {
            if (chance(rng, 30))
                out << molda(rng);
            else
                out << function(rng, false);
        }
        out << function(rng, true);
        return out.str();
    }

    std::string tokenSoup(std::mt19937_64& rng, std::size_t maxBytes)
    {
        static const std::vector<std::string> tokens = {
            "fn", "molda", "mostran", "mostra", "dipoz", "si", "sinon", "nkuantu",
            "para", "divolvi", "sai", "kontinua", "konfirma", "nter", "num",
            "bool", "textu", "sin", "nau", "(", ")", "{", "}", "[", "]", ";",
            ",", ".", "::", "=", "==", "!=", "<", "<=", ">", ">=", "+", "-",
            "*", "/", "+=", "-=", "*=", "/=", "&&", "||", "f\"", "\"", "'"
        };

        std::string out;
        while (out.size() < maxBytes)
        {
            if (chance(rng, 20))
                out += ident(rng);
            else if (chance(rng, 20))
                out += literal(rng);
            else
                out += pick(rng, tokens);
            out += chance(rng, 70) ? " " : "\n";
        }
        return out;
    }

    std::string rawBytes(std::mt19937_64& rng, std::size_t maxBytes)
    {
        const auto size = static_cast<std::size_t>(randomInt(rng, 1, static_cast<int>(maxBytes)));
        std::string out;
        out.reserve(size);
        for (std::size_t i = 0; i < size; ++i)
            out.push_back(static_cast<char>(randomInt(rng, 0, 255)));
        return out;
    }

    std::string makeInput(std::mt19937_64& rng, std::size_t maxBytes)
    {
        switch (randomInt(rng, 0, 4))
        {
            case 0:
                return rawBytes(rng, maxBytes);
            case 1:
                return tokenSoup(rng, maxBytes);
            default:
                return program(rng);
        }
    }

    void writeReproducer(const std::string& source)
    {
        std::ofstream out("kriol-random-fuzz-reproducer.kr", std::ios::binary);
        out.write(source.data(), static_cast<std::streamsize>(source.size()));
    }
}

int main()
{
    const std::uint64_t seed = readU64Env("KRIOL_RANDOM_FUZZ_SEED", std::random_device{}());
    const std::uint64_t iterations = readU64Env("KRIOL_RANDOM_FUZZ_ITERATIONS", 1000);
    const std::uint64_t maxBytes = std::min<std::uint64_t>(
        readU64Env("KRIOL_RANDOM_FUZZ_MAX_BYTES", 4096),
        kriol::tests::fuzz::MaxInputBytes
    );

    std::mt19937_64 rng(seed);
    std::cerr << "random fuzz seed=" << seed
              << " iterations=" << iterations
              << " max_bytes=" << maxBytes << '\n';

    for (std::uint64_t i = 0; i < iterations; ++i)
    {
        std::string source = makeInput(rng, static_cast<std::size_t>(maxBytes));
        std::string sourceName = "<random-fuzz:" + std::to_string(i) + ">";
        try
        {
            kriol::tests::fuzz::CompileOneInput(
                reinterpret_cast<const std::uint8_t*>(source.data()),
                source.size(),
                sourceName
            );
        }
        catch (const std::exception& error)
        {
            writeReproducer(source);
            std::cerr << "random fuzz crash candidate at iteration=" << i
                      << " seed=" << seed
                      << " error=" << error.what() << '\n'
                      << "wrote kriol-random-fuzz-reproducer.kr\n";
            return 1;
        }
        catch (...)
        {
            writeReproducer(source);
            std::cerr << "random fuzz non-standard crash candidate at iteration=" << i
                      << " seed=" << seed << '\n'
                      << "wrote kriol-random-fuzz-reproducer.kr\n";
            return 1;
        }
    }

    return 0;
}
