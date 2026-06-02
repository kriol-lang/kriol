%{
    #include "include/kriol/ast.hh"
    #include "include/kriol/cli.hh"

    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <memory>

    extern int yylex();
    extern int yylineno;
    void yyerror(kriol::ast::BlockSttmt** Program, const char* err);

    using namespace kriol;
%}

%parse-param { kriol::ast::BlockSttmt** Program }

%union {
    int token;
    std::string* integer;
    std::string* floatingpoint;
    std::string* string;
    kriol::ast::Expr* expr;
    kriol::ast::Sttmt* sttmt;
    kriol::ast::BlockSttmt* block;
    kriol::ast::VarDeclSttmt* vardecl;
    kriol::ast::FuncArgs* params;
    kriol::ast::FuncCallArgs* args;
    kriol::ast::LiteralExpr* litexpr;
    std::vector<kriol::ast::StructField>* struct_fields;
    std::vector<kriol::ast::StructLiteralExpr::FieldInit>* struct_inits;
    std::vector<std::unique_ptr<kriol::ast::FuncDeclSttmt>>* struct_methods;
}

%destructor { delete $$; } <string> <integer> <floatingpoint>
%destructor { delete $$; } <expr> <sttmt> <block> <vardecl> <params> <args> <litexpr>
%destructor { delete $$; } <struct_fields> <struct_inits> <struct_methods>

%token<string> IDENT "valid identifier" STR_LIT "string literal" FSTR_TEXT "f-string text"
%token<token> MOSTRA "mostra" MOSTRAN "mostran"
%token<integer> INT_LIT "integer literal"
%token<floatingpoint> FLOAT_LIT "floating-point literal "
%token<string> BOOL_LIT "boolean literal"
%token<token>  PLUS "+" MINUS "-" MUL "*" DIV "/"
%token<token>  EQ "=="  NE "!="  LT "<" LE "<=" GT ">" GE ">="
%token<token>  AND "&&" OR "||" ASSIGN "=" LCURLY "{" RCURLY "}" COMMA "," SEMIC ";" LBRAC "[" RBRAC "]"
%token<string> TYPE_NUM TYPE_BOOL TYPE_NTER TYPE_TEXTU
%token<token>  DIVOLVI "divolvi" PA "pa"
%token<token>  NKUANTU "nkuantu" SI "si" SINON "sinon" IMPRISTAN "inpristan"
%token<token>  PARA "para" CONTINUA "kontinua" DOT "." RPAR ")" LPAR "("
%token<token>  FN "fn" NOT "!" SAI "sai" KONFIRMA "konfirma" DIPOZ "dipoz"
%token<token>  FSTR_START "f-string" FSTR_END "end of f-string" FSTR_LBRACE "start of interpolation" FSTR_RBRACE "end of interpolation"
%token<token>  KOMU "komu" NADA "nada" MOLDA "molda" IMPL "impl" AMP "&" COLON ":"

%type<expr> expression assignment_expression primary_expression unary_expression initializer
            constant_expression constant logical_or_expressions logical_and_expressions
            equality_expression relational_expression additive_expression multiplicative_expression
            mostra_func_call fstring fstring_parts array_initializer array_initializer_elements value_expression
            struct_literal
%type<sttmt> expression_statement selection_statement iteration_statement jump_statement
             function_declaration declaration statement import_statement
             struct_declaration impl_declaration
%type<block> compound_statement statements else_then
%type<string> declarator identifier type_specifier type_name assignment_operator single_import
%type<vardecl> parameter_declaration array_declarator
%type<params> parameter_list parameter_optional_list
%type<args> argument_list
%type<struct_fields> struct_field_list struct_fields
%type<struct_inits> struct_init_list struct_inits
%type<struct_methods> struct_method_list struct_methods


%left LT GT LE GE EQ NE ASSIGN
%left PLUS MINUS
%left MUL DIV
%right NOT UMINUS
%start program

%define parse.error verbose

%%

program : statements { *Program = $1; }
        | error { *Program = nullptr; }
        ;

type_specifier : TYPE_NUM { $$ = $1; }
               | TYPE_BOOL { $$ = $1; }
               | TYPE_NTER { $$ = $1; }
               | TYPE_TEXTU { $$ = $1; }
               ;

type_name : type_specifier { $$ = $1; }
          | IDENT { $$ = $1; }
          | type_name MUL { $$ = new std::string(*$1 + "*"); delete $1; }
          ;

