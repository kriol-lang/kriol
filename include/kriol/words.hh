#ifndef KRIOL_WORDS_HH
#define KRIOL_WORDS_HH

#include <string>
#include <string_view>
#include <vector>

namespace kriol {
namespace words {

enum class Kind {
    Keyword,
    BuiltinType,
    Boolean
};

// A word the scanner reads as its own token, never as a name.
struct ScannerWord {
    std::string_view name;
    Kind kind;
};

// Every word the scanner reads as its own token. Defined in rules/scanner.l,
// from the table the scanner looks names up in.
const std::vector<ScannerWord>& scannerWords();

// The words of the language as JSON, printed by the hidden option
// kriol --print-words: keywords, builtinTypes, booleans, builtinFunctions,
// preludeTypes and deprecatedWords. syntax/ checks kriol-syntax.json against it.
std::string toJson();

} // namespace words
} // namespace kriol

#endif // KRIOL_WORDS_HH
