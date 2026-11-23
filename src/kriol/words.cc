#include "../../include/kriol/words.hh"
#include "../../include/kriol/deprecated.hh"
#include "../../include/kriol/prelude.hh"

namespace kriol {
namespace words {

namespace {

// The words are identifiers, so they need no escaping in JSON.
std::string quoted(std::string_view word) {
    return "\"" + std::string(word) + "\"";
}

std::string list(const std::vector<std::string_view>& names) {
    std::string json = "[";
    for (std::size_t i = 0; i < names.size(); ++i)
        json += (i == 0 ? "" : ", ") + quoted(names[i]);
    return json + "]";
}

std::vector<std::string_view> scannerWordsOf(Kind kind) {
    std::vector<std::string_view> names;
    for (const auto& word : scannerWords())
        if (word.kind == kind) names.push_back(word.name);
    return names;
}

std::vector<std::string_view> builtinFunctionNames() {
    std::vector<std::string_view> names;
    for (const auto& function : prelude::BuiltinFunctions) names.push_back(function.name);
    return names;
}

std::string renamedWords() {
    std::string json = "{";
    bool first = true;
    for (const auto& word : deprecated::RenamedWords) {
        json += (first ? "" : ", ") + quoted(word.oldName) + ": " + quoted(word.currentName);
        first = false;
    }
    return json + "}";
}

} // namespace

std::string toJson() {
    return "{\n"
           "  \"keywords\": " + list(scannerWordsOf(Kind::Keyword)) + ",\n"
           "  \"builtinTypes\": " + list(scannerWordsOf(Kind::BuiltinType)) + ",\n"
           "  \"booleans\": " + list(scannerWordsOf(Kind::Boolean)) + ",\n"
           "  \"builtinFunctions\": " + list(builtinFunctionNames()) + ",\n"
           "  \"preludeTypes\": " + list({prelude::ErrorTypeName}) + ",\n"
           "  \"deprecatedWords\": " + renamedWords() + "\n"
           "}\n";
}

} // namespace words
} // namespace kriol
