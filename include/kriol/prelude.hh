#ifndef _KRIOL_PRELUDE_HEADER
#define _KRIOL_PRELUDE_HEADER

#include <memory>
#include <string>

#include "ast.hh"

namespace kriol {
namespace prelude {

// The built-in error type, `molda Erru { textu mensage; }`.
inline constexpr const char* ErrorTypeName = "Erru";
inline constexpr const char* ErrorMessageField = "mensage";

inline std::unique_ptr<ast::MoldaDeclSttmt> makeErrorTypeDecl() {
    auto erru = std::make_unique<ast::MoldaDeclSttmt>(ErrorTypeName);
    erru->AddField(std::make_unique<ast::VarDeclSttmt>(Type::Text(), ErrorMessageField, nullptr));
    return erru;
}

enum class Builtin {
    None,
    Mostra,
    Mostran,
    Toma,
    Sai,
    Konfirma,
    Paniku
};

inline Builtin lookupBuiltin(const std::string& name) {
    if (name == "mostra") return Builtin::Mostra;
    if (name == "mostran") return Builtin::Mostran;
    if (name == "toma") return Builtin::Toma;
    if (name == "sai") return Builtin::Sai;
    if (name == "konfirma") return Builtin::Konfirma;
    if (name == "paniku") return Builtin::Paniku;
    return Builtin::None;
}

inline bool isPreludeName(const std::string& name) {
    return lookupBuiltin(name) != Builtin::None;
}

} // namespace prelude
} // namespace kriol

#endif // _KRIOL_PRELUDE_HEADER
