#ifndef _KRIOL_PRELUDE_HEADER
#define _KRIOL_PRELUDE_HEADER

#include <memory>
#include <string>
#include <string_view>

#include "ast.hh"
#include "deprecated.hh"

namespace kriol {
namespace prelude {

// The built-in error type, `molda Erru { textu mensagem; textu tipu; textu fixeru; int linha; }`.
inline constexpr const char* ErrorTypeName = "Erru";
inline constexpr const char* ErrorMessageField = "mensagem";
inline constexpr const char* ErrorKindField = "tipu";
inline constexpr const char* ErrorFileField = "fixeru";
inline constexpr const char* ErrorLineField = "linha";

inline bool isErrorFilledField(const std::string& name) {
    return name == ErrorKindField || name == ErrorFileField || name == ErrorLineField;
}

inline std::unique_ptr<ast::MoldaDeclSttmt> makeErrorTypeDecl() {
    auto erru = std::make_unique<ast::MoldaDeclSttmt>(ErrorTypeName);
    erru->AddField(std::make_unique<ast::VarDeclSttmt>(Type::Text(), ErrorMessageField, nullptr));
    erru->AddField(std::make_unique<ast::VarDeclSttmt>(Type::Text(), ErrorKindField, nullptr));
    erru->AddField(std::make_unique<ast::VarDeclSttmt>(Type::Text(), ErrorFileField, nullptr));
    erru->AddField(std::make_unique<ast::VarDeclSttmt>(Type::Integer(), ErrorLineField, nullptr));
    return erru;
}

enum class Builtin {
    None,
    Mostra,
    Mostraln,
    Toma,
    Sai,
    Konfirma,
    Paniku
};

struct BuiltinFunction {
    std::string_view name;
    Builtin builtin;
};

// The prelude's functions. kriol --print-words lists them, and
// syntax/kriol-syntax.json must follow this table.
inline constexpr BuiltinFunction BuiltinFunctions[] = {
    {"mostra", Builtin::Mostra},
    {"mostraln", Builtin::Mostraln},
    {"toma", Builtin::Toma},
    {"sai", Builtin::Sai},
    {"konfirma", Builtin::Konfirma},
    {"paniku", Builtin::Paniku},
};

// The built-in a name calls; an old spelling of a renamed built-in calls it too.
inline Builtin lookupBuiltin(const std::string& name) {
    if (auto currentName = deprecated::currentNameOf(name))
        return lookupBuiltin(std::string(*currentName));
    for (const auto& function : BuiltinFunctions)
        if (function.name == name) return function.builtin;
    return Builtin::None;
}

inline bool isPreludeName(const std::string& name) {
    return lookupBuiltin(name) != Builtin::None;
}

} // namespace prelude
} // namespace kriol

#endif // _KRIOL_PRELUDE_HEADER
