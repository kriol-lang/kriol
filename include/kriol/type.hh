#ifndef _KRIOL_TYPE_HEADER
#define _KRIOL_TYPE_HEADER

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace kriol {

enum class TypeKind {
    Invalid,
    Void,
    Bool,
    Integer,
    Number,
    Text,
    Named,
    FixedArray,
    Reference,
    ArrayLiteral,
    ArrayRepeat
};

class Type {
public:
    Type() = default;

    static Type Invalid() { return Type(); }
    static Type Void() { return Type(TypeKind::Void); }
    static Type Bool() { return Type(TypeKind::Bool); }
    static Type Integer() { return Type(TypeKind::Integer); }
    static Type Number() { return Type(TypeKind::Number); }
    static Type Text() { return Type(TypeKind::Text); }
    static Type ArrayLiteral() { return Type(TypeKind::ArrayLiteral); }
    static Type ArrayRepeat() { return Type(TypeKind::ArrayRepeat); }

    static Type Named(std::string name) {
        Type type(TypeKind::Named);
        type.Name = std::move(name);
        return type;
    }

    static Type FixedArray(Type element, std::size_t size) {
        Type type(TypeKind::FixedArray);
        type.Element = std::make_shared<Type>(std::move(element));
        type.Size = size;
        return type;
    }

    static Type Reference(Type target) {
        Type type(TypeKind::Reference);
        type.Element = std::make_shared<Type>(std::move(target));
        return type;
    }

    static Type FromName(const std::string& name) {
        if (name.empty()) return Invalid();
        if (name == "vaziu" || name == "void") return Void();
        if (name == "bool") return Bool();
        if (name == "nter") return Integer();
        if (name == "num") return Number();
        if (name == "textu" || name == "char*") return Text();
        return Named(name);
    }

    TypeKind kind() const { return Kind; }
    bool valid() const { return Kind != TypeKind::Invalid; }
    bool isArray() const { return Kind == TypeKind::FixedArray; }
    bool isReference() const { return Kind == TypeKind::Reference; }
    bool isVoid() const { return Kind == TypeKind::Void; }
    bool isNumeric() const { return Kind == TypeKind::Integer || Kind == TypeKind::Number; }

    const Type& elementType() const {
        static const Type invalid;
        return Element ? *Element : invalid;
    }

    std::size_t arraySize() const { return Size; }

    std::string str() const {
        switch (Kind) {
            case TypeKind::Invalid: return "";
            case TypeKind::Void: return "vaziu";
            case TypeKind::Bool: return "bool";
            case TypeKind::Integer: return "nter";
            case TypeKind::Number: return "num";
            case TypeKind::Text: return "textu";
            case TypeKind::Named: return Name;
            case TypeKind::FixedArray:
                return elementType().str() + "[" + std::to_string(Size) + "]";
            case TypeKind::Reference: return "ref " + elementType().str();
            case TypeKind::ArrayLiteral: return "array literal";
            case TypeKind::ArrayRepeat: return "array repeat";
        }
        return "";
    }

    friend bool operator==(const Type& lhs, const Type& rhs) {
        if (lhs.Kind != rhs.Kind || lhs.Name != rhs.Name || lhs.Size != rhs.Size)
            return false;
        if (static_cast<bool>(lhs.Element) != static_cast<bool>(rhs.Element))
            return false;
        return !lhs.Element || *lhs.Element == *rhs.Element;
    }

    friend bool operator!=(const Type& lhs, const Type& rhs) { return !(lhs == rhs); }

private:
    explicit Type(TypeKind kind) : Kind(kind) {}

    TypeKind Kind = TypeKind::Invalid;
    std::string Name;
    std::shared_ptr<Type> Element;
    std::size_t Size = 0;
};

} // namespace kriol

#endif // _KRIOL_TYPE_HEADER
