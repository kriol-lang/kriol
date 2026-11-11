#ifndef KRIOL_DEPRECATED_HH
#define KRIOL_DEPRECATED_HH

#include <optional>
#include <string>
#include <string_view>

namespace kriol {
namespace deprecated {

// A word that was renamed: the old spelling still compiles, with a warning.
struct RenamedWord {
    std::string_view oldName;
    std::string_view currentName;
};

inline constexpr RenamedWord RenamedWords[] = {
    {"mostran", "mostraln"},  // the built-in that prints and ends the line
};

// The current name of a renamed word, or nothing when the word was not renamed.
inline std::optional<std::string_view> currentNameOf(std::string_view name) {
    for (const auto& word : RenamedWords)
        if (word.oldName == name) return word.currentName;
    return std::nullopt;
}

// The warning text for a use of a renamed word's old spelling.
inline std::string renamedWordWarning(std::string_view oldName, std::string_view currentName) {
    return "'" + std::string(oldName) + "' está obsoleto; usa '" + std::string(currentName) + "'";
}

} // namespace deprecated
} // namespace kriol

#endif // KRIOL_DEPRECATED_HH
