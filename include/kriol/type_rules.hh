#ifndef KRIOL_TYPE_RULES_HEADER
#define KRIOL_TYPE_RULES_HEADER

#include "ast.hh"

namespace kriol {
namespace typerules {

// Numeric typing rules shared by sema and codegen.

// The type both operands of a binary operator are converted to.
Type promotedNumericType(const Type& lhs, const Type& rhs);

// The type of `lhs / rhs`: division never truncates, so two integers divide
// as f64; otherwise the promoted floating-point type.
Type divisionResultType(const Type& lhs, const Type& rhs);

// Like promotedNumericType, but an integer literal operand adopts the other
// operand's type when it fits (so `u8 x; x + 1` stays u8).
Type promotedNumericTypeForExpr(const ast::Expr* lhsExpr,
                                const ast::Expr* rhsExpr,
                                const Type& lhs,
                                const Type& rhs);

// The integer literal under any parentheses and unary minus signs, or null;
// `negative` flips once per minus sign.
const ast::LiteralExpr* integerLiteralExpr(const ast::Expr* expr, bool& negative);

// The numeric literal under any parentheses and unary minus signs, or null.
const ast::LiteralExpr* underlyingNumericLiteral(const ast::Expr* expr);

// Whether `expr` is an integer literal whose value fits in the integer type `to`.
bool integerLiteralFitsType(const ast::Expr* expr, const Type& to);

} // namespace typerules
} // namespace kriol

#endif // KRIOL_TYPE_RULES_HEADER
