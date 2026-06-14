#ifndef _KRIOL_TYPE_UTILS_HEADER
#define _KRIOL_TYPE_UTILS_HEADER

#include "type.hh"

#include <optional>
#include <string>

namespace kriol {
namespace typeutils {

inline bool isArrayType(const Type& type) { return type.isArray(); }

inline std::optional<std::size_t> firstArrayDim(const Type& type) {
    if (!type.isArray()) return std::nullopt;
    return type.arraySize();
}

inline Type arrayElementType(const Type& type) {
    return type.isArray() ? type.elementType() : type;
}

} // namespace typeutils
} // namespace kriol

#endif // _KRIOL_TYPE_UTILS_HEADER
