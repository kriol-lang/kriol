#include "../../include/kriol/sema.hh"
#include "../../include/kriol/type_utils.hh"

#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <cctype>

using namespace kriol::ast;

namespace kriol {
namespace sema {

namespace {

using kriol::typeutils::arrayElementType;
using kriol::typeutils::firstArrayDim;
using kriol::typeutils::isArrayType;

static bool startsWithUppercaseAscii(const std::string& name) {
    return !name.empty() && std::isupper(static_cast<unsigned char>(name[0]));
}

static Type promotedNumericType(const Type& lhs, const Type& rhs) {
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

}

bool SemanticAnalyzer::handleArrayIdentArg(ast::Expr& expr) {
    auto* ident = unwrapIdentExpr(&expr);
    if (!ident) return false;

    auto t = lookupVar(ident->Name);
    if (!t) {
        addError(errLoc(ident->LineNum) + "undefined variable name '" + ident->Name + "'");
        return true;
    }

    if (!isArrayType(*t)) return false;

    auto* init = lookupInitState(ident->Name);
    if (init && init->isArray && !init->fullyInitialized)
        addError(errLoc(ident->LineNum) + "array '" + ident->Name + "' may contain uninitialized elements");

    ident->ResolvedType = *t;
    expr.ResolvedType = *t;
    return true;
}

static const std::unordered_set<std::string> reservedKeywords = {
    // Built-in/runtime statements and call forms.
    "mostra", "mostran", "sai", "konfirma",

    // Language keywords.
    "si", "sinon", "nkuantu", "pa", "fn", "molda", "divolvi", "inpristan",
    "para", "kontinua", "dipoz",

    // Type names and literals.
    "num", "nter", "bool", "textu", "sin", "nau",
    "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
    "f32", "f64", "isize", "usize"
};

bool SemanticAnalyzer::isReservedKeyword(const std::string& name) {
    return reservedKeywords.count(name) != 0;
}

bool SemanticAnalyzer::checkDeclaredNameValid(const std::string& name,
                                            const std::string& kind,
                                            int lineNum) {
    if (!isReservedKeyword(name)) return true;

    addError(errLoc(lineNum) + kind + " name '" + name
             + "' is reserved and cannot be redeclared");

    return false;
}

bool SemanticAnalyzer::isWideningCoercion(const Type& from, const Type& to) {
    if (from == to) return true;
    if (from == Type::Bool() && (to.isInteger() || to.isFloat())) return true;
    if (from.isInteger() && to.isInteger()) {
        if (from.isSigned() == to.isSigned())
            return from.bitWidth() <= to.bitWidth();
        if (!from.isSigned() && to.isSigned())
            return from.bitWidth() < to.bitWidth();
        return false;
    }
    if (from.isFloat() && to.isFloat())
        return from.bitWidth() <= to.bitWidth();
    if (from.isInteger() && to.isFloat())
        return true;
    return false;
}

bool SemanticAnalyzer::integerLiteralFits(const ast::Expr* expr, const Type& to) {
    if (!to.isInteger()) return false;

    auto* lit = dynamic_cast<const LiteralExpr*>(expr);
    if (!lit || !lit->Type.isInteger()) return false;

    long long value = 0;
    try {
        value = std::stoll(lit->Value);
    } catch (...) {
        return false;
    }

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

bool SemanticAnalyzer::canCoerceExprTo(const ast::Expr* expr, const Type& to) {
    if (!expr) return false;
    const Type& from = expr->ResolvedType;
    auto* lit = dynamic_cast<const LiteralExpr*>(expr);
    if (lit && from.isFloat() && to.isFloat()) return true;
    return from == to || isWideningCoercion(from, to) || integerLiteralFits(expr, to);
}

bool SemanticAnalyzer::isPrintableType(const Type& type, bool allowArray) {
    if (type.isInteger() || type.isFloat() || type == Type::Bool() || type == Type::Text())
        return true;

    if (allowArray && type.isArray())
        return isPrintableType(type.elementType(), true);

    return false;
}

void SemanticAnalyzer::registerFuncSignature(FuncDeclSttmt& node) {
    if (!checkDeclaredNameValid(node.Name, "function", node.LineNum)) return;

    if (FunctionTable.count(node.Name)) {
        addError(errLoc(node.LineNum) + "duplicate function declaration '" + node.Name + "'");
        return;
    }

    FuncInfo info;
    info.retType = node.Type;
    validateTypeKnown(node.Type, node.LineNum, "return type of function '" + node.Name + "'");
    if (node.Args)
        for (auto& arg : node.Args->Args) {
            if (!arg) continue;
            validateTypeKnown(arg->Type, arg->LineNum, "parameter '" + arg->Name + "'");
            info.paramTypes.push_back(arg->Type);
        }
    FunctionTable[node.Name] = std::move(info);
}

void SemanticAnalyzer::registerRecord(MoldaDeclSttmt& node) {
    if (!checkDeclaredNameValid(node.Name, "molda", node.LineNum)) return;

    if (!startsWithUppercaseAscii(node.Name)) {
        addError(errLoc(node.LineNum) + "molda type name '" + node.Name
                 + "' must start with an uppercase letter");
        return;
    }

    if (RecordTable.count(node.Name)) {
        addError(errLoc(node.LineNum) + "duplicate molda declaration '" + node.Name + "'");
        return;
    }

    if (FunctionTable.count(node.Name))
        addError(errLoc(node.LineNum) + "molda name '" + node.Name + "' conflicts with an existing function");

    if (node.Fields.empty())
        addError(errLoc(node.LineNum) + "molda '" + node.Name + "' must declare at least one field");

    RecordInfo info;
    std::unordered_set<std::string> seenFields;
    for (auto& field : node.Fields) {
        if (!field) continue;

        if (!checkDeclaredNameValid(field->Name, "field", field->LineNum))
            continue;

        if (!seenFields.insert(field->Name).second) {
            addError(errLoc(field->LineNum) + "duplicate field '" + field->Name
                     + "' in molda '" + node.Name + "'");
            continue;
        }

        validateTypeKnown(field->Type, field->LineNum, "field '" + field->Name + "'");
        info.fieldIndex[field->Name] = info.fields.size();
        info.fields.push_back(field.get());
    }

    RecordTable[node.Name] = std::move(info);
}

bool SemanticAnalyzer::validateTypeKnown(const Type& type,
                                         int lineNum,
                                         const std::string& context) {
    if (!type.valid()) return false;
    if (type.isArray())
        return validateTypeKnown(type.elementType(), lineNum, context);
    if (!type.isNamed()) return true;
    if (RecordTable.count(type.name())) return true;

    addError(errLoc(lineNum) + "unknown type '" + type.str() + "' in " + context);
    return false;
}

bool SemanticAnalyzer::validateArrayInitializer(const Type& expectedType,
                                                ast::Expr* init,
                                                int lineNum,
                                                const std::string& context) {
    if (!isArrayType(expectedType)) {
        addError(errLoc(lineNum) + context + " requires a non-array value; use an explicit '<T>[...]' array literal only for array targets");
        return false;
    }

    auto* initLit = dynamic_cast<ArrayLiteralExpr*>(init);
    auto* initRep = dynamic_cast<ArrayRepeatExpr*>(init);
    if (!initLit && !initRep) return false;

    const std::size_t expectedSize = expectedType.arraySize();
    const Type elemType = arrayElementType(expectedType);

    if (initRep) {
        initRep->accept(*this);
        if (initRep->Count != expectedSize) {
            addError(errLoc(lineNum) + context + " has size "
                     + std::to_string(expectedSize) + " but repeat initializer [value] * "
                     + std::to_string(initRep->Count) + " has a different count");
            return false;
        }

        const Type fillType = initRep->Fill ? initRep->Fill->ResolvedType : Type::Invalid();
        if (fillType.valid() && !canCoerceExprTo(initRep->Fill.get(), elemType)) {
            addError(errLoc(lineNum) + context + " repeat initializer fill type '"
                     + fillType.str() + "' does not match array element type '"
                     + elemType.str() + "'");
            return false;
        }

        initRep->ResolvedType = expectedType;
        return true;
    }

    initLit->accept(*this);
    if (initLit->ExplicitElementType.valid()
            && initLit->ExplicitElementType != elemType
            && !isWideningCoercion(initLit->ExplicitElementType, elemType)) {
        addError(errLoc(lineNum) + context + " expects array element type '"
                 + elemType.str() + "', got explicit array literal element type '"
                 + initLit->ExplicitElementType.str() + "'");
        return false;
    }

    const std::size_t got = initLit->Elements.size();
    if (got != expectedSize) {
        addError(errLoc(lineNum) + context + " expects "
                 + std::to_string(expectedSize) + " initializer element(s), got "
                 + std::to_string(got));
        return false;
    }

    bool ok = true;
    for (std::size_t i = 0; i < initLit->Elements.size(); ++i) {
        const auto& gotType = initLit->Elements[i]->ResolvedType;
        if (gotType.valid() && !canCoerceExprTo(initLit->Elements[i].get(), elemType)) {
            addError(errLoc(lineNum) + context + " initializer element "
                     + std::to_string(i + 1) + ": expected '" + elemType.str()
                     + "', got '" + gotType.str() + "'");
            ok = false;
        }
    }

    initLit->ResolvedType = expectedType;
    return ok;
}

void SemanticAnalyzer::Check(BlockSttmt* program) {
    if (!program) return;

    for (auto& s : program->SttmtList) {
        if (auto* rec = dynamic_cast<MoldaDeclSttmt*>(s.get()))
            registerRecord(*rec);
    }

    // First pass: register all top-level function signatures so forward calls
    // and mutual recursion are resolved before any body is visited.
    for (auto& s : program->SttmtList) {
        if (auto* fn = dynamic_cast<FuncDeclSttmt*>(s.get()))
            registerFuncSignature(*fn);
    }

    // Second pass: full semantic walk.
    pushScope();
    for (auto& s : program->SttmtList)
        if (s) s->accept(*this);
    popScope();
}


bool SemanticAnalyzer::blockDefinitelyReturns(BlockSttmt* block) const {
    if (!block) return false;
    for (auto& stmt : block->SttmtList) {
        if (!stmt) continue;

        // Direct return
        if (dynamic_cast<ReturnSttmt*>(stmt.get()))
            return true;

        // Nested braced block that itself always returns
        if (auto* nested = dynamic_cast<BlockSttmt*>(stmt.get()))
            if (blockDefinitelyReturns(nested)) return true;

        // if/else where both branches always return
        if (auto* ifStmt = dynamic_cast<IfSttmt*>(stmt.get())) {
            if (ifStmt->Else &&
                blockDefinitelyReturns(ifStmt->Then.get()) &&
                blockDefinitelyReturns(ifStmt->Else.get()))
                return true;
        }
    }
    return false;
}


void SemanticAnalyzer::visit(VarDeclSttmt& node) {
    const std::string kind = node.IsParam ? "parameter" : "variable";
    bool canDeclare = checkDeclaredNameValid(node.Name, kind, node.LineNum);
    VarInitState initState;
    initState.isArray = node.IsArray;

    if (node.IsArray && node.ArraySize == 0) {
        addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must have a positive size");
        canDeclare = false;
    }

    if (!validateTypeKnown(node.Type, node.LineNum, kind + " '" + node.Name + "'"))
        canDeclare = false;

    if (node.IsArray && node.Value) {
        auto* initLit = dynamic_cast<ArrayLiteralExpr*>(node.Value.get());
        auto* initRep = dynamic_cast<ArrayRepeatExpr*>(node.Value.get());
        if (!initLit && !initRep) {
            addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must use an array initializer like [a, b, c] or [value] * N");
            canDeclare = false;
        } else {
            initState.elementInitialized.assign(node.ArraySize, false);
            if (!validateArrayInitializer(node.Type, node.Value.get(), node.LineNum,
                                          "array variable '" + node.Name + "'")) {
                canDeclare = false;
            }
            std::fill(initState.elementInitialized.begin(), initState.elementInitialized.end(), true);
            initState.fullyInitialized = canDeclare;
        }
    } else if (node.IsArray) {
        initState.elementInitialized.assign(node.ArraySize, false);
        initState.fullyInitialized = false;
    } else {
        initState.fullyInitialized = node.IsParam || (node.Value != nullptr);
    }

    // Check for duplicate in the innermost scope only.
    if (!SymbolScopes.empty()) {
        auto& cur = SymbolScopes.back();
        if (cur.count(node.Name)) {
            addError(errLoc(node.LineNum) + kind + " '" + node.Name + "' already declared in this scope");
            canDeclare = false;
        }
    }

    if (canDeclare)
        declareVar(node.Name, node.Type);

    if (canDeclare)
        declareInitState(node.Name, initState);

    if (node.Value && !node.IsArray) {
        if (dynamic_cast<ArrayLiteralExpr*>(node.Value.get()) || dynamic_cast<ArrayRepeatExpr*>(node.Value.get())) {
            addError(errLoc(node.LineNum) + kind + " '" + node.Name
                     + "' uses an array initializer without an array target; declare an array type or use an explicit '<T>[...]' array literal where an array value is expected");
            return;
        }

        node.Value->accept(*this);
        if (node.Value->ResolvedType.isVoid())
            addError(errLoc(node.LineNum) + "cannot assign void expression to variable '" + node.Name + "'");
        if (!node.IsArray && node.Value->ResolvedType.valid()
                && !canCoerceExprTo(node.Value.get(), node.Type))
            addError(errLoc(node.LineNum) + "cannot assign value of type '"
                     + node.Value->ResolvedType.str() + "' to variable '" + node.Name
                     + "' of type '" + node.Type.str() + "'");
    }
}

void SemanticAnalyzer::visit(MoldaDeclSttmt&) {
    // Molda declarations are registered before the semantic statement walk.
    // They define types but do not introduce runtime values.
}

void SemanticAnalyzer::visit(BlockSttmt& node) {
    // A braced compound_statement introduces a new scope; a bare block
    // (program root, function body) uses the scope pushed by the caller.
    if (node.BracketsOn) pushScope();
    for (auto& s : node.SttmtList)
        if (s) s->accept(*this);
    if (node.BracketsOn) popScope();
}

void SemanticAnalyzer::visit(FuncArgs& node) {
    // Handled inside FuncDeclSttmt
}

void SemanticAnalyzer::visit(FuncDeclSttmt& node) {
    // If the signature wasn't pre-registered (nested function), register it now.
    // Top-level functions are already in FunctionTable from the first pass.
    if (!FunctionTable.count(node.Name)) {
        // Top-level functions were handled in the pre-registration pass.
        if (SymbolScopes.size() > 1)
            registerFuncSignature(node);
    }

    Type savedRetType = CurrFuncRetType;
    std::string savedName    = CurrFuncName;
    CurrFuncRetType = node.Type;
    CurrFuncName    = node.Name;

    pushScope();

    // Add parameters to function scope
    if (node.Args) {
        for (auto& arg : node.Args->Args) {
            if (arg) arg->accept(*this);
        }
    }

    if (node.Body) node.Body->accept(*this);

    // Check that non-void, non-entry functions return on all paths
    bool isEntry = (node.Name == "inisiu");
    bool isVoid  = node.Type.isVoid();
    if (!isEntry && !isVoid && !blockDefinitelyReturns(node.Body.get()))
        addError(errLoc(node.LineNum) + "function '" + node.Name + "' does not return on all paths");

    popScope();
    CurrFuncRetType = savedRetType;
    CurrFuncName    = savedName;
}

void SemanticAnalyzer::visit(IfSttmt& node) {
    if (node.Cond) {
        node.Cond->accept(*this);
        if (node.Cond->ResolvedType.isVoid())
            addError(errLoc(node.LineNum) + "void expression cannot be used as a condition");
    }
    if (node.Then) node.Then->accept(*this);
    if (node.Else) node.Else->accept(*this);
}

void SemanticAnalyzer::visit(WhileSttmt& node) {
    if (node.Cond) {
        node.Cond->accept(*this);
        if (node.Cond->ResolvedType.isVoid())
            addError(errLoc(node.LineNum) + "void expression cannot be used as a condition");
    }
    ++LoopDepth;
    if (node.Do) node.Do->accept(*this);
    --LoopDepth;
}

void SemanticAnalyzer::visit(JumpSttmt& node) {
    // ReturnSttmt overrides this, so here we only see break/continue
    if (LoopDepth == 0)
        addError(errLoc(node.LineNum) + "'" + node.Name + "' used outside a loop");
}

void SemanticAnalyzer::visit(ReturnSttmt& node) {
    const std::string loc = errLoc(node.LineNum);

    if (CurrFuncRetType.valid() && CurrFuncRetType.isVoid() && node.ReturnValue)
        addError(loc + "returning a value from void function '" + CurrFuncName + "'");

    if (CurrFuncRetType.valid() && !CurrFuncRetType.isVoid() && !node.ReturnValue)
        addError(loc + "missing return value in non-void function '" + CurrFuncName + "'");

    if (node.ReturnValue) {
        node.ReturnValue->accept(*this);
        const Type& got = node.ReturnValue->ResolvedType;
        if (got.valid() && CurrFuncRetType.valid()
                && !CurrFuncRetType.isVoid()
                && !canCoerceExprTo(node.ReturnValue.get(), CurrFuncRetType))
            addError(loc + "returning '" + got.str() + "' from function '" + CurrFuncName
                     + "' declared as '" + CurrFuncRetType.str() + "'");
    }
}

void SemanticAnalyzer::visit(FuncCallArgs& node) {
    // Handled inside FunCallExpr
}

void SemanticAnalyzer::visit(FunCallExpr& node) {
    if (node.Args)
        for (auto& arg : node.Args->Args)
            if (arg) arg->accept(*this);

    auto* callee = unwrapIdentExpr(node.Callee.get());
    if (!callee) {
        if (node.Callee) node.Callee->accept(*this);
        addError(errLoc(node.LineNum) + "expression is not callable");
        return;
    }

    auto it = FunctionTable.find(callee->Name);
    if (it == FunctionTable.end()) {
        addError(errLoc(node.LineNum) + "undeclared function '" + callee->Name + "'");
        return;
    }

    const FuncInfo& info = it->second;
    node.ResolvedType = info.retType;

    const std::string loc = errLoc(node.LineNum);
    size_t got  = node.Args ? node.Args->Args.size() : 0;
    size_t want = info.paramTypes.size();

    if (got != want) {
        addError(loc + "function '" + callee->Name + "' expects " + std::to_string(want)
                 + " argument(s), got " + std::to_string(got));
        return; // type checks make no sense if counts differ
    }

    if (node.Args) {
        for (size_t i = 0; i < node.Args->Args.size(); ++i) {
            const Type& argType   = node.Args->Args[i]->ResolvedType;
            const Type& paramType = info.paramTypes[i];
            if (argType.valid() && !canCoerceExprTo(node.Args->Args[i].get(), paramType))
                addError(loc + "argument " + std::to_string(i + 1) + " of '" + callee->Name
                         + "': expected '" + paramType.str() + "', got '" + argType.str() + "'");
        }
    }
}

void SemanticAnalyzer::visit(BinExpr& node) {
    if (node.LHS) node.LHS->accept(*this);
    if (node.RHS) node.RHS->accept(*this);

    const Type lt = node.LHS ? node.LHS->ResolvedType : Type::Invalid();
    const Type rt = node.RHS ? node.RHS->ResolvedType : Type::Invalid();

    if (lt.isVoid())
        addError(errLoc(node.LineNum) + "void expression cannot be used as an operand");
    if (rt.isVoid())
        addError(errLoc(node.LineNum) + "void expression cannot be used as an operand");

    static const std::unordered_set<std::string> equalityOps = {
        "==", "!="
    };
    static const std::unordered_set<std::string> relationalOps = {
        "<", "<=", ">", ">="
    };

    if (node.Op == "&&" || node.Op == "||") {
        if (lt.valid() && lt != Type::Bool())
            addError(errLoc(node.LineNum) + "logical operator '" + node.Op
                     + "' requires boolean operands, got '" + lt.str() + "'");
        if (rt.valid() && rt != Type::Bool())
            addError(errLoc(node.LineNum) + "logical operator '" + node.Op
                     + "' requires boolean operands, got '" + rt.str() + "'");
        node.ResolvedType = Type::Bool();
        return;
    }

    if (equalityOps.count(node.Op) && lt == Type::Bool() && rt == Type::Bool()) {
        node.ResolvedType = Type::Bool();
        return;
    }

    if (!lt.isNumeric() || !rt.isNumeric()) {
        if (lt.valid() && rt.valid())
            addError(errLoc(node.LineNum) + "binary operator '" + node.Op
                     + "' requires numeric operands, got '" + lt.str()
                     + "' and '" + rt.str() + "'");
        node.ResolvedType = (equalityOps.count(node.Op) || relationalOps.count(node.Op))
            ? Type::Bool()
            : Type::Invalid();
        return;
    }

    if (equalityOps.count(node.Op) || relationalOps.count(node.Op))
        node.ResolvedType = Type::Bool();
    else
        node.ResolvedType = promotedNumericType(lt, rt);
}

void SemanticAnalyzer::visit(LiteralExpr& node) {
    node.ResolvedType = node.Type;
}

void SemanticAnalyzer::visit(ExprSttmt& node) {
    if (node.Expression) {
        node.Expression->accept(*this);
        node.ResolvedType = node.Expression->ResolvedType;
    }
}

void SemanticAnalyzer::visit(IdentExpr& node) {
    // Only flag as undefined if we are inside a function scope (SymbolScopes
    // has at least 2 levels: top-level scope + function scope).
    if (SymbolScopes.size() >= 2) {
        if (!lookupVar(node.Name))
            addError(errLoc(node.LineNum) + "undefined variable name '" + node.Name + "'");
    }
    auto t = lookupVar(node.Name);
    if (t) {
        node.ResolvedType = *t;
        if (isArrayType(*t))
            addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must be indexed");

        auto* init = lookupInitState(node.Name);
        if (init && !init->isArray && !init->fullyInitialized)
            addError(errLoc(node.LineNum) + "use of uninitialized variable '" + node.Name + "'");
    }
}

void SemanticAnalyzer::visit(ParExpr& node) {
    if (node.Content) {
        node.Content->accept(*this);
        node.ResolvedType = node.Content->ResolvedType;
    }
}

void SemanticAnalyzer::visit(ArrayAccessExpr& node) {
    if (node.Index) {
        node.Index->accept(*this);
        const Type& indexType = node.Index->ResolvedType;
        if (indexType.valid() && !indexType.isInteger())
            addError(errLoc(node.LineNum) + "array index must be an integer");
    }

    auto* baseIdent = unwrapIdentExpr(node.Base.get());
    Type arrayType = Type::Invalid();
    if (baseIdent) {
        auto found = lookupVar(baseIdent->Name);
        if (!found) {
            addError(errLoc(node.LineNum) + "undefined array name '" + baseIdent->Name + "'");
            return;
        }
        arrayType = *found;
        baseIdent->ResolvedType = arrayType;
    } else if (node.Base) {
        node.Base->accept(*this);
        arrayType = node.Base->ResolvedType;
    }

    if (!isArrayType(arrayType)) {
        addError(errLoc(node.LineNum) + "indexed expression is not an array");
        return;
    }

    node.ResolvedType = arrayElementType(arrayType);

    auto firstDim = firstArrayDim(arrayType);
    if (!firstDim || *firstDim == 0) {
        addError(errLoc(node.LineNum) + "array has invalid size metadata");
    }

    if (!baseIdent) return;
    auto* init = lookupInitState(baseIdent->Name);
    if (!init || !init->isArray) return;
    if (init->fullyInitialized) return;

    auto* lit = dynamic_cast<LiteralExpr*>(node.Index.get());
    if (!lit || !lit->Type.isInteger()) {
        addError(errLoc(node.LineNum) + "array '" + baseIdent->Name + "' may contain uninitialized elements");
        return;
    }

    long long idx = 0;
    try {
        idx = std::stoll(lit->Value);
    } catch (...) {
        addError(errLoc(node.LineNum) + "array index for '" + baseIdent->Name + "' must be an integer literal");
        return;
    }

    if (idx < 0 || static_cast<std::size_t>(idx) >= init->elementInitialized.size()) {
        // Bounds diagnostics are handled elsewhere.
        return;
    }

    if (!init->elementInitialized[static_cast<std::size_t>(idx)])
        addError(errLoc(node.LineNum) + "array element '" + baseIdent->Name + "[" + std::to_string(idx) + "]' is not initialized");
}

void SemanticAnalyzer::visit(MemberAccessExpr& node) {
    if (node.Base) node.Base->accept(*this);
    const Type baseType = node.Base ? node.Base->ResolvedType : Type::Invalid();
    if (!baseType.valid()) return;

    if (!baseType.isNamed()) {
        addError(errLoc(node.LineNum) + "member access requires a molda value");
        return;
    }

    auto recordIt = RecordTable.find(baseType.name());
    if (recordIt == RecordTable.end()) {
        addError(errLoc(node.LineNum) + "unknown molda type '" + baseType.str() + "'");
        return;
    }

    const auto& fields = recordIt->second;
    auto fieldIt = fields.fieldIndex.find(node.Member);
    if (fieldIt == fields.fieldIndex.end()) {
        addError(errLoc(node.LineNum) + "molda '" + baseType.str()
                 + "' has no field '" + node.Member + "'");
        return;
    }

    node.ResolvedType = fields.fields[fieldIt->second]->Type;
}

void SemanticAnalyzer::visit(QualifiedAccessExpr& node) {
    addError(errLoc(node.LineNum) + "qualified item access is not supported yet");
}

void SemanticAnalyzer::visit(ArrayLiteralExpr& node) {
    for (auto& element : node.Elements) {
        if (element) element->accept(*this);
    }

    if (!node.ExplicitElementType.valid()) {
        node.ResolvedType = Type::ArrayLiteral();
        return;
    }

    validateTypeKnown(node.ExplicitElementType, node.LineNum, "array literal element type");
    for (std::size_t i = 0; i < node.Elements.size(); ++i) {
        const Type& got = node.Elements[i]->ResolvedType;
        if (got.valid() && !canCoerceExprTo(node.Elements[i].get(), node.ExplicitElementType)) {
            addError(errLoc(node.LineNum) + "array literal element "
                     + std::to_string(i + 1) + ": expected '"
                     + node.ExplicitElementType.str() + "', got '"
                     + got.str() + "'");
        }
    }
    node.ResolvedType = Type::FixedArray(node.ExplicitElementType, node.Elements.size());
}

void SemanticAnalyzer::visit(ArrayRepeatExpr& node) {
    if (node.Fill) node.Fill->accept(*this);
    node.ResolvedType = Type::ArrayRepeat();
}

void SemanticAnalyzer::visit(RecordLiteralExpr& node) {
    auto recordIt = RecordTable.find(node.TypeName);
    if (recordIt == RecordTable.end()) {
        addError(errLoc(node.LineNum) + "unknown molda type '" + node.TypeName + "'");
        return;
    }

    const RecordInfo& info = recordIt->second;
    std::unordered_set<std::string> seen;
    std::vector<bool> initialized(info.fields.size(), false);

    for (auto& field : node.Fields) {
        if (!seen.insert(field.Name).second) {
            addError(errLoc(node.LineNum) + "duplicate field '" + field.Name
                     + "' in '" + node.TypeName + "' literal");
            continue;
        }

        auto indexIt = info.fieldIndex.find(field.Name);
        if (indexIt == info.fieldIndex.end()) {
            addError(errLoc(node.LineNum) + "molda '" + node.TypeName
                     + "' has no field '" + field.Name + "'");
            if (field.Value) field.Value->accept(*this);
            continue;
        }

        const Type& want = info.fields[indexIt->second]->Type;
        if (field.Value) {
            const bool isArrayInit = dynamic_cast<ArrayLiteralExpr*>(field.Value.get())
                || dynamic_cast<ArrayRepeatExpr*>(field.Value.get());
            if (isArrayInit) {
                validateArrayInitializer(want, field.Value.get(), node.LineNum,
                                         "field '" + field.Name + "' of '" + node.TypeName + "'");
            } else {
                field.Value->accept(*this);
            }
        }

        const Type& got = field.Value ? field.Value->ResolvedType : Type::Invalid();
        if (got.valid() && !canCoerceExprTo(field.Value.get(), want))
            addError(errLoc(node.LineNum) + "field '" + field.Name + "' of '"
                     + node.TypeName + "': expected '" + want.str()
                     + "', got '" + got.str() + "'");
        initialized[indexIt->second] = true;
    }

    for (std::size_t i = 0; i < info.fields.size(); ++i) {
        if (!initialized[i])
            addError(errLoc(node.LineNum) + "missing field '" + info.fields[i]->Name
                     + "' in '" + node.TypeName + "' literal");
    }

    node.ResolvedType = Type::Named(node.TypeName);
}

Type SemanticAnalyzer::resolveAssignableType(ast::Expr* expr, int lineNum) {
    if (!expr) return Type::Invalid();

    if (auto* ident = dynamic_cast<IdentExpr*>(expr)) {
        auto t = lookupVar(ident->Name);
        if (!t) {
            addError(errLoc(lineNum) + "undefined variable name '" + ident->Name + "'");
            return Type::Invalid();
        }
        ident->ResolvedType = *t;
        expr->ResolvedType = *t;
        return *t;
    }

    if (auto* arr = dynamic_cast<ArrayAccessExpr*>(expr)) {
        Type baseType = resolveAssignableType(arr->Base.get(), lineNum);

        if (arr->Index) {
            arr->Index->accept(*this);
            if (arr->Index->ResolvedType.valid() && !arr->Index->ResolvedType.isInteger())
                addError(errLoc(lineNum) + "array index must be an integer");
        }

        if (!isArrayType(baseType)) {
            addError(errLoc(lineNum) + "indexed assignment target is not an array");
            return Type::Invalid();
        }

        arr->ResolvedType = arrayElementType(baseType);
        return arr->ResolvedType;
    }

    if (auto* member = dynamic_cast<MemberAccessExpr*>(expr)) {
        Type baseType = resolveAssignableType(member->Base.get(), lineNum);
        if (!baseType.valid()) return Type::Invalid();

        if (!baseType.isNamed()) {
            addError(errLoc(lineNum) + "member assignment target requires a molda value");
            return Type::Invalid();
        }

        auto recordIt = RecordTable.find(baseType.name());
        if (recordIt == RecordTable.end()) {
            addError(errLoc(lineNum) + "unknown molda type '" + baseType.str() + "'");
            return Type::Invalid();
        }

        auto fieldIt = recordIt->second.fieldIndex.find(member->Member);
        if (fieldIt == recordIt->second.fieldIndex.end()) {
            addError(errLoc(lineNum) + "molda '" + baseType.str()
                     + "' has no field '" + member->Member + "'");
            return Type::Invalid();
        }

        member->ResolvedType = recordIt->second.fields[fieldIt->second]->Type;
        return member->ResolvedType;
    }

    expr->accept(*this);
    addError(errLoc(lineNum) + "invalid assignment target");
    return Type::Invalid();
}

void SemanticAnalyzer::visit(AssignExpr& node) {
    Type assigneeType = resolveAssignableType(node.Assignee.get(), node.LineNum);
    bool canMarkInitialized = true;

    if (!assigneeType.valid()) {
        canMarkInitialized = false;
    } else if (isArrayType(assigneeType) && !dynamic_cast<ArrayAccessExpr*>(node.Assignee.get())) {
        if (auto* ident = dynamic_cast<IdentExpr*>(node.Assignee.get())) {
            addError(errLoc(node.LineNum) + "cannot assign directly to array variable '"
                     + ident->Name + "'; assign to an index");
        } else {
            addError(errLoc(node.LineNum) + "cannot assign directly to array value; assign to an index");
        }
            canMarkInitialized = false;
    }

    if (node.Assigned) {
        node.Assigned->accept(*this);
        node.ResolvedType = node.Assigned->ResolvedType;

        if (node.AssignOp != "=" && assigneeType.valid()) {
            if (!assigneeType.isNumeric()) {
                addError(errLoc(node.LineNum) + "compound assignment operator '"
                         + node.AssignOp + "' requires a numeric target, got '"
                         + assigneeType.str() + "'");
                canMarkInitialized = false;
            }

            if (node.Assigned->ResolvedType.valid() && !node.Assigned->ResolvedType.isNumeric()) {
                addError(errLoc(node.LineNum) + "compound assignment operator '"
                         + node.AssignOp + "' requires a numeric value, got '"
                         + node.Assigned->ResolvedType.str() + "'");
                canMarkInitialized = false;
            }
        }

        if (assigneeType.valid() && node.Assigned->ResolvedType.valid()
                && !canCoerceExprTo(node.Assigned.get(), assigneeType)) {
            addError(errLoc(node.LineNum) + "cannot assign value of type '" + node.Assigned->ResolvedType.str()
                     + "' to target of type '" + assigneeType.str() + "'");
            canMarkInitialized = false;
        }
    }

    if (!canMarkInitialized) return;

    if (auto* ident = dynamic_cast<IdentExpr*>(node.Assignee.get())) {
        auto* init = lookupInitStateMutable(ident->Name);
        if (init && !init->isArray) init->fullyInitialized = true;
    } else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(node.Assignee.get())) {
        auto* baseIdent = unwrapIdentExpr(arr->Base.get());
        auto* init = baseIdent ? lookupInitStateMutable(baseIdent->Name) : nullptr;
        if (init && init->isArray && !init->fullyInitialized) {
            auto* lit = dynamic_cast<LiteralExpr*>(arr->Index.get());
            if (lit && lit->Type.isInteger()) {
                try {
                    long long idx = std::stoll(lit->Value);
                    if (idx >= 0 && static_cast<std::size_t>(idx) < init->elementInitialized.size()) {
                        init->elementInitialized[static_cast<std::size_t>(idx)] = true;
                        init->fullyInitialized = std::all_of(init->elementInitialized.begin(),
                                                             init->elementInitialized.end(),
                                                             [](bool v) { return v; });
                    }
                } catch (...) {
                    // NOTE: Non-integer literal is validated elsewhere.
                }
            }
        }
    }
}

void SemanticAnalyzer::visit(ForSttmt& node) {
    if (node.Start) node.Start->accept(*this);
    if (node.Cond) {
        node.Cond->accept(*this);
        if (node.Cond->ResolvedType.isVoid())
            addError(errLoc(node.LineNum) + "void expression cannot be used as a condition");
    }
    ++LoopDepth;
    if (node.Then)  node.Then->accept(*this);
    --LoopDepth;
    if (node.After) node.After->accept(*this);
}

void SemanticAnalyzer::visit(MostraFunCallExpr& node) {
    if (node.Args)
        for (auto& arg : node.Args->Args) {
            if (!arg) continue;
            if (!handleArrayIdentArg(*arg))
                arg->accept(*this);
            const Type& t = arg->ResolvedType;
            if (t.valid() && !isPrintableType(t, true))
                addError(errLoc(node.LineNum) + "cannot print value of type '" + t.str() + "'");
        }
    node.ResolvedType = Type::Void();
}

void SemanticAnalyzer::visit(ImportSttmt& node) {
    addError(errLoc(node.LineNum) + "'inpristan' (import) statements are not supported yet");
}

void SemanticAnalyzer::visit(FStringExpr& node) {
    for (auto& seg : node.Parts) {
        if (!seg.expr) continue;
        if (handleArrayIdentArg(*seg.expr)) {
            const Type& t = seg.expr->ResolvedType;
            if (t.valid() && !isPrintableType(t, true))
                addError(errLoc(node.LineNum) + "f-string interpolation: cannot format value of type '" + t.str() + "'");
        } else {
            seg.expr->accept(*this);
            const Type& t = seg.expr->ResolvedType;
            if (t.valid() && !isPrintableType(t, true))
                addError(errLoc(node.LineNum) + "f-string interpolation: cannot format value of type '" + t.str() + "'");
        }
    }
    node.ResolvedType = Type::Text();
}

void SemanticAnalyzer::visit(UnaryExpr& node) {
    if (node.Operand) node.Operand->accept(*this);
    if (node.Op == "!")
        node.ResolvedType = Type::Bool();
    else // "-" (numeric negation) keeps operand type, default to num if unknown
        node.ResolvedType = node.Operand ? node.Operand->ResolvedType : Type::Number();
}

void SemanticAnalyzer::visit(SaiSttmt& node) {
    if (node.Code) {
        node.Code->accept(*this);
        const Type& t = node.Code->ResolvedType;
        if (!t.isInteger()) {
            std::string err = "sai() expects an integer exit code";
            if (t.valid()) err += ", got value of type '" + t.str() + "'";
            addError(errLoc(node.LineNum) + err);
        }
    }
}

void SemanticAnalyzer::visit(KonfirmaSttmt& node) {
    if (node.Cond) {
        node.Cond->accept(*this);
        const Type& t = node.Cond->ResolvedType;
        if (t != Type::Bool() && !t.isInteger() && !t.isFloat()) {
            std::string err = "konfirma() expects a boolean condition";
            if (t.valid()) err += ", got value of type '" + t.str() + "'";
            addError(errLoc(node.LineNum) + err);
        }
    };
}

} // namespace sema
} // namespace kriol
