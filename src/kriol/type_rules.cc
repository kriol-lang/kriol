#include "../../include/kriol/type_rules.hh"

#include <algorithm>
#include <string>

using namespace kriol::ast;

namespace kriol {
namespace typerules {

Type promotedNumericType(const Type& lhs, const Type& rhs) {
    if (!lhs.valid()) return rhs;
    if (!rhs.valid()) return lhs;
    if (lhs.isFloat() || rhs.isFloat()) {
        unsigned bits = std::max(lhs.isFloat() ? lhs.bitWidth() : 0,
                                 rhs.isFloat() ? rhs.bitWidth() : 0);
        return Type::Float(bits == 32 ? 32 : 64);
    }
    if (lhs.isInteger() && rhs.isInteger()) {
        unsigned bits = std::max(lhs.bitWidth(), rhs.bitWidth());
        bool isSigned = lhs.isSigned() || rhs.isSigned();
        if (lhs.isSigned() != rhs.isSigned())
            bits = std::max(bits, std::min(64u, bits * 2));
        return isSigned ? Type::SignedInteger(bits) : Type::UnsignedInteger(bits);
    }
    return lhs.valid() ? lhs : rhs;
}

const LiteralExpr* integerLiteralExpr(const Expr* expr, bool& negative) {
    if (!expr) return nullptr;
    if (auto* par = dynamic_cast<const ParExpr*>(expr))
        return integerLiteralExpr(par->Content.get(), negative);
    if (auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        if (unary->Op != "-") return nullptr;
        negative = !negative;
        return integerLiteralExpr(unary->Operand.get(), negative);
    }
    auto* lit = dynamic_cast<const LiteralExpr*>(expr);
    return lit && lit->Type.isInteger() ? lit : nullptr;
}

const LiteralExpr* underlyingNumericLiteral(const Expr* expr) {
    if (!expr) return nullptr;
    if (auto* par = dynamic_cast<const ParExpr*>(expr))
        return underlyingNumericLiteral(par->Content.get());
    if (auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        if (unary->Op != "-") return nullptr;
        return underlyingNumericLiteral(unary->Operand.get());
    }
    auto* lit = dynamic_cast<const LiteralExpr*>(expr);
    return lit && lit->Type.isNumeric() ? lit : nullptr;
}

bool integerLiteralFitsType(const Expr* expr, const Type& to) {
    if (!to.isInteger()) return false;

    bool negative = false;
    auto* lit = integerLiteralExpr(expr, negative);
    if (!lit) return false;

    long long value = 0;
    try {
        value = std::stoll(lit->Value);
    } catch (...) {
        return false;
    }
    if (negative) value = -value;

    const unsigned bits = to.bitWidth();
    if (bits == 0 || bits > 64) return false;

    if (to.isSigned()) {
        if (bits == 64) return true;
        const long long min = -(1LL << (bits - 1));
        const long long max = (1LL << (bits - 1)) - 1;
        return value >= min && value <= max;
    }

    if (value < 0) return false;
    if (bits == 64) return true;
    const unsigned long long max = (1ULL << bits) - 1;
    return static_cast<unsigned long long>(value) <= max;
}

Type promotedNumericTypeForExpr(const Expr* lhsExpr,
                                       const Expr* rhsExpr,
                                       const Type& lhs,
                                       const Type& rhs) {
    if (lhs.isInteger() && rhs.isInteger()) {
        bool ignored = false;
        const bool lhsLiteral = integerLiteralExpr(lhsExpr, ignored) != nullptr;
        ignored = false;
        const bool rhsLiteral = integerLiteralExpr(rhsExpr, ignored) != nullptr;

        if (lhsLiteral && !rhsLiteral && integerLiteralFitsType(lhsExpr, rhs))
            return rhs;
        if (rhsLiteral && !lhsLiteral && integerLiteralFitsType(rhsExpr, lhs))
            return lhs;
    }

    return promotedNumericType(lhs, rhs);
}

} // namespace typerules
} // namespace kriol
