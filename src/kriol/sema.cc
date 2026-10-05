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
using kriol::typeutils::hasEmptyDimension;
using kriol::typeutils::isArrayType;
using namespace kriol::typerules;

static bool startsWithUppercaseAscii(const std::string& name) {
    return !name.empty() && std::isupper(static_cast<unsigned char>(name[0]));
}

const std::string erruName = prelude::ErrorTypeName;

// "o argumento 1 de 'f' tem de ser do tipo 'int', mas é do tipo 'textu'"-style
// ending shared by the type mismatch messages.
std::string mustBeOfType(const kriol::Type& want, const kriol::Type& got) {
    return "tem de ser do tipo '" + want.str() + "', mas é do tipo '" + got.str() + "'";
}

// "foi dado 1" or "foram dados 3", for argument and element counts.
std::string givenCount(std::size_t count) {
    return count == 1 ? "foi dado 1" : "foram dados " + std::to_string(count);
}

std::string countOf(std::size_t count, const char* singular, const char* plural) {
    return std::to_string(count) + " " + (count == 1 ? singular : plural);
}

// The hint for a value that only converts to `to` explicitly, such as an int
// stored in a u8: a cast does it, keeping the loss of information visible.
std::string castHint(const kriol::Type& from, const kriol::Type& to) {
    const auto castable = [](const kriol::Type& t) { return t.isNumeric() || t == kriol::Type::Bool(); };
    if (!castable(from) || !castable(to)) return "";
    return "; para converter, escreve (" + to.str() + ") antes do valor";
}

// Contracts "de" or "em" with the article that starts `phrase`: "de" and
// "o array 'a'" give "do array 'a'", "em" and "a variável 'x'" "na variável 'x'".
std::string joined(const std::string& preposition, const std::string& phrase) {
    const bool masculine = phrase.rfind("o ", 0) == 0;
    const bool feminine = phrase.rfind("a ", 0) == 0;
    if (!masculine && !feminine)
        return preposition + " " + phrase;
    const std::string stem = preposition == "de" ? "d" : "n";
    return stem + phrase;
}

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

static const std::unordered_set<std::string> reservedKeywords = {
    // Language keywords.
    "si", "sinon", "nkuantu", "pa", "fn", "molda", "divolvi", "inpristan",
    "kebra", "kontinua", "tenta", "lansa",

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

    addError(errLoc(lineNum) + "'" + name + "' é um nome reservado do Kriol e não pode ser usado como nome de "
             + kind);

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
    // A real value converts implicitly to an integer, truncating toward zero.
    if (from.isFloat() && to.isInteger()) return true;
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
    if (!checkDeclaredNameValid(node.Name, "função", node.LineNum)) return;

    if (FunctionTable.count(node.Name)) {
        addError(errLoc(node.LineNum) + "a função '" + node.Name + "' já foi declarada; cada função tem de ter um nome diferente");
        return;
    }

    if (node.Name == "inisiu") {
        if (node.Args && !node.Args->Args.empty())
            addError(errLoc(node.LineNum) + "a função 'inisiu' não pode ter parâmetros, porque é por ela que o programa começa");
        if (!node.Type.isVoid())
            addError(errLoc(node.LineNum) + "a função 'inisiu' não pode declarar um tipo de retorno, porque é por ela que o programa começa");
    }

    FuncInfo info;
    info.retType = node.Type;
    info.canFail = node.CanFail();
    if (node.CanFail() && node.ErrorTypeName != erruName)
        addError(errLoc(node.LineNum) + "o tipo de erro '" + node.ErrorTypeName + "' da função '" + node.Name
                 + "' não existe; depois de ':' só pode vir '" + erruName + "'");
    validateTypeKnown(node.Type, node.LineNum, "tipo de retorno da função '" + node.Name + "'");
    if (hasEmptyDimension(node.Type))
        addError(errLoc(node.LineNum) + "o array devolvido pela função '" + node.Name
                 + "' tem de ter um tamanho maior do que zero");
    else if (node.Type.isArray() && storageBytes(node.Type) > KR_MAX_LOCAL_BYTES)
        addError(errLoc(node.LineNum) + "o valor devolvido pela função '" + node.Name + "' ocupa "
                 + std::to_string(storageBytes(node.Type)) + " bytes, mais do que o limite de "
                 + std::to_string(KR_MAX_LOCAL_BYTES) + " bytes");
    if (node.Args)
        for (auto& arg : node.Args->Args) {
            if (!arg) continue;
            validateTypeKnown(arg->Type, arg->LineNum, "parâmetro '" + arg->Name + "'");
            info.paramTypes.push_back(arg->Type);
        }
    FunctionTable[node.Name] = std::move(info);
}

