%option noyywrap yylineno stack
%option nounistd never-interactive nounput noinput noyy_top_state

%{
    #include "include/kriol/ast.hh"
    #include "include/kriol/cli.hh"

    #include <string>
    #include <cstdio>
    #include <iostream>
    #include <cstring>
    #include "parser.hh"

    #define SAVE_TOKEN     yylval.string  = new std::string(yytext, yyleng)
    #define TOKENIZE(sym) (yylval.token   = sym)

    // yylineno already counts the token's own newlines when this runs.
    #define YY_USER_ACTION                                  \
        {                                                   \
            int newlines = 0;                               \
            for (int i = 0; i < yyleng; ++i)                \
                newlines += yytext[i] == '\n';              \
            yylloc.first_line = yylineno - newlines;        \
            yylloc.last_line = yylineno;                    \
        }

    #define SCAN_ERROR(message) kriol::cli::ReportParseError(yylloc.first_line, message)

    static int g_fstr_depth = 0;
%}

INT [0-9]+

FLOAT {INT}\.[0-9]+

TYPE_IDENT [A-Z][a-zA-Z0-9_]*
IDENT [a-z_][a-zA-Z0-9_]*

STRING_DQ   ["]([^"\\\n]|\\.|\\\r?\n)*["]
STRING_SQ   [']([^'\\\n]|\\.|\\\r?\n)*[']

%x FSTRING_DQ FSTRING_SQ

%%

[ \t\r\n]      /* ignore */;
"//"[^\n]*      /* single-line comment */;
"/*"([^*]|\*+[^*/])*\*+"/"  /* multi-line comment */;

"fn"            return TOKENIZE(FN);
"molda"         return TOKENIZE(MOLDA);
"inpristan"     return TOKENIZE(IMPRISTAN);
"si"            return TOKENIZE(SI);
"sinon"         return TOKENIZE(SINON);
"divolvi"       return TOKENIZE(DIVOLVI);
"pa"            return TOKENIZE(PA);
"nkuantu"       return TOKENIZE(NKUANTU);
"tenta"         return TOKENIZE(TENTA);
"lansa"         return TOKENIZE(LANSA);
"num"           SAVE_TOKEN;   return TYPE_NUM;
"int"           SAVE_TOKEN;   return TYPE_INT;
"bool"          SAVE_TOKEN;   return TYPE_BOOL;
"textu"         SAVE_TOKEN;   return TYPE_TEXTU;
"i8"            SAVE_TOKEN;   return TYPE_PRIMITIVE;
"i16"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"i32"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"i64"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"u8"            SAVE_TOKEN;   return TYPE_PRIMITIVE;
"u16"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"u32"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"u64"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"f32"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"f64"           SAVE_TOKEN;   return TYPE_PRIMITIVE;
"isize"         SAVE_TOKEN;   return TYPE_PRIMITIVE;
"usize"         SAVE_TOKEN;   return TYPE_PRIMITIVE;
"sin"           { yylval.string = new std::string("1"); return BOOL_LIT; }
"nau"           { yylval.string = new std::string("0"); return BOOL_LIT; }
"kebra"         return TOKENIZE(KEBRA);
"kontinua"      return TOKENIZE(CONTINUA);
"+"             return TOKENIZE(PLUS);
"-"             return TOKENIZE(MINUS);
"*"             return TOKENIZE(MUL);
"/"             return TOKENIZE(DIV);
"%"             return TOKENIZE(MOD);
"%="            return TOKENIZE(MOD_ASSIGN);
"("             return TOKENIZE(LPAR);
")"             return TOKENIZE(RPAR);
"."             return TOKENIZE(DOT);
"::"            return TOKENIZE(COLONCOLON);
":"             return TOKENIZE(COLON);
","             return TOKENIZE(COMMA);
";"             return TOKENIZE(SEMIC);
"{"             return TOKENIZE(LCURLY);
"}"             { if (g_fstr_depth > 0) { --g_fstr_depth; yy_pop_state(); return TOKENIZE(FSTR_RBRACE); } return TOKENIZE(RCURLY); }
"["             return TOKENIZE(LBRAC);
"]"             return TOKENIZE(RBRAC);
"="             return TOKENIZE(ASSIGN);
"=="            return TOKENIZE(EQ);
"!="            return TOKENIZE(NE);
"!"             return TOKENIZE(NOT);
">"             return TOKENIZE(GT);
">="            return TOKENIZE(GE);
"<"             return TOKENIZE(LT);
"<="            return TOKENIZE(LE);
"&&"            return TOKENIZE(AND);
"||"            return TOKENIZE(OR);
"+="            return TOKENIZE(PLUS_ASSIGN);
"-="            return TOKENIZE(MINUS_ASSIGN);
"*="            return TOKENIZE(MUL_ASSIGN);
"/="            return TOKENIZE(DIV_ASSIGN);
"&"             return TOKENIZE(BIT_AND);
"|"             return TOKENIZE(BIT_OR);
"^"             return TOKENIZE(BIT_XOR);
"~"             return TOKENIZE(BIT_NOT);
"&="            return TOKENIZE(AND_ASSIGN);
"|="            return TOKENIZE(OR_ASSIGN);
"^="            return TOKENIZE(XOR_ASSIGN);

{INT}           SAVE_TOKEN;   return INT_LIT;
{FLOAT}         SAVE_TOKEN;   return FLOAT_LIT;
{TYPE_IDENT}    SAVE_TOKEN;   return TYPE_IDENT;
{IDENT}         SAVE_TOKEN;   return IDENT;
f["]            { yy_push_state(FSTRING_DQ); return TOKENIZE(FSTR_START); }
f[']            { yy_push_state(FSTRING_SQ); return TOKENIZE(FSTR_START); }
{STRING_DQ}     SAVE_TOKEN;   return STR_LIT;
{STRING_SQ}     SAVE_TOKEN;   return STR_LIT;

<FSTRING_DQ>{
    "\""                  { yy_pop_state(); return TOKENIZE(FSTR_END); }
    "{"                   { ++g_fstr_depth; yy_push_state(INITIAL); return TOKENIZE(FSTR_LBRACE); }
    ([^"{\\\n]|\\.)+      { SAVE_TOKEN; return FSTR_TEXT; }
    <<EOF>>               { SCAN_ERROR("unterminated f-string"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    \n                    { SCAN_ERROR("raw newline inside f-string (missing closing quote?)"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    .                     { SCAN_ERROR("unexpected character '" + std::string(yytext) + "' inside f-string"); }
}

<FSTRING_SQ>{
    "'"                   { yy_pop_state(); return TOKENIZE(FSTR_END); }
    "{"                   { ++g_fstr_depth; yy_push_state(INITIAL); return TOKENIZE(FSTR_LBRACE); }
    ([^'{\\\n]|\\.)+      { SAVE_TOKEN; return FSTR_TEXT; }
    <<EOF>>               { SCAN_ERROR("unterminated f-string"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    \n                    { SCAN_ERROR("raw newline inside f-string (missing closing quote?)"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    .                     { SCAN_ERROR("unexpected character '" + std::string(yytext) + "' inside f-string"); }
}

[\xC0-\xFF][\x80-\xBF]* { SCAN_ERROR("unknown character '" + std::string(yytext) + "'; names and keywords use ASCII letters"); }
.               { SCAN_ERROR("unknown token '" + std::string(yytext) + "'"); }

%%

void kriol_scanner_reset_state(void)
{
    g_fstr_depth = 0;
    yy_start_stack_ptr = 0;
    BEGIN(INITIAL);
}
