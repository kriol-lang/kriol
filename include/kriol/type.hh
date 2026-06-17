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
    Float,
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
    static Type Integer() { return SignedInteger(64); }
    static Type Number() { return Float(64); }
    static Type Text() { return Type(TypeKind::Text); }
    static Type ArrayLiteral() { return Type(TypeKind::ArrayLiteral); }
    static Type ArrayRepeat() { return Type(TypeKind::ArrayRepeat); }

    static Type SignedInteger(unsigned bits) {
        Type type(TypeKind::Integer);
        type.Signed = true;
        type.Bits = bits;
        return type;
    }

    static Type UnsignedInteger(unsigned bits) {
        Type type(TypeKind::Integer);
        type.Signed = false;
        type.Bits = bits;
        return type;
    }

    static Type Float(unsigned bits) {
        Type type(TypeKind::Float);
        type.Bits = bits;
        return type;
    }

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
        if (name == "nter" || name == "i64") return SignedInteger(64);
        if (name == "i8") return SignedInteger(8);
        if (name == "i16") return SignedInteger(16);
        if (name == "i32") return SignedInteger(32);
        if (name == "u8") return UnsignedInteger(8);
        if (name == "u16") return UnsignedInteger(16);
        if (name == "u32") return UnsignedInteger(32);
        if (name == "u64") return UnsignedInteger(64);
        if (name == "isize") return SignedInteger(64);
        if (name == "usize") return UnsignedInteger(64);
        if (name == "num" || name == "f64") return Float(64);
        if (name == "f32") return Float(32);
        if (name == "textu" || name == "char*") return Text();
        return Named(name);
    }

    TypeKind kind() const { return Kind; }
    bool valid() const { return Kind != TypeKind::Invalid; }
    bool isArray() const { return Kind == TypeKind::FixedArray; }
    bool isReference() const { return Kind == TypeKind::Reference; }
    bool isVoid() const { return Kind == TypeKind::Void; }
    bool isInteger() const { return Kind == TypeKind::Integer; }
    bool isSignedInteger() const { return Kind == TypeKind::Integer && Signed; }
    bool isUnsignedInteger() const { return Kind == TypeKind::Integer && !Signed; }
    bool isFloat() const { return Kind == TypeKind::Float; }
    bool isNumeric() const { return Kind == TypeKind::Integer || Kind == TypeKind::Float; }
    bool isNamed() const { return Kind == TypeKind::Named; }
    bool isSigned() const { return Signed; }
    unsigned bitWidth() const { return Bits; }

    const std::string& name() const { return Name; }

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
            case TypeKind::Integer:
                if (Signed && Bits == 64) return "nter";
                return std::string(Signed ? "i" : "u") + std::to_string(Bits);
            case TypeKind::Float:
                return Bits == 64 ? "num" : "f" + std::to_string(Bits);
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
        if (lhs.Kind != rhs.Kind || lhs.Name != rhs.Name || lhs.Size != rhs.Size
                || lhs.Signed != rhs.Signed || lhs.Bits != rhs.Bits)
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
    bool Signed = true;
    unsigned Bits = 0;
};

} // namespace kriol

#endif // _KRIOL_TYPE_HEADER