void SemanticAnalyzer::registerRecord(MoldaDeclSttmt& node, bool builtin) {
    if (!builtin && !checkDeclaredNameValid(node.Name, "molde", node.LineNum)) return;

    if (!startsWithUppercaseAscii(node.Name)) {
        std::string suggestion = node.Name;
        suggestion[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(suggestion[0])));
        addError(errLoc(node.LineNum) + "o nome do molde '" + node.Name
                 + "' tem de começar por uma letra maiúscula, por exemplo '" + suggestion + "'");
        return;
    }

    if (RecordTable.count(node.Name)) {
        addError(errLoc(node.LineNum) + "o molde '" + node.Name + "' já foi declarado");
        return;
    }

    if (FunctionTable.count(node.Name))
        addError(errLoc(node.LineNum) + "o molde '" + node.Name + "' tem o mesmo nome que uma função");

    if (node.Fields.empty())
        addError(errLoc(node.LineNum) + "o molde '" + node.Name + "' tem de ter pelo menos um campo");

    RecordInfo info;
    std::unordered_set<std::string> seenFields;
    for (auto& field : node.Fields) {
        if (!field) continue;

        if (!checkDeclaredNameValid(field->Name, "campo", field->LineNum))
            continue;

        if (!seenFields.insert(field->Name).second) {
            addError(errLoc(field->LineNum) + "o campo '" + field->Name
                     + "' aparece repetido no molde '" + node.Name + "'");
            continue;
        }

        validateTypeKnown(field->Type, field->LineNum, "campo '" + field->Name + "'");
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

    addError(errLoc(lineNum) + "o tipo '" + type.str() + "' não existe (" + context
             + "); os tipos novos declaram-se com 'molda'");
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
        addError(errLoc(lineNum) + context + " não é um array, por isso não pode receber uma lista [...]");
        return false;
    }

    auto* initLit = dynamic_cast<ArrayLiteralExpr*>(init);
    auto* initRep = dynamic_cast<ArrayRepeatExpr*>(init);
    if (!initLit && !initRep) return false;

    const std::size_t expectedSize = expectedType.arraySize();
    const Type elemType = arrayElementType(expectedType);

    // A row of a multi-dimensional array may itself be an array initializer.
    auto checkElement = [&](ast::Expr* element, const std::string& what) {
        if (!element) return false;
        if (dynamic_cast<ArrayLiteralExpr*>(element) || dynamic_cast<ArrayRepeatExpr*>(element))
            return validateArrayInitializer(elemType, element, lineNum, what);
        element->accept(*this);
        const Type& got = element->ResolvedType;
        if (got.valid() && !canCoerceExprTo(element, elemType)) {
            addError(errLoc(lineNum) + what + " " + mustBeOfType(elemType, got));
            return false;
        }
        return true;
    };

    if (initRep) {
        if (initRep->Count != expectedSize) {
            addError(errLoc(lineNum) + context + " tem " + countOf(expectedSize, "elemento", "elementos")
                     + ", mas a repetição [valor; " + std::to_string(initRep->Count) + "] tem "
                     + std::to_string(initRep->Count));
            return false;
        }
        if (!checkElement(initRep->Fill.get(), "o valor repetido " + joined("em", context)))
            return false;
        initRep->ResolvedType = expectedType;
        return true;
    }

    if (initLit->ExplicitElementType.valid()) {
        initLit->accept(*this);
        if (initLit->ExplicitElementType != elemType
                && !isWideningCoercion(initLit->ExplicitElementType, elemType)) {
            addError(errLoc(lineNum) + "os elementos " + joined("de", context) + " têm de ser do tipo '"
                     + elemType.str() + "', mas a lista foi escrita com o tipo '"
                     + initLit->ExplicitElementType.str() + "'");
            return false;
        }
    }

    const std::size_t got = initLit->Elements.size();
    if (got != expectedSize) {
        addError(errLoc(lineNum) + context + " tem " + countOf(expectedSize, "elemento", "elementos")
                 + ", mas a lista tem " + std::to_string(got));
        return false;
    }

    bool ok = true;
    for (std::size_t i = 0; i < initLit->Elements.size(); ++i) {
        const std::string what = "o elemento " + std::to_string(i + 1) + " " + joined("de", context);
        if (initLit->ExplicitElementType.valid()) {
            const Type& gotType = initLit->Elements[i]->ResolvedType;
            if (gotType.valid() && !canCoerceExprTo(initLit->Elements[i].get(), elemType)) {
                addError(errLoc(lineNum) + what + " " + mustBeOfType(elemType, gotType));
                ok = false;
            }
        } else if (!checkElement(initLit->Elements[i].get(), what)) {
            ok = false;
        }
    }

    initLit->ResolvedType = expectedType;
    return ok;
}

