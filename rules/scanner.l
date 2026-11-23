%option noyywrap yylineno stack
%option nounistd never-interactive nounput noinput noyy_top_state

%{
    #include "include/kriol/ast.hh"
    #include "include/kriol/cli.hh"

    #include <string>
    #include <cstdio>
    #include <iostream>
    #include <cstring>
    #include <string_view>
    #include <unordered_map>
    #include <vector>
    #include "parser.hh"
    #include "include/kriol/words.hh"

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

    // One entry per open f-string interpolation: how many `{` inside it are
    // still open, so the `}` of a molda literal does not end the interpolation.
    static std::vector<int> g_fstr_braces;

    // The words the scanner reads as their own token; any other name is an
    // IDENT. kriol --print-words lists them, and syntax/kriol-syntax.json must
    // follow this table. A boolean carries the value its literal stands for.
    struct WordToken {
        std::string_view name;
        int token;
        const char* boolValue = nullptr;
    };

    static constexpr WordToken WordTokens[] = {
        {"fn", FN}, {"molda", MOLDA}, {"inpristan", IMPRISTAN},
        {"si", SI}, {"sinon", SINON}, {"pa", PA}, {"nkuantu", NKUANTU},
        {"kebra", KEBRA}, {"kontinua", CONTINUA}, {"divolvi", DIVOLVI},
        {"lansa", LANSA}, {"spia", SPIA}, {"oops", OOPS}, {"sempri", SEMPRI},

        {"num", TYPE_NUM}, {"int", TYPE_INT}, {"bool", TYPE_BOOL}, {"textu", TYPE_TEXTU},
        {"i8", TYPE_PRIMITIVE}, {"i16", TYPE_PRIMITIVE}, {"i32", TYPE_PRIMITIVE}, {"i64", TYPE_PRIMITIVE},
        {"u8", TYPE_PRIMITIVE}, {"u16", TYPE_PRIMITIVE}, {"u32", TYPE_PRIMITIVE}, {"u64", TYPE_PRIMITIVE},
        {"f32", TYPE_PRIMITIVE}, {"f64", TYPE_PRIMITIVE}, {"isize", TYPE_PRIMITIVE}, {"usize", TYPE_PRIMITIVE},

        {"sin", BOOL_LIT, "1"}, {"nau", BOOL_LIT, "0"},
    };

    static kriol::words::Kind KindOf(int token)
    {
        switch (token) {
            case BOOL_LIT:
                return kriol::words::Kind::Boolean;
            case TYPE_NUM: case TYPE_INT: case TYPE_BOOL: case TYPE_TEXTU: case TYPE_PRIMITIVE:
                return kriol::words::Kind::BuiltinType;
            default:
                return kriol::words::Kind::Keyword;
        }
    }

    static const WordToken* FindWord(std::string_view name)
    {
        static const auto words = [] {
            std::unordered_map<std::string_view, const WordToken*> byName;
            for (const auto& word : WordTokens) byName.emplace(word.name, &word);
            return byName;
        }();
        const auto found = words.find(name);
        return found == words.end() ? nullptr : found->second;
    }

    // A lower-case name: one of the words above, or an IDENT.
    static int ScanName(const char* text, int length)
    {
        const WordToken* word = FindWord(std::string_view(text, length));
        if (!word) {
            SAVE_TOKEN;
            return IDENT;
        }
        switch (KindOf(word->token)) {
            case kriol::words::Kind::Boolean:
                yylval.string = new std::string(word->boolValue);
                return word->token;
            case kriol::words::Kind::BuiltinType:
                SAVE_TOKEN;
                return word->token;
            case kriol::words::Kind::Keyword:
                break;
        }
        return TOKENIZE(word->token);
    }
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
"/*"([^*]|\*+[^*/])*\**     { SCAN_ERROR("o comentário não foi fechado; falta '*/'"); }

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
"{"             { if (!g_fstr_braces.empty()) ++g_fstr_braces.back(); return TOKENIZE(LCURLY); }
"}"             {
                    if (!g_fstr_braces.empty()) {
                        if (g_fstr_braces.back() == 0) { g_fstr_braces.pop_back(); yy_pop_state(); return TOKENIZE(FSTR_RBRACE); }
                        --g_fstr_braces.back();
                    }
                    return TOKENIZE(RCURLY);
                }
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
{IDENT}         return ScanName(yytext, yyleng);
f["]            { yy_push_state(FSTRING_DQ); return TOKENIZE(FSTR_START); }
f[']            { yy_push_state(FSTRING_SQ); return TOKENIZE(FSTR_START); }
{STRING_DQ}     SAVE_TOKEN;   return STR_LIT;
{STRING_SQ}     SAVE_TOKEN;   return STR_LIT;
 /* A text without its closing quote: report it, then go on as if it were closed. */
["]([^"\\\n]|\\.)*  { SCAN_ERROR("o texto não foi fechado; falta a aspa final \""); yylval.string = new std::string(std::string(yytext) + "\""); return STR_LIT; }
[']([^'\\\n]|\\.)*  { SCAN_ERROR("o texto não foi fechado; falta a aspa final '"); yylval.string = new std::string(std::string(yytext) + "'"); return STR_LIT; }

<FSTRING_DQ>{
    "\""                  { yy_pop_state(); return TOKENIZE(FSTR_END); }
    "{"                   { g_fstr_braces.push_back(0); yy_push_state(INITIAL); return TOKENIZE(FSTR_LBRACE); }
    ([^"{\\\n]|\\.)+      { SAVE_TOKEN; return FSTR_TEXT; }
    <<EOF>>               { SCAN_ERROR("o texto interpolado não foi fechado; falta a aspa final"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    \n                    { SCAN_ERROR("o texto interpolado não pode continuar na linha seguinte; falta a aspa final?"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    .                     { SCAN_ERROR("o carácter '" + std::string(yytext) + "' não é válido neste texto interpolado"); }
}

<FSTRING_SQ>{
    "'"                   { yy_pop_state(); return TOKENIZE(FSTR_END); }
    "{"                   { g_fstr_braces.push_back(0); yy_push_state(INITIAL); return TOKENIZE(FSTR_LBRACE); }
    ([^'{\\\n]|\\.)+      { SAVE_TOKEN; return FSTR_TEXT; }
    <<EOF>>               { SCAN_ERROR("o texto interpolado não foi fechado; falta a aspa final"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    \n                    { SCAN_ERROR("o texto interpolado não pode continuar na linha seguinte; falta a aspa final?"); yy_pop_state(); return TOKENIZE(FSTR_END); }
    .                     { SCAN_ERROR("o carácter '" + std::string(yytext) + "' não é válido neste texto interpolado"); }
}

[\xC0-\xFF][\x80-\xBF]* { SCAN_ERROR("o carácter '" + std::string(yytext) + "' só pode aparecer dentro de textos; os nomes usam apenas letras sem acentos"); }
.               { SCAN_ERROR("o símbolo '" + std::string(yytext) + "' não faz parte da linguagem"); }

%%

void kriol_scanner_reset_state(void)
{
    g_fstr_braces.clear();
    yy_start_stack_ptr = 0;
    BEGIN(INITIAL);
}

const std::vector<kriol::words::ScannerWord>& kriol::words::scannerWords()
{
    static const auto words = [] {
        std::vector<ScannerWord> list;
        for (const auto& word : WordTokens) list.push_back({word.name, KindOf(word.token)});
        return list;
    }();
    return words;
}
