#include "include/kriol/type.hh"

#include <cassert>

int main() {
    using kriol::Type;

    assert(!Type::Invalid().valid());
    assert(Type::Integer() == Type::Integer());
    assert(Type::Integer() != Type::Number());
    assert(Type::FromName("textu") == Type::Text());
    assert(Type::FromName("Mesa") == Type::Named("Mesa"));

    Type row = Type::FixedArray(Type::Integer(), 4);
    Type matrix = Type::FixedArray(row, 3);
    assert(row.isArray());
    assert(row.arraySize() == 4);
    assert(row.elementType() == Type::Integer());
    assert(matrix.elementType() == row);
    assert(matrix.str() == "nter[4][3]");

    Type reference = Type::Reference(Type::Named("Mesa"));
    assert(reference.isReference());
    assert(reference.elementType() == Type::Named("Mesa"));
    assert(reference.str() == "ref Mesa");
}