bool SemanticAnalyzer::visitArrayValue(ast::Expr* value, const Type& expected, int lineNum,
                                       const std::string& context) {
    if (!value) return false;
    if (dynamic_cast<ArrayLiteralExpr*>(value) || dynamic_cast<ArrayRepeatExpr*>(value))
        return validateArrayInitializer(expected, value, lineNum, context);

    value->accept(*this);
    const Type& got = value->ResolvedType;
    if (got.valid() && got != expected) {
        addError(errLoc(lineNum) + context + " " + mustBeOfType(expected, got));
        return false;
    }
    return true;
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
                 + "fora das funções só se podem declarar variáveis, funções e moldes; coloca esta instrução dentro de uma função, por exemplo em 'inisiu'");
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
        addError(errLoc(0) + "falta a função 'inisiu', por onde o programa começa; acrescenta 'fn inisiu() { ... }'");

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
    const std::string kind = node.IsParam ? "parâmetro" : "variável";
    const std::string named = (node.IsParam ? "o parâmetro '" : "a variável '") + node.Name + "'";
    bool canDeclare = checkDeclaredNameValid(node.Name, kind, node.LineNum);

    if (node.IsArray && hasEmptyDimension(node.Type)) {
        addError(errLoc(node.LineNum) + "o array '" + node.Name + "' tem de ter um tamanho maior do que zero");
        canDeclare = false;
    }

    if (!validateTypeKnown(node.Type, node.LineNum, kind + " '" + node.Name + "'"))
        canDeclare = false;

    if (canDeclare && FunctionDepth > 0) {
        const std::size_t bytes = storageBytes(node.Type);
        if (bytes > KR_MAX_LOCAL_BYTES) {
            addError(errLoc(node.LineNum) + named + " ocupa "
                     + std::to_string(bytes) + " bytes, mais do que o limite de "
                     + std::to_string(KR_MAX_LOCAL_BYTES) + " bytes dentro de uma função"
                     + (node.IsParam ? "" : "; declara-a fora das funções"));
            canDeclare = false;
        }
    }

    if (node.IsArray && node.Value
            && !visitArrayValue(node.Value.get(), node.Type, node.LineNum, "o array '" + node.Name + "'"))
        canDeclare = false;

    // Check for duplicate in the innermost scope only.
    if (!SymbolScopes.empty()) {
        auto& cur = SymbolScopes.back();
        if (cur.count(node.Name)) {
            addError(errLoc(node.LineNum) + "o nome '" + node.Name + "' já foi declarado neste bloco");
            canDeclare = false;
        }
    }

    if (canDeclare)
        declareVar(node.Name, node.Type);


    if (node.Value && !node.IsArray) {
        if (dynamic_cast<ArrayLiteralExpr*>(node.Value.get()) || dynamic_cast<ArrayRepeatExpr*>(node.Value.get())) {
            addError(errLoc(node.LineNum) + named + " não é um array, por isso não pode receber uma lista [...]; "
                     "para declarar um array, escreve o tamanho a seguir ao tipo, por exemplo 'int[3] "
                     + node.Name + " = [1, 2, 3];'");
            return;
        }

        node.Value->accept(*this);
        if (node.Value->ResolvedType.isVoid())
            addError(errLoc(node.LineNum) + "a função chamada não devolve nenhum valor que se possa guardar "
                     + joined("em", named));
        else if (node.Value->ResolvedType.valid() && !canCoerceExprTo(node.Value.get(), node.Type))
            addError(errLoc(node.LineNum) + "não se pode guardar um valor do tipo '"
                     + node.Value->ResolvedType.str() + "' " + joined("em", named) + ", que é do tipo '"
                     + node.Type.str() + "'" + castHint(node.Value->ResolvedType, node.Type));
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
        addError(errLoc(node.LineNum) + "não se pode declarar a função '" + node.Name
                 + "' dentro de outra função; declara-a fora, ao nível do ficheiro");
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
        addError(errLoc(node.LineNum) + "a função '" + node.Name + "' nem sempre devolve um valor; "
                 "garante que todos os caminhos terminam com 'divolvi'");

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
        addError(errLoc(lineNum) + "a condição chama uma função que não devolve nenhum valor");
        return;
    }
    if (t != Type::Bool() && !t.isNumeric())
        addError(errLoc(lineNum) + "a condição tem de ser um valor lógico ou um número, mas é do tipo '"
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
        addError(errLoc(node.LineNum) + "'" + node.Name + "' só pode ser usado dentro de um ciclo 'pa' ou 'nkuantu'");
}

void SemanticAnalyzer::requireFallibleFunction(const std::string& keyword, int lineNum) {
    if (CurrFuncCanFail) return;
    const std::string example = CurrFuncName.empty() ? "f" : CurrFuncName;
    addError(errLoc(lineNum) + "'" + keyword + "' só pode ser usado numa função que pode falhar, "
             "declarada com ': " + erruName + "', como 'fn " + example + "(...) : " + erruName + "'");
}

