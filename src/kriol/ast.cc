#include "../../include/kriol/ast.hh"
#include "../../include/kriol/codegen.hh"
#include "../../include/kriol/cbinds.hh"

using namespace kriol::ast;

std::string translate_type(const std::string& type) {
    if (type == "num")   return "double";
    if (type == "nter")  return "long long";
    if (type == "vaziu") return "void";
    if (type == "bool")  return "unsigned short";
    if (type == "textu") return "char*";
    return type;
}

std::string CodeGenVisitor::inferFormat(Expr* expr) {
    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        if (lit->Type == "int")            return "%d";
        if (lit->Type == "float")          return "%g";
        if (lit->Type == "unsigned short") return "%u";
        if (lit->Type == "char*")          return "%s";
    }
    if (auto* id = dynamic_cast<IdentExpr*>(expr)) {
        auto it = symTable.find(id->Name);
        if (it != symTable.end()) {
            const std::string& t = it->second;
            if (t == "num")            return "%g";
            if (t == "nter")           return "%lld";
            if (t == "unsigned short") return "%u";
            if (t == "textu")          return "%s";
            if (t == "char*")          return "%s";
        }
    }
    return "";
}


void CodeGenVisitor::visit(VarDeclSttmt& node) {
    // track for auto-formatting of printf in MostraFunCallExpr
    symTable[node.Name] = node.Type;
    os << translate_type(node.Type) << " " << node.Name;
    if (node.Value) {
        os << C_EQ;
        node.Value->accept(*this);
    }
}

void CodeGenVisitor::visit(BlockSttmt& node) {
    if (node.BracketsOn) os << C_OPEN_CURLY;
    for (auto& sttmt : node.SttmtList) {
        if (sttmt) sttmt->accept(*this);
    }
    if (node.BracketsOn) os << C_CLOSE_CURLY;
}

void CodeGenVisitor::visit(FuncArgs& node) {
    for (size_t i = 0; i < node.Args.size(); ++i) {
        if (node.Args[i]) node.Args[i]->accept(*this);
        if (i < node.Args.size() - 1) os << C_COMMA;
    }
}

void CodeGenVisitor::visit(FuncDeclSttmt& node) {
    // localizing the main entry point
    std::string name = node.Name;
    std::string type = translate_type(node.Type);
    if (name == "inisiu") {
        name = "main";
        type = "int";  // C mandates int main()
    }

    os << type << " " << name << C_OPEN_PAR;
    if (node.Args) node.Args->accept(*this);
    os << C_CLOSE_PAR;
    if (node.Body) node.Body->accept(*this);
}

void CodeGenVisitor::visit(IfSttmt& node) {
    os << C_IF << C_OPEN_PAR;
    if (node.Cond) node.Cond->accept(*this);
    os << C_CLOSE_PAR;
    if (node.Then) node.Then->accept(*this);
    if (node.Else) {
        os << " " << C_ELSE << " ";
        node.Else->accept(*this);
    }
}

void CodeGenVisitor::visit(WhileSttmt& node) {
    os << C_WHILE << C_OPEN_PAR;
    if (node.Cond) node.Cond->accept(*this);
    os << C_CLOSE_PAR;
    if (node.Do) node.Do->accept(*this);
}

void CodeGenVisitor::visit(JumpSttmt& node) {
    os << node.Name << C_SEMICOLON;
}

void CodeGenVisitor::visit(ReturnSttmt& node) {
    os << C_RETURN << " ";
    if (node.ReturnValue) node.ReturnValue->accept(*this);
    os << C_SEMICOLON;
}

void CodeGenVisitor::visit(FuncCallArgs& node) {
    for (size_t i = 0; i < node.Args.size(); ++i) {
        if (node.Args[i]) node.Args[i]->accept(*this);
        if (i < node.Args.size() - 1) os << C_COMMA;
    }
}

void CodeGenVisitor::visit(FunCallExpr& node) {
    os << node.Name << C_OPEN_PAR;
    if (node.Args) node.Args->accept(*this);
    os << C_CLOSE_PAR;
}

void CodeGenVisitor::visit(MostraFunCallExpr& node) {
    os << C_PRINTF << C_OPEN_PAR;

    if (!node.Args || node.Args->Args.empty()) {
        os << C_CLOSE_PAR;
        return;
    }

    auto& args = node.Args->Args;

    // If the first argument is a string literal the user is supplying
    // their own format string, pass through unchanged.
    if (auto* first = dynamic_cast<LiteralExpr*>(args[0].get())) {
        if (first->Type == "char*") {
            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i]) args[i]->accept(*this);
                if (i < args.size() - 1) os << C_COMMA;
            }
            os << C_CLOSE_PAR;
            return;
        }
    }

    // Auto-format: build a format string from argument types.
    std::string fmt;
    for (auto& arg : args) {
        std::string spec = inferFormat(arg.get());
        // Fall back to %g (double) when expression type is not statically known.
        fmt += spec.empty() ? "%g" : spec;
    }

    os << "\"" << fmt << "\"";
    for (auto& arg : args) {
        os << C_COMMA;
        if (arg) arg->accept(*this);
    }
    os << C_CLOSE_PAR;
}

void CodeGenVisitor::visit(BinExpr& node) {
    if (node.LHS) node.LHS->accept(*this);
    os << " " << node.Op << " ";
    if (node.RHS) node.RHS->accept(*this);
}

void CodeGenVisitor::visit(LiteralExpr& node) {
    if (node.AutoCast) {
        os << C_OPEN_PAR << translate_type(node.Type) << C_CLOSE_PAR;
    }
    os << node.Value;
}

void CodeGenVisitor::visit(ExprSttmt& node) {
    if (node.Expression) node.Expression->accept(*this);
    os << C_SEMICOLON;
}

void CodeGenVisitor::visit(IdentExpr& node) {
    os << node.Name;
}

void CodeGenVisitor::visit(ParExpr& node) {
    os << C_OPEN_PAR;
    if (node.Content) node.Content->accept(*this);
    os << C_CLOSE_PAR;
}

void CodeGenVisitor::visit(AssignExpr& node) {
    if (node.Assignee) node.Assignee->accept(*this);
    os << C_EQ;
    if (node.Assigned) node.Assigned->accept(*this);
}

void CodeGenVisitor::visit(ForSttmt& node) {
    os << C_FOR << C_OPEN_PAR;
    if (node.Start) node.Start->accept(*this);
    os << C_SEMICOLON;
    if (node.Cond) node.Cond->accept(*this);
    os << C_SEMICOLON;
    if (node.After) node.After->accept(*this);
    os << C_CLOSE_PAR;
    if (node.Then) node.Then->accept(*this);
}

void CodeGenVisitor::visit(ImportSttmt& node) {
    os << C_IMPORT << " " << node.Import << "\n";
}
