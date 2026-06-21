#ifndef _KRIOL_PRELUDE_HEADER
#define _KRIOL_PRELUDE_HEADER

#include <string>

namespace kriol {
namespace prelude {

enum class Builtin {
    None,
    Mostra,
    Mostran,
    Toma,
    Sai,
    Konfirma
};

inline Builtin lookupBuiltin(const std::string& name) {
    if (name == "mostra") return Builtin::Mostra;
    if (name == "mostran") return Builtin::Mostran;
    if (name == "toma") return Builtin::Toma;
    if (name == "sai") return Builtin::Sai;
    if (name == "konfirma") return Builtin::Konfirma;
    return Builtin::None;
}

inline bool isPreludeName(const std::string& name) {
    return lookupBuiltin(name) != Builtin::None;
}

} // namespace prelude
} // namespace kriol

#endif // _KRIOL_PRELUDE_HEADER