void SemanticAnalyzer::visit(ReturnSttmt& node) {
    const std::string loc = errLoc(node.LineNum);

    if (node.Throws) {
        requireFallibleFunction("lansa", node.LineNum);
        if (node.ReturnValue) {
            node.ReturnValue->accept(*this);
            const Type& got = node.ReturnValue->ResolvedType;
            if (got.valid() && got != Type::Named(erruName))
                addError(loc + "'lansa' recebe um valor do tipo '" + erruName + "', mas este é do tipo '"
                         + got.str() + "'; por exemplo: lansa " + erruName + "{mensage = \"...\"};");
        }
        return;
    }

    if (CurrFuncRetType.valid() && CurrFuncRetType.isVoid() && node.ReturnValue)
        addError(loc + "a função '" + CurrFuncName + "' não declara um tipo de retorno, por isso 'divolvi' não pode ter um valor");

    if (CurrFuncRetType.valid() && !CurrFuncRetType.isVoid() && !node.ReturnValue)
        addError(loc + "a função '" + CurrFuncName + "' devolve um valor do tipo '" + CurrFuncRetType.str()
                 + "', mas este 'divolvi' não tem valor");

    if (node.ReturnValue && CurrFuncRetType.isArray()) {
        visitArrayValue(node.ReturnValue.get(), CurrFuncRetType, node.LineNum,
                        "o valor devolvido por '" + CurrFuncName + "'");
    } else if (node.ReturnValue) {
        node.ReturnValue->accept(*this);
        const Type& got = node.ReturnValue->ResolvedType;
        if (got.valid() && CurrFuncRetType.valid()
                && !CurrFuncRetType.isVoid()
                && !canCoerceExprTo(node.ReturnValue.get(), CurrFuncRetType))
            addError(loc + "a função '" + CurrFuncName + "' devolve um valor do tipo '" + CurrFuncRetType.str()
                     + "', mas este 'divolvi' tem um valor do tipo '" + got.str() + "'");
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
        addError(errLoc(node.LineNum) + "o argumento " + std::to_string(index + 1) + " de '" + callee
                 + "' " + mustBeOfType(expected, got));
}

void SemanticAnalyzer::visit(TypeCallExpr& node) {
    const std::string loc = errLoc(node.LineNum);
    const std::string callee = node.OwnerType.str() + "::" + node.Function;
    node.ResolvedType = Type::Invalid();
    node.ErrorHandled = (&node == HandledCall);
    visitArgs(node);

    if (node.Function != "konverti") {
        addError(loc + "a função '" + callee + "' não existe; a única função de um tipo é 'konverti', "
                 "como em '" + node.OwnerType.str() + "::konverti(texto)'");
        return;
    }

    if (!node.OwnerType.isNumeric() && node.OwnerType != Type::Bool()) {
        addError(loc + "não se pode converter texto em '" + node.OwnerType.str()
                 + "'; 'konverti' só converte texto em números e em 'bool'");
        return;
    }

    const size_t got = node.Args ? node.Args->Args.size() : 0;
    if (got != 1)
        addError(loc + "'" + callee + "' recebe 1 argumento, o texto a converter, mas " + givenCount(got));
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
        addError(errLoc(node.LineNum) + "só se podem chamar funções, e esta expressão não é uma função");
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
                    arg->accept(*this);
                    const Type& t = arg->ResolvedType;
                    if (t.valid() && !isPrintableType(t, true))
                        addError(loc + "não se pode escrever um valor do tipo '" + t.str()
                                 + "' de uma só vez; escreve os campos do molde um a um");
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
                addError(loc + "'toma' recebe no máximo 1 argumento, a mensagem a mostrar, mas "
                         + givenCount(got));
            else if (got == 1)
                expectArgType(node, 0, Type::Text(), "toma");
            return;

        case prelude::Builtin::Sai:
            visitArgs(node);
            if (got > 1) {
                addError(loc + "'sai' recebe no máximo 1 argumento, o código de saída, mas "
                         + givenCount(got));
                node.ResolvedType = Type::Void();
                return;
            }
            if (got == 1) {
                const Type& t = node.Args->Args[0]->ResolvedType;
                if (t.valid() && !t.isInteger())
                    addError(loc + "o código de saída de 'sai' tem de ser um número inteiro, mas é do tipo '"
                             + t.str() + "'");
            }
            node.ResolvedType = Type::Void();
            return;

        case prelude::Builtin::Konfirma: {
            visitArgs(node);
            node.ResolvedType = Type::Void();
            if (got < 1 || got > 2) {
                addError(loc + "'konfirma' recebe 1 ou 2 argumentos, a condição e uma mensagem opcional, mas "
                         + givenCount(got));
                return;
            }
            const Type& cond = node.Args->Args[0]->ResolvedType;
            if (cond.valid() && cond != Type::Bool() && !cond.isInteger() && !cond.isFloat())
                addError(loc + "a condição de 'konfirma' tem de ser um valor lógico ou um número, mas é do tipo '"
                         + cond.str() + "'");
            if (got == 2)
                expectArgType(node, 1, Type::Text(), "konfirma");
            return;
        }

        case prelude::Builtin::Paniku:
            visitArgs(node);
            node.ResolvedType = Type::Void();
            if (got != 1)
                addError(loc + "'paniku' recebe 1 argumento, a mensagem de erro, mas " + givenCount(got));
            else
                expectArgType(node, 0, Type::Text(), "paniku");
            return;

        case prelude::Builtin::None:
            break;
    }

    auto it = FunctionTable.find(callee->Name);
    if (it == FunctionTable.end()) {
        visitArgs(node);
        addError(errLoc(node.LineNum) + "a função '" + callee->Name + "' não foi declarada");
        return;
    }

    // An array parameter takes an array initializer checked against its type.
    if (node.Args)
        for (size_t i = 0; i < node.Args->Args.size(); ++i) {
            auto& arg = node.Args->Args[i];
            if (!arg) continue;
            if (i < it->second.paramTypes.size() && it->second.paramTypes[i].isArray())
                visitArrayValue(arg.get(), it->second.paramTypes[i], node.LineNum,
                                "o argumento " + std::to_string(i + 1) + " de '" + callee->Name + "'");
            else
                arg->accept(*this);
        }

    const FuncInfo& info = it->second;
    node.ResolvedType = info.retType;
    node.Fallible = info.canFail;
    node.ErrorHandled = handled;
    // Only functions declared with ': Erru' warn; an unhandled toma() or
    // T::konverti() also stops the program on failure, but silently, to keep
    // simple programs simple.
    if (info.canFail && !handled)
        addWarning(warnLoc(node.LineNum) + "o erro de '" + callee->Name + "' não é tratado, por isso o "
                   "programa termina se a função falhar; usa 'tenta' ou 'sinon'");

    size_t want = info.paramTypes.size();

    if (got != want) {
        addError(loc + "a função '" + callee->Name + "' recebe " + countOf(want, "argumento", "argumentos")
                 + ", mas " + givenCount(got));
        return; // type checks make no sense if counts differ
    }

    if (node.Args) {
        for (size_t i = 0; i < node.Args->Args.size(); ++i) {
            const Type& argType   = node.Args->Args[i]->ResolvedType;
            const Type& paramType = info.paramTypes[i];
            if (paramType.isArray()) continue;  // checked by visitArrayValue
            if (argType.valid() && !canCoerceExprTo(node.Args->Args[i].get(), paramType))
                addError(loc + "o argumento " + std::to_string(i + 1) + " de '" + callee->Name
                         + "' " + mustBeOfType(paramType, argType));
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
        addError(errLoc(lineNum) + "'" + keyword + "' tem de ser usado com a chamada de uma função que pode falhar");
        return false;
    }

    {
        ScopedValue<const FunCallExpr*> handled(HandledCall, call);
        operand->accept(*this);
    }
    if (!call->Fallible) {
        addError(errLoc(lineNum) + "'" + keyword + "' só serve para chamadas que podem falhar, e esta não pode");
        return false;
    }
    return true;
}

void SemanticAnalyzer::visit(BinExpr& node) {
    if (node.Op == "sinon") {
        const bool valid = visitFallibleOperand(node.LHS.get(), "sinon", node.LineNum);
        const Type& valueType = node.LHS->ResolvedType;
        if (valid && valueType.isArray()) {
            visitArrayValue(node.RHS.get(), valueType, node.LineNum, "o valor depois de 'sinon'");
            node.ResolvedType = valueType;
            return;
        }
        if (node.RHS) node.RHS->accept(*this);
        if (!valid) { node.ResolvedType = Type::Invalid(); return; }

        if (valueType.isVoid()) {
            addError(errLoc(node.LineNum) + "'sinon' precisa de uma chamada que devolva um valor, "
                     "mas esta função não devolve nenhum");
            node.ResolvedType = Type::Invalid();
            return;
        }
        if (node.RHS && node.RHS->ResolvedType.valid()
                && !canCoerceExprTo(node.RHS.get(), valueType))
            addError(errLoc(node.LineNum) + "o valor depois de 'sinon' "
                     + mustBeOfType(valueType, node.RHS->ResolvedType));
        node.ResolvedType = valueType;
        return;
    }

    if (node.LHS) node.LHS->accept(*this);
    if (node.RHS) node.RHS->accept(*this);

    const Type lt = node.LHS ? node.LHS->ResolvedType : Type::Invalid();
    const Type rt = node.RHS ? node.RHS->ResolvedType : Type::Invalid();

    if (lt.isVoid() || rt.isVoid()) {
        addError(errLoc(node.LineNum) + "uma função que não devolve nenhum valor não pode ser usada numa operação");
        node.ResolvedType = Type::Invalid();
        return;
    }

    static const std::unordered_set<std::string> equalityOps = {
        "==", "!="
    };
    static const std::unordered_set<std::string> relationalOps = {
        "<", "<=", ">", ">="
    };

    if (node.Op == "&&" || node.Op == "||") {
        // As in a condition, a number is true when it is not zero.
        for (const Type* operand : {&lt, &rt})
            if (operand->valid() && *operand != Type::Bool() && !operand->isNumeric())
                addError(errLoc(node.LineNum) + "o operador '" + node.Op
                         + "' precisa de valores lógicos ou de números, mas recebeu um valor do tipo '"
                         + operand->str() + "'");
        node.ResolvedType = Type::Bool();
        return;
    }

    if (equalityOps.count(node.Op) && lt == Type::Bool() && rt == Type::Bool()) {
        node.ResolvedType = Type::Bool();
        return;
    }

    if (isBitwiseOp(node.Op)) {
        if (lt.valid() && rt.valid() && (!lt.isInteger() || !rt.isInteger()))
            addError(errLoc(node.LineNum) + "o operador '" + node.Op
                     + "' só funciona com números inteiros, mas recebeu valores dos tipos '" + lt.str()
                     + "' e '" + rt.str() + "'");
        node.ResolvedType = lt.isInteger() && rt.isInteger()
            ? promotedNumericTypeForExpr(node.LHS.get(), node.RHS.get(), lt, rt)
            : Type::Invalid();
        return;
    }

    if (!lt.isNumeric() || !rt.isNumeric()) {
        if (equalityOps.count(node.Op) && lt == Type::Text() && rt == Type::Text())
            addError(errLoc(node.LineNum) + "ainda não é possível comparar textos com '" + node.Op + "'");
        else if (lt.valid() && rt.valid())
            addError(errLoc(node.LineNum) + "o operador '" + node.Op
                     + "' só funciona com números, mas recebeu valores dos tipos '" + lt.str()
                     + "' e '" + rt.str() + "'");
        node.ResolvedType = (equalityOps.count(node.Op) || relationalOps.count(node.Op))
            ? Type::Bool()
            : Type::Invalid();
        return;
    }

    if (equalityOps.count(node.Op) || relationalOps.count(node.Op)) {
        node.ResolvedType = Type::Bool();
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
            addError(errLoc(node.LineNum) + "o número inteiro '" + node.Value
                     + "' é demasiado grande");
            node.ResolvedType = Type::Invalid();
        }
    } else if (node.Type.isFloat()) {
        try {
            (void)std::stod(node.Value);
        } catch (...) {
            addError(errLoc(node.LineNum) + "o número real '" + node.Value
                     + "' é demasiado grande");
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
        addError(errLoc(node.LineNum) + "a variável '" + node.Name + "' não foi declarada; "
                 "declara-a antes de a usar, por exemplo 'int " + node.Name + " = 0;'");
        return;
    }

    checkNotSelfInitialized(node.Name, node.LineNum);
    node.ResolvedType = *t;
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
            addError(errLoc(node.LineNum) + "o índice de um array tem de ser um número inteiro");
    }

    auto* baseIdent = unwrapIdentExpr(node.Base.get());
    Type arrayType = Type::Invalid();
    if (baseIdent) {
        auto found = lookupVar(baseIdent->Name);
        if (!found) {
            addError(errLoc(node.LineNum) + "o array '" + baseIdent->Name + "' não foi declarado");
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
        addError(errLoc(node.LineNum) + "só se pode usar [índice] com arrays, e este valor não é um array");
        return;
    }

    node.ResolvedType = arrayElementType(arrayType);

    auto firstDim = firstArrayDim(arrayType);
    if (!firstDim || *firstDim == 0) {
        addError(errLoc(node.LineNum) + "erro interno: o tamanho do array é inválido");
    }
}

void SemanticAnalyzer::visit(MemberAccessExpr& node) {
    if (node.Base) node.Base->accept(*this);
    const Type baseType = node.Base ? node.Base->ResolvedType : Type::Invalid();
    if (!baseType.valid()) return;

    if (!baseType.isNamed()) {
        addError(errLoc(node.LineNum) + "só os moldes têm campos, e este valor não é um molde");
        return;
    }

    auto recordIt = RecordTable.find(baseType.name());
    if (recordIt == RecordTable.end()) {
        addError(errLoc(node.LineNum) + "o molde '" + baseType.str() + "' não existe");
        return;
    }

    const auto& fields = recordIt->second;
    auto fieldIt = fields.fieldIndex.find(node.Member);
    if (fieldIt == fields.fieldIndex.end()) {
        addError(errLoc(node.LineNum) + "o molde '" + baseType.str()
                 + "' não tem o campo '" + node.Member + "'");
        return;
    }

    node.ResolvedType = fields.fields[fieldIt->second]->Type;
}

void SemanticAnalyzer::visit(QualifiedAccessExpr& node) {
    addError(errLoc(node.LineNum) + "o acesso com '::' ainda não é suportado");
}

void SemanticAnalyzer::visit(ArrayLiteralExpr& node) {
    for (auto& element : node.Elements) {
        if (element) element->accept(*this);
    }

    if (!node.ExplicitElementType.valid()) {
        node.ResolvedType = Type::ArrayLiteral();
        return;
    }

    validateTypeKnown(node.ExplicitElementType, node.LineNum, "tipo dos elementos da lista");
    for (std::size_t i = 0; i < node.Elements.size(); ++i) {
        const Type& got = node.Elements[i]->ResolvedType;
        if (got.valid() && !canCoerceExprTo(node.Elements[i].get(), node.ExplicitElementType)) {
            addError(errLoc(node.LineNum) + "o elemento " + std::to_string(i + 1) + " da lista "
                     + mustBeOfType(node.ExplicitElementType, got));
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
        addError(errLoc(node.LineNum) + "o molde '" + node.TypeName + "' não existe");
        return;
    }

    const RecordInfo& info = recordIt->second;
    std::unordered_set<std::string> seen;
    std::vector<bool> initialized(info.fields.size(), false);

    for (auto& field : node.Fields) {
        if (!seen.insert(field.Name).second) {
            addError(errLoc(node.LineNum) + "o campo '" + field.Name
                     + "' aparece repetido na criação de '" + node.TypeName + "'");
            continue;
        }

        auto indexIt = info.fieldIndex.find(field.Name);
        if (indexIt == info.fieldIndex.end()) {
            addError(errLoc(node.LineNum) + "o molde '" + node.TypeName
                     + "' não tem o campo '" + field.Name + "'");
            if (field.Value) field.Value->accept(*this);
            continue;
        }

        const Type& want = info.fields[indexIt->second]->Type;
        if (field.Value) {
            const bool isArrayInit = dynamic_cast<ArrayLiteralExpr*>(field.Value.get())
                || dynamic_cast<ArrayRepeatExpr*>(field.Value.get());
            if (isArrayInit) {
                validateArrayInitializer(want, field.Value.get(), node.LineNum,
                                         "o campo '" + field.Name + "' de '" + node.TypeName + "'");
            } else {
                field.Value->accept(*this);
            }
        }

        const Type& got = field.Value ? field.Value->ResolvedType : Type::Invalid();
        if (got.valid() && !canCoerceExprTo(field.Value.get(), want))
            addError(errLoc(node.LineNum) + "o campo '" + field.Name + "' de '"
                     + node.TypeName + "' " + mustBeOfType(want, got));
        initialized[indexIt->second] = true;
    }

    for (std::size_t i = 0; i < info.fields.size(); ++i) {
        if (!initialized[i])
            addError(errLoc(node.LineNum) + "falta o campo '" + info.fields[i]->Name
                     + "' na criação de '" + node.TypeName + "'; todos os campos têm de ter um valor");
    }

    node.ResolvedType = Type::Named(node.TypeName);
}

Type SemanticAnalyzer::resolveAssignableType(ast::Expr* expr, int lineNum) {
    if (!expr) return Type::Invalid();

    if (auto* ident = dynamic_cast<IdentExpr*>(expr)) {
        auto t = lookupVar(ident->Name);
        if (!t) {
            addError(errLoc(lineNum) + "a variável '" + ident->Name + "' não foi declarada; "
                     "declara-a antes de a usar, por exemplo 'int " + ident->Name + " = 0;'");
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
                addError(errLoc(lineNum) + "o índice de um array tem de ser um número inteiro");
        }

        if (!isArrayType(baseType)) {
            addError(errLoc(lineNum) + "só se pode usar [índice] com arrays, e este valor não é um array");
            return Type::Invalid();
        }

        arr->ResolvedType = arrayElementType(baseType);
        return arr->ResolvedType;
    }

    if (auto* member = dynamic_cast<MemberAccessExpr*>(expr)) {
        Type baseType = resolveAssignableType(member->Base.get(), lineNum);
        if (!baseType.valid()) return Type::Invalid();

        if (!baseType.isNamed()) {
            addError(errLoc(lineNum) + "só os moldes têm campos, e este valor não é um molde");
            return Type::Invalid();
        }

        auto recordIt = RecordTable.find(baseType.name());
        if (recordIt == RecordTable.end()) {
            addError(errLoc(lineNum) + "o molde '" + baseType.str() + "' não existe");
            return Type::Invalid();
        }

        auto fieldIt = recordIt->second.fieldIndex.find(member->Member);
        if (fieldIt == recordIt->second.fieldIndex.end()) {
            addError(errLoc(lineNum) + "o molde '" + baseType.str()
                     + "' não tem o campo '" + member->Member + "'");
            return Type::Invalid();
        }

        member->ResolvedType = recordIt->second.fields[fieldIt->second]->Type;
        return member->ResolvedType;
    }

    expr->accept(*this);
    addError(errLoc(lineNum) + "só se pode atribuir um valor a uma variável, a um elemento de um array "
             "ou a um campo de um molde");
    return Type::Invalid();
}

void SemanticAnalyzer::visit(AssignExpr& node) {
    Type assigneeType = resolveAssignableType(node.Assignee.get(), node.LineNum);

    if (!node.Assigned) return;

    if (assigneeType.valid() && isArrayType(assigneeType)) {
        if (node.AssignOp != "=") {
            addError(errLoc(node.LineNum) + "o operador '" + node.AssignOp
                     + "' só funciona com números, e '" + assigneeType.str() + "' é um array");
            node.Assigned->accept(*this);
        } else {
            visitArrayValue(node.Assigned.get(), assigneeType, node.LineNum, "o valor atribuído");
        }
        node.ResolvedType = assigneeType;
        return;
    }

    node.Assigned->accept(*this);
    node.ResolvedType = node.Assigned->ResolvedType;
    const Type& valueType = node.Assigned->ResolvedType;

    if (node.AssignOp != "=" && assigneeType.valid()) {
        const bool bitwise = isBitwiseOp(std::string_view(node.AssignOp).substr(0, node.AssignOp.size() - 1));
        const char* required = bitwise ? "números inteiros" : "números";
        const bool targetOk = bitwise ? assigneeType.isInteger() : assigneeType.isNumeric();
        const bool valueOk = bitwise ? valueType.isInteger() : valueType.isNumeric();

        if (!targetOk)
            addError(errLoc(node.LineNum) + "o operador '" + node.AssignOp + "' só funciona com "
                     + required + ", mas o destino é do tipo '" + assigneeType.str() + "'");
        if (valueType.valid() && !valueOk)
            addError(errLoc(node.LineNum) + "o operador '" + node.AssignOp + "' só funciona com "
                     + required + ", mas o valor é do tipo '" + valueType.str() + "'");
    }

    if (assigneeType.valid() && valueType.valid()
            && !canCoerceExprTo(node.Assigned.get(), assigneeType))
        addError(errLoc(node.LineNum) + "não se pode guardar um valor do tipo '" + valueType.str()
                 + "' num destino do tipo '" + assigneeType.str() + "'" + castHint(valueType, assigneeType));
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
    addError(errLoc(node.LineNum) + "'inpristan' (importar módulos) ainda não é suportado");
}

void SemanticAnalyzer::visit(FStringExpr& node) {
    for (auto& seg : node.Parts) {
        if (!seg.expr) continue;
        seg.expr->accept(*this);
        const Type& t = seg.expr->ResolvedType;
        if (t.valid() && !isPrintableType(t, true))
            addError(errLoc(node.LineNum) + "não se pode escrever um valor do tipo '" + t.str()
                     + "' num texto interpolado; escreve os campos do molde um a um");
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
        addError(loc + "não se pode converter para '" + to.str() + "' com (tipo); só se converte para números e 'bool'"
                 + (to == Type::Text() ? "; para obter texto usa um texto interpolado, como f\"{valor}\"" : ""));
        return;
    }
    if (!from.valid()) return;
    if (!castable(from)) {
        addError(loc + "não se pode converter um valor do tipo '" + from.str() + "' para '" + to.str() + "' com (tipo)"
                 + (from == Type::Text() ? "; para converter texto usa '" + to.str() + "::konverti(texto)'" : ""));
        return;
    }
    node.ResolvedType = to;
}

void SemanticAnalyzer::checkNotSelfInitialized(const std::string& name, int lineNum) {
    if (name == InitializingVar)
        addError(errLoc(lineNum) + "a variável '" + name + "' é usada no seu próprio valor inicial");
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
        // As in a condition, a number is true when it is not zero.
        if (opType.valid() && opType != Type::Bool() && !opType.isNumeric())
            addError(errLoc(node.LineNum) + "o operador '!' precisa de um valor lógico ou de um número, "
                     "mas recebeu um valor do tipo '" + opType.str() + "'");
        node.ResolvedType = Type::Bool();
    } else if (node.Op == "~") {
        if (opType.valid() && !opType.isInteger())
            addError(errLoc(node.LineNum) + "o operador '~' só funciona com números inteiros, "
                     "mas recebeu um valor do tipo '" + opType.str() + "'");
        node.ResolvedType = opType.isInteger() ? opType : Type::Invalid();
    } else { // "-" (numeric negation) keeps operand type
        if (opType.valid() && !opType.isNumeric())
            addError(errLoc(node.LineNum) + "o operador '-' só funciona com números, "
                     "mas recebeu um valor do tipo '" + opType.str() + "'");
        node.ResolvedType = opType.isNumeric() ? opType : Type::Invalid();
    }
}

} // namespace sema
} // namespace kriol