constant : INT_LIT   { auto lit = new ast::LiteralExpr("nter",  *$1); lit->LineNum = yylineno; $$ = lit; delete $1; }
          | FLOAT_LIT { auto lit = new ast::LiteralExpr("num",   *$1); lit->LineNum = yylineno; $$ = lit; delete $1; }
          | BOOL_LIT  { auto lit = new ast::LiteralExpr("bool",  *$1); lit->LineNum = yylineno; $$ = lit; delete $1; }
          | STR_LIT   { auto lit = new ast::LiteralExpr("char*", *$1); lit->LineNum = yylineno; $$ = lit; delete $1; }
          | NADA      { auto lit = new ast::LiteralExpr("nada", "nada"); lit->LineNum = yylineno; $$ = lit; }
          | fstring   { $$ = $1; }
          ;

fstring : FSTR_START fstring_parts FSTR_END { $$ = $2; }
        ;

fstring_parts : %empty { auto fs = new ast::FStringExpr(); fs->LineNum = yylineno; $$ = fs; }
              | fstring_parts FSTR_TEXT { static_cast<ast::FStringExpr*>($1)->addText(*$2); delete $2; $$ = $1; }
              | fstring_parts FSTR_LBRACE constant_expression FSTR_RBRACE { static_cast<ast::FStringExpr*>($1)->addExpr(std::unique_ptr<ast::Expr>($3)); $$ = $1; }
              ;

identifier : IDENT { $$ = $1; }
           ;

declarator : identifier { $$ = $1; }
           ;

declaration : type_name declarator ASSIGN initializer SEMIC { auto d = new ast::VarDeclSttmt(*$1, *$2, std::unique_ptr<ast::Expr>($4)); d->LineNum = yylineno; $$ = d; delete $1; delete $2; }
            | type_name array_declarator ASSIGN initializer SEMIC { $2->SetType(*$1 + "[" + std::to_string($2->ArraySize) + "]"); $2->Value = std::unique_ptr<ast::Expr>($4); $2->LineNum = yylineno; $$ = $2; delete $1; }
            | DIPOZ type_name declarator SEMIC { auto d = new ast::VarDeclSttmt(*$2, *$3, nullptr); d->LineNum = yylineno; $$ = d; delete $2; delete $3; }
            | DIPOZ type_name array_declarator SEMIC { $3->SetType(*$2 + "[" + std::to_string($3->ArraySize) + "]"); $3->LineNum = yylineno; $$ = $3; delete $2; }
            ;

array_declarator : LBRAC INT_LIT RBRAC declarator { $$ = new ast::VarDeclSttmt("void", *$4, nullptr); $$->IsArray = true; $$->ArraySize = std::stoul(*$2); delete $2; delete $4; }
                 ;

initializer : expression { $$ = $1; }
            | array_initializer { $$ = $1; }
            | array_initializer MUL INT_LIT { auto* lit = static_cast<ast::ArrayLiteralExpr*>($1); auto fill = std::move(lit->Elements[0]); delete lit; auto n = new ast::ArrayRepeatExpr(std::move(fill), std::stoul(*$3)); n->LineNum = yylineno; $$ = n; delete $3; }
            ;

value_expression : constant_expression { $$ = $1; }
                 ;

array_initializer : LBRAC array_initializer_elements RBRAC { $$ = $2; }
                  ;

array_initializer_elements : value_expression { auto n = new ast::ArrayLiteralExpr(); n->LineNum = yylineno; n->addElement(std::unique_ptr<ast::Expr>($1)); $$ = n; }
                           | array_initializer_elements COMMA value_expression { static_cast<ast::ArrayLiteralExpr*>($1)->addElement(std::unique_ptr<ast::Expr>($3)); $$ = $1; }
                           ;

expression : assignment_expression { $$ = $1; }
           ;

constant_expression : logical_or_expressions { $$ = $1; }
                    ;

