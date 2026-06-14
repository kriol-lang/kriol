#include "../../include/kriol/sema.hh"
#include "../../include/kriol/type_utils.hh"

#include <stdexcept>
#include <unordered_set>
#include <algorithm>

using namespace kriol::ast;

namespace kriol {
namespace sema {

namespace {

using kriol::typeutils::arrayElementType;
using kriol::typeutils::firstArrayDim;
using kriol::typeutils::isArrayType;

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
    "si", "sinon", "nkuantu", "pa", "fn", "divolvi", "inpristan",
    "para", "kontinua", "dipoz",

    // Type names and literals.
    "num", "nter", "bool", "textu", "sin", "nau"
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
    if (from == Type::Integer() && to == Type::Number()) return true;
    if (from == Type::Bool() && to == Type::Integer()) return true;
    if (from == Type::Bool() && to == Type::Number()) return true;
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
    if (node.Args)
        for (auto& arg : node.Args->Args)
            if (arg) info.paramTypes.push_back(arg->Type);
    FunctionTable[node.Name] = std::move(info);
}

void SemanticAnalyzer::Check(BlockSttmt* program) {
    if (!program) return;

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

    if (node.IsArray && node.Value) {
        auto* initLit = dynamic_cast<ArrayLiteralExpr*>(node.Value.get());
        auto* initRep = dynamic_cast<ArrayRepeatExpr*>(node.Value.get());
        if (!initLit && !initRep) {
            addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' must use an array initializer like [a, b, c] or [value] * N");
            canDeclare = false;
        } else if (initRep) {
            initRep->accept(*this);
            const std::size_t expected = node.ArraySize;
            if (initRep->Count != expected) {
                addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' has size "
                         + std::to_string(expected) + " but repeat initializer [value] * "
                         + std::to_string(initRep->Count) + " has a different count");
                canDeclare = false;
            }
            const Type elemType = arrayElementType(node.Type);
            const Type fillType = initRep->Fill ? initRep->Fill->ResolvedType : Type::Invalid();
            if (fillType.valid() && fillType != elemType && !isWideningCoercion(fillType, elemType))
                addError(errLoc(node.LineNum) + "repeat initializer fill type '" + fillType.str()
                         + "' does not match array element type '" + elemType.str() + "'");
            initState.elementInitialized.assign(expected, true);
            initState.fullyInitialized = true;
        } else {
            initLit->accept(*this);
            const std::size_t expected = node.ArraySize;
            const std::size_t got = initLit->Elements.size();
            initState.elementInitialized.assign(expected, false);
            if (got != expected) {
                addError(errLoc(node.LineNum) + "array variable '" + node.Name + "' expects "
                         + std::to_string(expected) + " initializer element(s), got " + std::to_string(got));
                canDeclare = false;
            }

            const Type elemType = arrayElementType(node.Type);
            for (size_t i = 0; i < initLit->Elements.size(); ++i) {
                const auto& t = initLit->Elements[i]->ResolvedType;
                if (t.valid() && t != elemType && !isWideningCoercion(t, elemType)) {
                    addError(errLoc(node.LineNum) + "array initializer element " + std::to_string(i + 1)
                             + ": expected '" + elemType.str() + "', got '" + t.str() + "'");
                }
                if (i < initState.elementInitialized.size()) initState.elementInitialized[i] = true;
            }
            initState.fullyInitialized = (got == expected);
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

    if (node.Value) {
        node.Value->accept(*this);
        if (node.Value->ResolvedType.isVoid())
            addError(errLoc(node.LineNum) + "cannot assign void expression to variable '" + node.Name + "'");
    }
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
                && got != CurrFuncRetType
                && !isWideningCoercion(got, CurrFuncRetType))
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
            if (argType.valid() && argType != paramType
                    && !isWideningCoercion(argType, paramType))
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

    static const std::unordered_set<std::string> boolOps = {
        "==", "!=", "<", "<=", ">", ">=", "&&", "||"
    };

    if (boolOps.count(node.Op)) {
        node.ResolvedType = Type::Bool();
    } else {
        // Arithmetic: promote nter+num -> num, otherwise keep operand type
        if (lt == Type::Number() || rt == Type::Number())
            node.ResolvedType = Type::Number();
        else if (lt.valid())
            node.ResolvedType = lt;
        else
            node.ResolvedType = rt;
    }
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
        if (indexType.valid() && indexType != Type::Integer())
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
    if (!lit || lit->Type != Type::Integer()) {
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
    addError(errLoc(node.LineNum) + "member access is not supported yet");
}

void SemanticAnalyzer::visit(QualifiedAccessExpr& node) {
    addError(errLoc(node.LineNum) + "qualified item access is not supported yet");
}

void SemanticAnalyzer::visit(ArrayLiteralExpr& node) {
    for (auto& element : node.Elements) {
        if (element) element->accept(*this);
    }
    node.ResolvedType = Type::ArrayLiteral();
}

void SemanticAnalyzer::visit(ArrayRepeatExpr& node) {
    if (node.Fill) node.Fill->accept(*this);
    node.ResolvedType = Type::ArrayRepeat();
}

void SemanticAnalyzer::visit(AssignExpr& node) {
    Type assigneeType;
    bool canMarkInitialized = true;

    if (auto* ident = dynamic_cast<IdentExpr*>(node.Assignee.get())) {
        auto t = lookupVar(ident->Name);
        if (!t) {
            addError(errLoc(node.LineNum) + "undefined variable name '" + ident->Name + "'");
            canMarkInitialized = false;
        } else {
            assigneeType = *t;
            if (isArrayType(assigneeType)) {
                addError(errLoc(node.LineNum) + "cannot assign directly to array variable '" + ident->Name + "'; assign to an index");
                canMarkInitialized = false;
            }
        }
    } else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(node.Assignee.get())) {
        if (arr->Index) {
            arr->Index->accept(*this);
            if (arr->Index->ResolvedType != Type::Integer()) {
                addError(errLoc(node.LineNum) + "array index must be an integer");
                canMarkInitialized = false;
            }
        }

        auto* baseIdent = unwrapIdentExpr(arr->Base.get());
        auto t = baseIdent ? lookupVar(baseIdent->Name) : std::optional<Type>{};
        if (!t) {
            addError(errLoc(node.LineNum) + "array assignment target must be a variable");
            canMarkInitialized = false;
        } else if (!isArrayType(*t)) {
            addError(errLoc(node.LineNum) + "indexed assignment target is not an array");
            canMarkInitialized = false;
        } else {
            assigneeType = arrayElementType(*t);
        }
    } else if (node.Assignee) {
        node.Assignee->accept(*this);
        addError(errLoc(node.LineNum) + "invalid assignment target");
        canMarkInitialized = false;
    }

    if (node.Assigned) {
        node.Assigned->accept(*this);
        node.ResolvedType = node.Assigned->ResolvedType;

        if (assigneeType.valid() && node.Assigned->ResolvedType.valid()
                && assigneeType != node.Assigned->ResolvedType
                && !isWideningCoercion(node.Assigned->ResolvedType, assigneeType)) {
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
            if (lit && lit->Type == Type::Integer()) {
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
        for (auto& arg : node.Args->Args)
            if (arg && !handleArrayIdentArg(*arg))
                arg->accept(*this);
    node.ResolvedType = Type::Void();
}

void SemanticAnalyzer::visit(ImportSttmt& node) {
    throw std::runtime_error("'inpristan' (import) statements are not supported yet");
}

void SemanticAnalyzer::visit(FStringExpr& node) {
    for (auto& seg : node.Parts) {
        if (!seg.expr) continue;
        if (!handleArrayIdentArg(*seg.expr)) {
            seg.expr->accept(*this);
            const Type& t = seg.expr->ResolvedType;
            const bool printable = t == Type::Integer() || t == Type::Number()
                || t == Type::Bool() || t == Type::Text();
            if (t.valid() && !printable)
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
        if (t != Type::Integer()) {
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
        if (t != Type::Bool() && t != Type::Integer() && t != Type::Number()) {
            std::string err = "konfirma() expects a boolean condition";
            if (t.valid()) err += ", got value of type '" + t.str() + "'";
            addError(errLoc(node.LineNum) + err);
        }
    };
}

} // namespace sema
} // namespace kriol
