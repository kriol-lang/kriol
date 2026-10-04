#include "../../include/kriol/sema.hh"
#include "../../include/kriol/type_rules.hh"
#include "../../include/kriol/type_utils.hh"
#include "../../include/kriol/prelude.hh"

#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <utility>

using namespace kriol::ast;

namespace kriol {
namespace sema {

namespace {

using kriol::typeutils::arrayElementType;
using kriol::typeutils::firstArrayDim;
using kriol::typeutils::isArrayType;
using namespace kriol::typerules;

static bool startsWithUppercaseAscii(const std::string& name) {
    return !name.empty() && std::isupper(static_cast<unsigned char>(name[0]));
}

const std::string erruName = prelude::ErrorTypeName;

// Sets a member for the lifetime of the guard and restores it afterwards.
template <typename T>
class ScopedValue {
public:
    ScopedValue(T& target, T value) : Target(target), Saved(std::exchange(target, std::move(value))) {}
    ~ScopedValue() { Target = std::move(Saved); }
    ScopedValue(const ScopedValue&) = delete;
    ScopedValue& operator=(const ScopedValue&) = delete;

private:
    T& Target;
    T Saved;
};

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

    checkNotSelfInitialized(ident->Name, ident->LineNum);
    ident->ResolvedType = *t;
    expr.ResolvedType = *t;
    return true;
}

static const std::unordered_set<std::string> reservedKeywords = {
    // Language keywords.
    "si", "sinon", "nkuantu", "pa", "fn", "molda", "divolvi", "inpristan",
    "para", "kontinua", "tenta", "lansa",

    // Type names and literals.
    "num", "int", "bool", "textu", "sin", "nau",
    "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
    "f32", "f64", "isize", "usize"
};

bool SemanticAnalyzer::isReservedKeyword(const std::string& name) {
    return reservedKeywords.count(name) != 0 || prelude::isPreludeName(name)
        || name == erruName;
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
    return integerLiteralFitsType(expr, to);
}

bool SemanticAnalyzer::canCoerceExprTo(const ast::Expr* expr, const Type& to) {
    if (!expr) return false;
    const Type& from = expr->ResolvedType;
    auto* lit = underlyingNumericLiteral(expr);
    if (lit && lit->Type.isFloat() && from.isFloat() && to.isFloat()) return true;
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

    if (node.Name == "inisiu") {
        if (node.Args && !node.Args->Args.empty())
            addError(errLoc(node.LineNum) + "entry function 'inisiu' must not take parameters");
        if (!node.Type.isVoid())
            addError(errLoc(node.LineNum) + "entry function 'inisiu' must not declare a return type");
    }

    FuncInfo info;
    info.retType = node.Type;
    info.canFail = node.CanFail();
    if (node.CanFail() && node.ErrorTypeName != erruName)
        addError(errLoc(node.LineNum) + "unknown error type '" + node.ErrorTypeName
                 + "' in function '" + node.Name + "'; only '" + erruName + "' can follow ':'");
    validateTypeKnown(node.Type, node.LineNum, "return type of function '" + node.Name + "'");
    if (node.Args)
        for (auto& arg : node.Args->Args) {
            if (!arg) continue;
            validateTypeKnown(arg->Type, arg->LineNum, "parameter '" + arg->Name + "'");
            info.paramTypes.push_back(arg->Type);
        }
    FunctionTable[node.Name] = std::move(info);
}

void SemanticAnalyzer::registerRecord(MoldaDeclSttmt& node, bool builtin) {
    if (!builtin && !checkDeclaredNameValid(node.Name, "molda", node.LineNum)) return;

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

std::size_t SemanticAnalyzer::storageBytes(const Type& type) const {
    constexpr std::size_t saturated = static_cast<std::size_t>(-1);

    if (type.isArray()) {
        const std::size_t element = storageBytes(type.elementType());
        if (element != 0 && type.arraySize() > saturated / element)
            return saturated;
        return element * type.arraySize();
    }
    if (type.isInteger() || type.isFloat())
        return std::max<std::size_t>(1, type.bitWidth() / 8);
    if (type == Type::Bool())
        return 1;
    if (type.isNamed()) {
        auto recordIt = RecordTable.find(type.name());
        if (recordIt == RecordTable.end())
            return 0;
        std::size_t total = 0;
        for (const auto* field : recordIt->second.fields) {
            const std::size_t bytes = storageBytes(field->Type);
            if (bytes > saturated - total)
                return saturated;
            total += bytes;
        }
        return total;
    }
    return 8; // textu: a pointer, counted at its 64-bit size on every target
}

bool SemanticAnalyzer::validateArrayInitializer(const Type& expectedType,
                                                ast::Expr* init,
                                                int lineNum,
                                                const std::string& context) {
    if (!isArrayType(expectedType)) {
        addError(errLoc(lineNum) + context + " requires a non-array value; use an explicit '(T[])[...]' array literal only for array targets");
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
                     + std::to_string(expectedSize) + " but repeat initializer [value; "
                     + std::to_string(initRep->Count) + "] has a different count");
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

    // Only declarations may appear at the program root.
    for (auto& s : program->SttmtList) {
        if (!s) continue;
        if (dynamic_cast<VarDeclSttmt*>(s.get())) continue;
        if (dynamic_cast<FuncDeclSttmt*>(s.get())) continue;
        if (dynamic_cast<MoldaDeclSttmt*>(s.get())) continue;
        if (dynamic_cast<ImportSttmt*>(s.get())) continue;
        if (auto* exprSttmt = dynamic_cast<ExprSttmt*>(s.get()))
            if (!exprSttmt->Expression) continue; // bare ';'
        addError(errLoc(s->LineNum)
                 + "only declarations (variables, functions, molda, imports) are allowed at the top level");
    }

    registerRecord(*ErrorTypeDecl, true);
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

    if (!FunctionTable.count("inisiu"))
        addError(errLoc(0) + "program must define an entry function 'fn inisiu()'");

    // Second pass: full semantic walk.
    pushScope();
    for (auto& s : program->SttmtList)
        if (s) s->accept(*this);
    popScope();
}


// A statement that is a call to paniku() or sai() never completes normally.
static bool isDivergingCallStatement(const Sttmt* stmt) {
    auto* exprStmt = dynamic_cast<const ExprSttmt*>(stmt);
    auto* call = exprStmt ? dynamic_cast<const FunCallExpr*>(exprStmt->Expression.get()) : nullptr;
    auto* callee = call ? dynamic_cast<const IdentExpr*>(call->Callee.get()) : nullptr;
    if (!callee) return false;
    const auto builtin = prelude::lookupBuiltin(callee->Name);
    return builtin == prelude::Builtin::Paniku || builtin == prelude::Builtin::Sai;
}

bool SemanticAnalyzer::blockDefinitelyReturns(BlockSttmt* block) const {
    if (!block) return false;
    for (auto& stmt : block->SttmtList) {
        if (!stmt) continue;

        if (isDivergingCallStatement(stmt.get()))
            return true;

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
    ScopedValue<std::string> initializing(InitializingVar, node.Name);
    const std::string kind = node.IsParam ? "parameter" : "variable";
    bool canDeclare = checkDeclaredNameValid(node.Name, kind, node.LineNum);

    if (node.IsArray && node.ArraySize == 0) {
        addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must have a positive size");
        canDeclare = false;
    }

    if (!validateTypeKnown(node.Type, node.LineNum, kind + " '" + node.Name + "'"))
        canDeclare = false;

    if (canDeclare && FunctionDepth > 0) {
        const std::size_t bytes = storageBytes(node.Type);
        if (bytes > KR_MAX_LOCAL_BYTES) {
            addError(errLoc(node.LineNum) + kind + " '" + node.Name + "' needs "
                     + std::to_string(bytes) + " bytes of stack, over the limit of "
                     + std::to_string(KR_MAX_LOCAL_BYTES) + " bytes"
                     + (node.IsParam ? "" : "; declare it at the top level instead"));
            canDeclare = false;
        }
    }

    if (node.IsArray && node.Value) {
        auto* initLit = dynamic_cast<ArrayLiteralExpr*>(node.Value.get());
        auto* initRep = dynamic_cast<ArrayRepeatExpr*>(node.Value.get());
        if (!initLit && !initRep) {
            addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must use an array initializer like [a, b, c] or [value; N]");
            canDeclare = false;
        } else {
            if (!validateArrayInitializer(node.Type, node.Value.get(), node.LineNum,
                                          "array variable '" + node.Name + "'")) {
                canDeclare = false;
            }
        }
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


    if (node.Value && !node.IsArray) {
        if (dynamic_cast<ArrayLiteralExpr*>(node.Value.get()) || dynamic_cast<ArrayRepeatExpr*>(node.Value.get())) {
            addError(errLoc(node.LineNum) + kind + " '" + node.Name
                     + "' uses an array initializer without an array target; declare an array type or use an explicit '(T[])[...]' array literal where an array value is expected");
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
    // Nested functions are not supported.
    if (FunctionDepth > 0) {
        addError(errLoc(node.LineNum) + "nested function declarations are not supported; declare '"
                 + node.Name + "' at the top level");
        return;
    }

    // If the signature wasn't pre-registered (function inside a block), register it now.
    // Top-level functions are already in FunctionTable from the first pass.
    if (!FunctionTable.count(node.Name)) {
        // Top-level functions were handled in the pre-registration pass.
        if (SymbolScopes.size() > 1)
            registerFuncSignature(node);
    }

    ++FunctionDepth;
    Type savedRetType = CurrFuncRetType;
    std::string savedName    = CurrFuncName;
    CurrFuncRetType = node.Type;
    CurrFuncName    = node.Name;
    const bool savedCanFail = CurrFuncCanFail;
    CurrFuncCanFail = node.CanFail();

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
    CurrFuncCanFail = savedCanFail;
    --FunctionDepth;
}

void SemanticAnalyzer::checkConditionType(const ast::Expr* cond, int lineNum) {
    if (!cond) return;
    const Type& t = cond->ResolvedType;
    if (!t.valid()) return;
    if (t.isVoid()) {
        addError(errLoc(lineNum) + "void expression cannot be used as a condition");
        return;
    }
    if (t != Type::Bool() && !t.isNumeric())
        addError(errLoc(lineNum) + "condition must be a boolean or numeric value, got '"
                 + t.str() + "'");
}

void SemanticAnalyzer::visit(IfSttmt& node) {
    if (node.Init) pushScope();
    if (node.Init) node.Init->accept(*this);
    if (node.Cond) {
        node.Cond->accept(*this);
        checkConditionType(node.Cond.get(), node.LineNum);
    }
    if (node.Then) node.Then->accept(*this);
    if (node.Else) node.Else->accept(*this);
    if (node.Init) popScope();
}

void SemanticAnalyzer::visit(WhileSttmt& node) {
    if (node.Cond) {
        node.Cond->accept(*this);
        checkConditionType(node.Cond.get(), node.LineNum);
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

void SemanticAnalyzer::requireFallibleFunction(const std::string& keyword, int lineNum) {
    if (CurrFuncCanFail) return;
    const std::string example = CurrFuncName.empty() ? "f" : CurrFuncName;
    addError(errLoc(lineNum) + "'" + keyword + "' can only be used in a function that declares an "
             "error type, such as 'fn " + example + "(...) : " + erruName + "'");
}

void SemanticAnalyzer::visit(ReturnSttmt& node) {
    const std::string loc = errLoc(node.LineNum);

    if (node.Throws) {
        requireFallibleFunction("lansa", node.LineNum);
        if (node.ReturnValue) {
            node.ReturnValue->accept(*this);
            const Type& got = node.ReturnValue->ResolvedType;
            if (got.valid() && got != Type::Named(erruName))
                addError(loc + "'lansa' expects a value of type '" + erruName + "', got '" + got.str() + "'");
        }
        return;
    }

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

void SemanticAnalyzer::visitArgs(FunCallExpr& node) {
    if (!node.Args) return;
    for (auto& arg : node.Args->Args)
        if (arg) arg->accept(*this);
}

void SemanticAnalyzer::expectArgType(FunCallExpr& node, std::size_t index, const Type& expected,
                                     const std::string& callee) {
    const Type& got = node.Args->Args[index]->ResolvedType;
    if (got.valid() && got != expected)
        addError(errLoc(node.LineNum) + "argument " + std::to_string(index + 1) + " of '" + callee
                 + "': expected '" + expected.str() + "', got '" + got.str() + "'");
}

void SemanticAnalyzer::visit(TypeCallExpr& node) {
    const std::string loc = errLoc(node.LineNum);
    const std::string callee = node.OwnerType.str() + "::" + node.Function;
    node.ResolvedType = Type::Invalid();
    node.ErrorHandled = (&node == HandledCall);
    visitArgs(node);

    if (node.Function != "konverti") {
        addError(loc + "unknown function '" + callee + "'; the only function of a type is '<type>::konverti'");
        return;
    }

    if (!node.OwnerType.isNumeric() && node.OwnerType != Type::Bool()) {
        addError(loc + "cannot convert text to '" + node.OwnerType.str()
                 + "'; only numbers and 'bool' can be converted");
        return;
    }

    const size_t got = node.Args ? node.Args->Args.size() : 0;
    if (got != 1)
        addError(loc + "'" + callee + "' expects 1 argument(s), got " + std::to_string(got));
    else
        expectArgType(node, 0, Type::Text(), callee);

    node.ResolvedType = node.OwnerType;
    node.Fallible = true;
}

void SemanticAnalyzer::visit(FunCallExpr& node) {
    const bool handled = (&node == HandledCall);
    auto* callee = unwrapIdentExpr(node.Callee.get());
    if (!callee) {
        visitArgs(node);
        if (node.Callee) node.Callee->accept(*this);
        addError(errLoc(node.LineNum) + "expression is not callable");
        return;
    }

    const std::string loc = errLoc(node.LineNum);
    size_t got = node.Args ? node.Args->Args.size() : 0;

    switch (prelude::lookupBuiltin(callee->Name)) {
        case prelude::Builtin::Mostra:
        case prelude::Builtin::Mostran:
            if (node.Args) {
                for (auto& arg : node.Args->Args) {
                    if (!arg) continue;
                    if (!handleArrayIdentArg(*arg))
                        arg->accept(*this);
                    const Type& t = arg->ResolvedType;
                    if (t.valid() && !isPrintableType(t, true))
                        addError(loc + "cannot print value of type '" + t.str() + "'");
                }
            }
            node.ResolvedType = Type::Void();
            return;

        case prelude::Builtin::Toma:
            node.Fallible = true;
            node.ErrorHandled = handled;
            visitArgs(node);
            node.ResolvedType = Type::Text();
            if (got > 1)
                addError(loc + "prelude function 'toma' expects 0 or 1 argument(s), got "
                         + std::to_string(got));
            else if (got == 1)
                expectArgType(node, 0, Type::Text(), "toma");
            return;

        case prelude::Builtin::Sai:
            visitArgs(node);
            if (got > 1) {
                addError(loc + "prelude function 'sai' expects 0 or 1 argument(s), got "
                         + std::to_string(got));
                node.ResolvedType = Type::Void();
                return;
            }
            if (got == 1) {
                const Type& t = node.Args->Args[0]->ResolvedType;
                if (t.valid() && !t.isInteger())
                    addError(loc + "sai() expects an integer exit code, got value of type '" + t.str() + "'");
            }
            node.ResolvedType = Type::Void();
            return;

        case prelude::Builtin::Konfirma: {
            visitArgs(node);
            node.ResolvedType = Type::Void();
            if (got < 1 || got > 2) {
                addError(loc + "prelude function 'konfirma' expects 1 or 2 argument(s), got "
                         + std::to_string(got));
                return;
            }
            const Type& cond = node.Args->Args[0]->ResolvedType;
            if (cond.valid() && cond != Type::Bool() && !cond.isInteger() && !cond.isFloat())
                addError(loc + "konfirma() expects a boolean condition, got value of type '" + cond.str() + "'");
            if (got == 2)
                expectArgType(node, 1, Type::Text(), "konfirma");
            return;
        }

        case prelude::Builtin::Paniku:
            visitArgs(node);
            node.ResolvedType = Type::Void();
            if (got != 1)
                addError(loc + "prelude function 'paniku' expects 1 argument(s), got "
                         + std::to_string(got));
            else
                expectArgType(node, 0, Type::Text(), "paniku");
            return;

        case prelude::Builtin::None:
            break;
    }

    visitArgs(node);

    auto it = FunctionTable.find(callee->Name);
    if (it == FunctionTable.end()) {
        addError(errLoc(node.LineNum) + "undeclared function '" + callee->Name + "'");
        return;
    }

    const FuncInfo& info = it->second;
    node.ResolvedType = info.retType;
    node.Fallible = info.canFail;
    node.ErrorHandled = handled;
    // Only functions declared with ': Erru' warn; an unhandled toma() or
    // T::konverti() also stops the program on failure, but silently, to keep
    // simple programs simple.
    if (info.canFail && !handled)
        addWarning(errLoc(node.LineNum) + "the error of '" + callee->Name + "' is not handled, so the "
                   "program stops if it fails; use 'tenta' or 'sinon'");

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

FunCallExpr* SemanticAnalyzer::unwrapCallExpr(Expr* expr) {
    while (auto* par = dynamic_cast<ParExpr*>(expr))
        expr = par->Content.get();
    return dynamic_cast<FunCallExpr*>(expr);
}

bool SemanticAnalyzer::visitFallibleOperand(Expr* operand, const std::string& keyword, int lineNum) {
    auto* call = unwrapCallExpr(operand);
    if (!call) {
        if (operand) operand->accept(*this);
        addError(errLoc(lineNum) + "'" + keyword + "' expects a call to a function that can fail");
        return false;
    }

    {
        ScopedValue<const FunCallExpr*> handled(HandledCall, call);
        operand->accept(*this);
    }
    if (!call->Fallible) {
        addError(errLoc(lineNum) + "'" + keyword + "' applied to a call that cannot fail");
        return false;
    }
    return true;
}

void SemanticAnalyzer::visit(BinExpr& node) {
    if (node.Op == "sinon") {
        const bool valid = visitFallibleOperand(node.LHS.get(), "sinon", node.LineNum);
        if (node.RHS) node.RHS->accept(*this);
        if (!valid) { node.ResolvedType = Type::Invalid(); return; }

        const Type& valueType = node.LHS->ResolvedType;
        if (valueType.isVoid()) {
            addError(errLoc(node.LineNum) + "'sinon' needs a call that returns a value; "
                     "this function returns nothing");
            node.ResolvedType = Type::Invalid();
            return;
        }
        if (node.RHS && node.RHS->ResolvedType.valid()
                && !canCoerceExprTo(node.RHS.get(), valueType))
            addError(errLoc(node.LineNum) + "the value after 'sinon' has type '"
                     + node.RHS->ResolvedType.str() + "', expected '" + valueType.str() + "'");
        node.ResolvedType = valueType;
        return;
    }

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

    if (isBitwiseOp(node.Op)) {
        if (lt.valid() && rt.valid() && (!lt.isInteger() || !rt.isInteger()))
            addError(errLoc(node.LineNum) + "bitwise operator '" + node.Op
                     + "' requires integer operands, got '" + lt.str()
                     + "' and '" + rt.str() + "'");
        node.ResolvedType = lt.isInteger() && rt.isInteger()
            ? promotedNumericTypeForExpr(node.LHS.get(), node.RHS.get(), lt, rt)
            : Type::Invalid();
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

    if (equalityOps.count(node.Op) || relationalOps.count(node.Op)) {
        node.ResolvedType = Type::Bool();
    } else if (node.Op == "/") {
        node.ResolvedType = divisionResultType(lt, rt);
    } else {
        node.ResolvedType = promotedNumericTypeForExpr(node.LHS.get(), node.RHS.get(), lt, rt);
    }
}

bool SemanticAnalyzer::isBitwiseOp(std::string_view op) {
    return op == "&" || op == "|" || op == "^";
}

void SemanticAnalyzer::validateLiteralRange(ast::LiteralExpr& node) {
    if (node.Type.isInteger()) {
        try {
            (void)std::stoll(node.Value);
        } catch (...) {
            addError(errLoc(node.LineNum) + "integer literal '" + node.Value
                     + "' is out of range");
            node.ResolvedType = Type::Invalid();
        }
    } else if (node.Type.isFloat()) {
        try {
            (void)std::stod(node.Value);
        } catch (...) {
            addError(errLoc(node.LineNum) + "floating-point literal '" + node.Value
                     + "' is out of range");
            node.ResolvedType = Type::Invalid();
        }
    }
}

void SemanticAnalyzer::visit(LiteralExpr& node) {
    node.ResolvedType = node.Type;
    validateLiteralRange(node);
}

void SemanticAnalyzer::visit(ExprSttmt& node) {
    if (node.Expression) {
        node.Expression->accept(*this);
        node.ResolvedType = node.Expression->ResolvedType;
    }
}

void SemanticAnalyzer::visit(IdentExpr& node) {
    auto t = lookupVar(node.Name);
    if (!t) {
        addError(errLoc(node.LineNum) + "undefined variable name '" + node.Name + "'");
        return;
    }

    checkNotSelfInitialized(node.Name, node.LineNum);
    node.ResolvedType = *t;
    if (isArrayType(*t))
        addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must be indexed");
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
        checkNotSelfInitialized(baseIdent->Name, node.LineNum);
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
        checkNotSelfInitialized(ident->Name, lineNum);
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

    if (assigneeType.valid() && isArrayType(assigneeType)
            && !dynamic_cast<ArrayAccessExpr*>(node.Assignee.get())) {
        if (auto* ident = dynamic_cast<IdentExpr*>(node.Assignee.get())) {
            addError(errLoc(node.LineNum) + "cannot assign directly to array variable '"
                     + ident->Name + "'; assign to an index");
        } else {
            addError(errLoc(node.LineNum) + "cannot assign directly to array value; assign to an index");
        }
    }

    if (!node.Assigned) return;

    node.Assigned->accept(*this);
    node.ResolvedType = node.Assigned->ResolvedType;
    const Type& valueType = node.Assigned->ResolvedType;

    if (node.AssignOp != "=" && assigneeType.valid()) {
        const bool bitwise = isBitwiseOp(std::string_view(node.AssignOp).substr(0, node.AssignOp.size() - 1));
        const char* required = bitwise ? "an integer" : "a numeric";
        const bool targetOk = bitwise ? assigneeType.isInteger() : assigneeType.isNumeric();
        const bool valueOk = bitwise ? valueType.isInteger() : valueType.isNumeric();

        if (!targetOk)
            addError(errLoc(node.LineNum) + "compound assignment operator '"
                     + node.AssignOp + "' requires " + required + " target, got '"
                     + assigneeType.str() + "'");
        if (valueType.valid() && !valueOk)
            addError(errLoc(node.LineNum) + "compound assignment operator '"
                     + node.AssignOp + "' requires " + required + " value, got '"
                     + valueType.str() + "'");
        if (node.AssignOp == "/=" && assigneeType.isInteger())
            addError(errLoc(node.LineNum) + "compound assignment operator '/=' always yields a real "
                     "number and cannot be stored in a target of type '" + assigneeType.str() + "'");
    }

    if (assigneeType.valid() && valueType.valid()
            && !canCoerceExprTo(node.Assigned.get(), assigneeType))
        addError(errLoc(node.LineNum) + "cannot assign value of type '" + valueType.str()
                 + "' to target of type '" + assigneeType.str() + "'");
}

void SemanticAnalyzer::visit(ForSttmt& node) {
    pushScope();
    if (node.Start) node.Start->accept(*this);
    if (node.Cond) {
        node.Cond->accept(*this);
        checkConditionType(node.Cond.get(), node.LineNum);
    }
    ++LoopDepth;
    if (node.Then)  node.Then->accept(*this);
    --LoopDepth;
    if (node.After) node.After->accept(*this);
    popScope();
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

void SemanticAnalyzer::visit(CastExpr& node) {
    node.ResolvedType = Type::Invalid();
    if (node.Operand) node.Operand->accept(*this);
    const Type& from = node.Operand ? node.Operand->ResolvedType : Type::Invalid();
    const Type& to = node.Target;
    const std::string loc = errLoc(node.LineNum);

    // Text goes through T::konverti instead, because that conversion can fail.
    const auto castable = [](const Type& t) { return t.isNumeric() || t == Type::Bool(); };

    if (!castable(to)) {
        addError(loc + "cannot cast to '" + to.str() + "'; only numbers and 'bool' are valid cast targets");
        return;
    }
    if (!from.valid()) return;
    if (!castable(from)) {
        addError(loc + "cannot cast a value of type '" + from.str() + "' to '" + to.str() + "'");
        return;
    }
    node.ResolvedType = to;
}

void SemanticAnalyzer::checkNotSelfInitialized(const std::string& name, int lineNum) {
    if (name == InitializingVar)
        addError(errLoc(lineNum) + "variable '" + name + "' is used in its own initializer");
}

void SemanticAnalyzer::visit(UnaryExpr& node) {
    if (node.Op == "tenta") {
        requireFallibleFunction("tenta", node.LineNum);
        const bool valid = visitFallibleOperand(node.Operand.get(), "tenta", node.LineNum);
        node.ResolvedType = valid ? node.Operand->ResolvedType : Type::Invalid();
        return;
    }

    if (node.Operand) node.Operand->accept(*this);
    const Type opType = node.Operand ? node.Operand->ResolvedType : Type::Invalid();

    if (node.Op == "!") {
        if (opType.valid() && opType != Type::Bool())
            addError(errLoc(node.LineNum) + "logical operator '!' requires a boolean operand, got '"
                     + opType.str() + "'");
        node.ResolvedType = Type::Bool();
    } else if (node.Op == "~") {
        if (opType.valid() && !opType.isInteger())
            addError(errLoc(node.LineNum) + "bitwise operator '~' requires an integer operand, got '"
                     + opType.str() + "'");
        node.ResolvedType = opType.isInteger() ? opType : Type::Invalid();
    } else { // "-" (numeric negation) keeps operand type
        if (opType.valid() && !opType.isNumeric())
            addError(errLoc(node.LineNum) + "unary operator '-' requires a numeric operand, got '"
                     + opType.str() + "'");
        node.ResolvedType = opType.isNumeric() ? opType : Type::Invalid();
    }
}

} // namespace sema
} // namespace kriol