logical_or_expressions : logical_and_expressions { $$ = $1; }
                       | logical_or_expressions OR logical_and_expressions { auto n = new ast::BinExpr("||", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                       ;

logical_and_expressions : equality_expression { $$ = $1; }
                        | logical_and_expressions AND equality_expression { auto n = new ast::BinExpr("&&", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                        ;

equality_expression : relational_expression { $$ = $1; }
                    | equality_expression EQ relational_expression { auto n = new ast::BinExpr("==", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                    | equality_expression NE relational_expression { auto n = new ast::BinExpr("!=", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                    ;

relational_expression : additive_expression { $$ = $1; }
                      | relational_expression LT additive_expression { auto n = new ast::BinExpr("<", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                      | relational_expression GT additive_expression { auto n = new ast::BinExpr(">", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                      | relational_expression LE additive_expression { auto n = new ast::BinExpr("<=", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                      | relational_expression GE additive_expression { auto n = new ast::BinExpr(">=", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                      ;

additive_expression : multiplicative_expression { $$ = $1; }
                    | additive_expression PLUS multiplicative_expression { auto n = new ast::BinExpr("+", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                    | additive_expression MINUS multiplicative_expression { auto n = new ast::BinExpr("-", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                    ;

multiplicative_expression : unary_expression { $$ = $1; }
                          | multiplicative_expression MUL unary_expression { auto n = new ast::BinExpr("*", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                          | multiplicative_expression DIV unary_expression { auto n = new ast::BinExpr("/", std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
                          ;

unary_expression : primary_expression                          { $$ = $1; }
                 | NOT unary_expression                         { auto n = new ast::UnaryExpr("!", std::unique_ptr<ast::Expr>($2)); n->LineNum = yylineno; $$ = n; }
                 | MINUS unary_expression %prec UMINUS          { auto n = new ast::UnaryExpr("-", std::unique_ptr<ast::Expr>($2)); n->LineNum = yylineno; $$ = n; }
                 | AMP unary_expression                         { auto n = new ast::UnaryExpr("&", std::unique_ptr<ast::Expr>($2)); n->LineNum = yylineno; $$ = n; }
                 | MUL unary_expression %prec UMINUS            { auto n = new ast::UnaryExpr("*", std::unique_ptr<ast::Expr>($2)); n->LineNum = yylineno; $$ = n; }
                 | unary_expression KOMU type_name              { auto n = new ast::CastExpr(std::unique_ptr<ast::Expr>($1), *$3); n->LineNum = yylineno; $$ = n; delete $3; }
                 ;

primary_expression : IDENT LPAR argument_list RPAR { auto n = new ast::FunCallExpr(*$1, std::unique_ptr<ast::FuncCallArgs>($3)); n->LineNum = yylineno; $$ = n; delete $1; }
                   | IDENT LPAR RPAR               { auto n = new ast::FunCallExpr(*$1, nullptr); n->LineNum = yylineno; $$ = n; delete $1; }
                   | mostra_func_call               { $$ = $1; }
                   | IDENT                          { auto n = new ast::IdentExpr(*$1); n->LineNum = yylineno; $$ = n; delete $1; }
                   | IDENT LBRAC value_expression RBRAC   { auto n = new ast::ArrayAccessExpr(*$1, std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; delete $1; }
                   | constant                       { $$ = $1; }
                   | LPAR expression RPAR           { auto n = new ast::ParExpr(std::unique_ptr<ast::Expr>($2)); n->LineNum = yylineno; $$ = n; }
                   | primary_expression DOT IDENT   { auto n = new ast::FieldAccessExpr(std::unique_ptr<ast::Expr>($1), *$3); n->LineNum = yylineno; $$ = n; delete $3; }
                   | primary_expression DOT IDENT LPAR argument_list RPAR { auto n = new ast::MethodCallExpr(std::unique_ptr<ast::Expr>($1), *$3, std::unique_ptr<ast::FuncCallArgs>($5)); n->LineNum = yylineno; $$ = n; delete $3; }
                   | primary_expression DOT IDENT LPAR RPAR               { auto n = new ast::MethodCallExpr(std::unique_ptr<ast::Expr>($1), *$3, nullptr); n->LineNum = yylineno; $$ = n; delete $3; }
                   ;

assignment_expression : constant_expression { $$ = $1; }
                      | struct_literal { $$ = $1; }
                      | primary_expression assignment_operator assignment_expression { auto n = new ast::AssignExpr(*$2, std::unique_ptr<ast::Expr>($1), std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; delete $2; }
                      ;

assignment_operator : ASSIGN { $$ = new std::string("=", 2); }
                    ;

function_declaration : FN declarator LPAR parameter_optional_list RPAR type_name compound_statement { auto n = new ast::FuncDeclSttmt(*$6, *$2, std::unique_ptr<ast::FuncArgs>($4), std::unique_ptr<ast::BlockSttmt>($7)); n->LineNum = yylineno; $$ = n; delete $6; delete $2; }
                     | FN declarator LPAR parameter_optional_list RPAR compound_statement { auto n = new ast::FuncDeclSttmt("vaziu", *$2, std::unique_ptr<ast::FuncArgs>($4), std::unique_ptr<ast::BlockSttmt>($6)); n->LineNum = yylineno; $$ = n; delete $2; }
                     ;

parameter_optional_list : parameter_list { $$ = $1; }
                        | %empty { $$ = new ast::FuncArgs(); }
                        ;

parameter_list : parameter_declaration { $$ = new ast::FuncArgs(); $$->AddArg(std::unique_ptr<ast::VarDeclSttmt>($1)); }
               | parameter_list COMMA parameter_declaration { $1->AddArg(std::unique_ptr<ast::VarDeclSttmt>($3)); $$ = $1; }
               ;

parameter_declaration : type_name declarator { $$ = new ast::VarDeclSttmt(*$1, *$2, nullptr); $$->IsParam = true; $$->LineNum = yylineno; delete $1; delete $2; }
                      ;

argument_list : argument_list COMMA expression { $1->AddArg(std::unique_ptr<ast::Expr>($3)); $$ = $1; }
              | expression { $$ = new ast::FuncCallArgs(); $$->AddArg(std::unique_ptr<ast::Expr>($1)); }
              ;

mostra_func_call : MOSTRA LPAR argument_list RPAR { auto n = new ast::MostraFunCallExpr(std::unique_ptr<ast::FuncCallArgs>($3)); n->LineNum = yylineno; $$ = n; }
                 | MOSTRA LPAR RPAR { auto n = new ast::MostraFunCallExpr(nullptr); n->LineNum = yylineno; $$ = n; }
                 | MOSTRAN LPAR argument_list RPAR { auto n = new ast::MostraFunCallExpr(std::unique_ptr<ast::FuncCallArgs>($3), true); n->LineNum = yylineno; $$ = n; }
                 | MOSTRAN LPAR RPAR { auto n = new ast::MostraFunCallExpr(nullptr, true); n->LineNum = yylineno; $$ = n; }
                 ;

statements : statements statement { $1->AddSttmt(std::unique_ptr<ast::Sttmt>($2)); $$ = $1; }
           | statement { $$ = new ast::BlockSttmt(); $$->AddSttmt(std::unique_ptr<ast::Sttmt>($1)); }
           ;

statement : expression_statement { $$ = $1; }
          | compound_statement { $$ = $1; }
          | selection_statement { $$ = $1; }
          | iteration_statement { $$ = $1; }
          | jump_statement { $$ = $1; }
          | function_declaration { $$ = $1; }
          | declaration { $$ = $1; }
          | import_statement { $$ = $1; }
          | struct_declaration { $$ = $1; }
          | impl_declaration { $$ = $1; }
          | error SEMIC { $$ = nullptr; }
          ;

import_statement : IMPRISTAN single_import { $$ = new ast::ImportSttmt(*$2); delete $2; }
                 ;

single_import : STR_LIT { $$ = $1; }
              ;

expression_statement : expression SEMIC { $$ = new ast::ExprSttmt(std::unique_ptr<ast::Expr>($1)); }
                     | SEMIC { $$ = new ast::ExprSttmt(nullptr); }
                     ;

compound_statement : LCURLY statements RCURLY { $$ = $2; $$->UseBrackets(); }
                   | LCURLY RCURLY { $$ = new ast::BlockSttmt(); $$->UseBrackets(); }
                   ;

selection_statement : SI constant_expression compound_statement { auto n = new ast::IfSttmt(std::unique_ptr<ast::Expr>($2), std::unique_ptr<ast::BlockSttmt>($3), nullptr); n->LineNum = yylineno; $$ = n; }
                    | SI constant_expression compound_statement SINON else_then { auto n = new ast::IfSttmt(std::unique_ptr<ast::Expr>($2), std::unique_ptr<ast::BlockSttmt>($3), std::unique_ptr<ast::BlockSttmt>($5)); n->LineNum = yylineno; $$ = n; }
                    ;

else_then : compound_statement { $$ = $1; }
          | selection_statement { $$ = new ast::BlockSttmt(); $$->AddSttmt(std::unique_ptr<ast::Sttmt>($1)); }
          ;

iteration_statement : NKUANTU constant_expression compound_statement { auto n = new ast::WhileSttmt(std::unique_ptr<ast::Expr>($2), std::unique_ptr<ast::BlockSttmt>($3)); n->LineNum = yylineno; $$ = n; }
                    | PA expression SEMIC constant_expression SEMIC expression compound_statement { auto n = new ast::ForSttmt(std::unique_ptr<ast::Expr>($2), std::unique_ptr<ast::Expr>($4), std::unique_ptr<ast::Expr>($6), std::unique_ptr<ast::BlockSttmt>($7)); n->LineNum = yylineno; $$ = n; }
                    ;

jump_statement : PARA SEMIC { auto n = new ast::JumpSttmt("break"); n->LineNum = yylineno; $$ = n; }
               | CONTINUA SEMIC { auto n = new ast::JumpSttmt("continue"); n->LineNum = yylineno; $$ = n; }
               | DIVOLVI expression SEMIC { auto n = new ast::ReturnSttmt(std::unique_ptr<ast::Expr>($2)); n->LineNum = yylineno; $$ = n; }
               | DIVOLVI SEMIC { auto n = new ast::ReturnSttmt(nullptr); n->LineNum = yylineno; $$ = n; }
               | SAI LPAR expression RPAR SEMIC { auto n = new ast::SaiSttmt(std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
               | SAI LPAR RPAR SEMIC { auto n = new ast::SaiSttmt(nullptr); n->LineNum = yylineno; $$ = n; }
               | KONFIRMA LPAR expression RPAR SEMIC { auto n = new ast::KonfirmaSttmt(std::unique_ptr<ast::Expr>($3)); n->LineNum = yylineno; $$ = n; }
               | KONFIRMA LPAR RPAR SEMIC { auto n = new ast::KonfirmaSttmt(nullptr); n->LineNum = yylineno; $$ = n; }
               ;

struct_declaration : MOLDA IDENT LCURLY struct_fields RCURLY { $$ = new ast::StructDeclSttmt(*$2, *$4); $$->LineNum = yylineno; delete $2; delete $4; }
                   ;

struct_fields : %empty { $$ = new std::vector<ast::StructField>(); }
              | struct_field_list { $$ = $1; }
              ;

struct_field_list : type_name declarator { $$ = new std::vector<ast::StructField>(); $$->push_back(ast::StructField(*$1, *$2)); delete $1; delete $2; }
                  | struct_field_list COMMA type_name declarator { $1->push_back(ast::StructField(*$3, *$4)); $$ = $1; delete $3; delete $4; }
                  | struct_field_list COMMA { $$ = $1; }
                  ;

impl_declaration : IMPL IDENT LCURLY struct_methods RCURLY { $$ = new ast::ImplSttmt(*$2, std::move(*$4)); $$->LineNum = yylineno; delete $2; delete $4; }
                 ;

struct_methods : %empty { $$ = new std::vector<std::unique_ptr<ast::FuncDeclSttmt>>(); }
               | struct_method_list { $$ = $1; }
               ;

struct_method_list : function_declaration { $$ = new std::vector<std::unique_ptr<ast::FuncDeclSttmt>>(); $$->push_back(std::unique_ptr<ast::FuncDeclSttmt>(static_cast<ast::FuncDeclSttmt*>($1))); }
                   | struct_method_list function_declaration { $1->push_back(std::unique_ptr<ast::FuncDeclSttmt>(static_cast<ast::FuncDeclSttmt*>($2))); $$ = $1; }
                   ;

struct_literal : IDENT LCURLY struct_inits RCURLY { $$ = new ast::StructLiteralExpr(*$1, std::move(*$3)); $$->LineNum = yylineno; delete $1; delete $3; }
               ;

struct_inits : %empty { $$ = new std::vector<ast::StructLiteralExpr::FieldInit>(); }
             | struct_init_list { $$ = $1; }
             ;

struct_init_list : IDENT COLON expression { $$ = new std::vector<ast::StructLiteralExpr::FieldInit>(); $$->push_back({*$1, std::unique_ptr<ast::Expr>($3)}); delete $1; }
                 | struct_init_list COMMA IDENT COLON expression { $1->push_back({*$3, std::unique_ptr<ast::Expr>($5)}); $$ = $1; delete $3; }
                 ;
%%

void yyerror(kriol::ast::BlockSttmt** Program, const char* err) {
    kriol::cli::PrintErr(kriol::cli::GetSourceFile(), yylineno, err, 1);
}
