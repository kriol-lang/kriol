/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_HH_INCLUDED
# define YY_YY_PARSER_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    IDENT = 258,                   /* "valid identifier"  */
    STR_LIT = 259,                 /* "string literal"  */
    FSTR_TEXT = 260,               /* "f-string text"  */
    MOSTRA = 261,                  /* "mostra"  */
    MOSTRAN = 262,                 /* "mostran"  */
    INT_LIT = 263,                 /* "integer literal"  */
    FLOAT_LIT = 264,               /* "floating-point literal "  */
    BOOL_LIT = 265,                /* "boolean literal"  */
    PLUS = 266,                    /* "+"  */
    MINUS = 267,                   /* "-"  */
    MUL = 268,                     /* "*"  */
    DIV = 269,                     /* "/"  */
    EQ = 270,                      /* "=="  */
    NE = 271,                      /* "!="  */
    LT = 272,                      /* "<"  */
    LE = 273,                      /* "<="  */
    GT = 274,                      /* ">"  */
    GE = 275,                      /* ">="  */
    PLUS_ASSIGN = 276,             /* "+="  */
    MINUS_ASSIGN = 277,            /* "-="  */
    MUL_ASSIGN = 278,              /* "*="  */
    DIV_ASSIGN = 279,              /* "/="  */
    AND = 280,                     /* "&&"  */
    OR = 281,                      /* "||"  */
    ASSIGN = 282,                  /* "="  */
    LCURLY = 283,                  /* "{"  */
    RCURLY = 284,                  /* "}"  */
    COMMA = 285,                   /* ","  */
    SEMIC = 286,                   /* ";"  */
    LBRAC = 287,                   /* "["  */
    RBRAC = 288,                   /* "]"  */
    TYPE_NUM = 289,                /* TYPE_NUM  */
    TYPE_BOOL = 290,               /* TYPE_BOOL  */
    TYPE_VOID = 291,               /* TYPE_VOID  */
    TYPE_NTER = 292,               /* TYPE_NTER  */
    TYPE_TEXTU = 293,              /* TYPE_TEXTU  */
    DIVOLVI = 294,                 /* "divolvi"  */
    PA = 295,                      /* "pa"  */
    NKUANTU = 296,                 /* "nkuantu"  */
    SI = 297,                      /* "si"  */
    SINON = 298,                   /* "sinon"  */
    IMPRISTAN = 299,               /* "inpristan"  */
    PARA = 300,                    /* "para"  */
    CONTINUA = 301,                /* "kontinua"  */
    DOT = 302,                     /* "."  */
    RPAR = 303,                    /* ")"  */
    LPAR = 304,                    /* "("  */
    FN = 305,                      /* "fn"  */
    NOT = 306,                     /* "!"  */
    SAI = 307,                     /* "sai"  */
    KONFIRMA = 308,                /* "konfirma"  */
    DIPOZ = 309,                   /* "dipoz"  */
    FSTR_START = 310,              /* "f-string"  */
    FSTR_END = 311,                /* "end of f-string"  */
    FSTR_LBRACE = 312,             /* "start of interpolation"  */
    FSTR_RBRACE = 313,             /* "end of interpolation"  */
    UMINUS = 314                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 19 "rules/parser.y"

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

#line 137 "parser.hh"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (kriol::ast::BlockSttmt** Program);


#endif /* !YY_YY_PARSER_HH_INCLUDED  */
