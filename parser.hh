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
    IDENT = 258,                   /* IDENT  */
    STR_LIT = 259,                 /* STR_LIT  */
    MOSTRA = 260,                  /* MOSTRA  */
    MOSTRAN = 261,                 /* MOSTRAN  */
    INT_LIT = 262,                 /* INT_LIT  */
    FLOAT_LIT = 263,               /* FLOAT_LIT  */
    BOOL_LIT = 264,                /* BOOL_LIT  */
    PLUS = 265,                    /* PLUS  */
    MINUS = 266,                   /* MINUS  */
    MUL = 267,                     /* MUL  */
    DIV = 268,                     /* DIV  */
    EQ = 269,                      /* EQ  */
    NE = 270,                      /* NE  */
    LT = 271,                      /* LT  */
    LE = 272,                      /* LE  */
    GT = 273,                      /* GT  */
    GE = 274,                      /* GE  */
    AND = 275,                     /* AND  */
    OR = 276,                      /* OR  */
    ASSIGN = 277,                  /* ASSIGN  */
    LCURLY = 278,                  /* LCURLY  */
    RCURLY = 279,                  /* RCURLY  */
    COMMA = 280,                   /* COMMA  */
    SEMIC = 281,                   /* SEMIC  */
    LBRAC = 282,                   /* LBRAC  */
    RBRAC = 283,                   /* RBRAC  */
    TYPE_NUM = 284,                /* TYPE_NUM  */
    TYPE_BOOL = 285,               /* TYPE_BOOL  */
    TYPE_VOID = 286,               /* TYPE_VOID  */
    TYPE_NTER = 287,               /* TYPE_NTER  */
    TYPE_TEXTU = 288,              /* TYPE_TEXTU  */
    DIVOLVI = 289,                 /* DIVOLVI  */
    PA = 290,                      /* PA  */
    STRUT = 291,                   /* STRUT  */
    NKUANTU = 292,                 /* NKUANTU  */
    SI = 293,                      /* SI  */
    SINON = 294,                   /* SINON  */
    IMPRISTAN = 295,               /* IMPRISTAN  */
    PARA = 296,                    /* PARA  */
    CONTINUA = 297,                /* CONTINUA  */
    DOT = 298,                     /* DOT  */
    RPAR = 299,                    /* RPAR  */
    LPAR = 300,                    /* LPAR  */
    FN = 301,                      /* FN  */
    UMINUS = 302                   /* UMINUS  */
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
    std::string* boolean;
    std::string* string;
    kriol::ast::Expr* expr;
    kriol::ast::Sttmt* sttmt;
    kriol::ast::BlockSttmt* block;
    kriol::ast::VarDeclSttmt* vardecl;
    kriol::ast::FuncArgs* params;
    kriol::ast::FuncCallArgs* args;
    kriol::ast::LiteralExpr* litexpr;

#line 126 "parser.hh"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (kriol::ast::BlockSttmt** Program);


#endif /* !YY_YY_PARSER_HH_INCLUDED  */
