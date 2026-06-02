/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "rules/parser.y"

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

#line 87 "parser.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.hh"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENT = 3,                      /* "valid identifier"  */
  YYSYMBOL_STR_LIT = 4,                    /* "string literal"  */
  YYSYMBOL_FSTR_TEXT = 5,                  /* "f-string text"  */
  YYSYMBOL_MOSTRA = 6,                     /* "mostra"  */
  YYSYMBOL_MOSTRAN = 7,                    /* "mostran"  */
  YYSYMBOL_INT_LIT = 8,                    /* "integer literal"  */
  YYSYMBOL_FLOAT_LIT = 9,                  /* "floating-point literal "  */
  YYSYMBOL_BOOL_LIT = 10,                  /* "boolean literal"  */
  YYSYMBOL_PLUS = 11,                      /* "+"  */
  YYSYMBOL_MINUS = 12,                     /* "-"  */
  YYSYMBOL_MUL = 13,                       /* "*"  */
  YYSYMBOL_DIV = 14,                       /* "/"  */
  YYSYMBOL_EQ = 15,                        /* "=="  */
  YYSYMBOL_NE = 16,                        /* "!="  */
  YYSYMBOL_LT = 17,                        /* "<"  */
  YYSYMBOL_LE = 18,                        /* "<="  */
  YYSYMBOL_GT = 19,                        /* ">"  */
  YYSYMBOL_GE = 20,                        /* ">="  */
  YYSYMBOL_AND = 21,                       /* "&&"  */
  YYSYMBOL_OR = 22,                        /* "||"  */
  YYSYMBOL_ASSIGN = 23,                    /* "="  */
  YYSYMBOL_LCURLY = 24,                    /* "{"  */
  YYSYMBOL_RCURLY = 25,                    /* "}"  */
  YYSYMBOL_COMMA = 26,                     /* ","  */
  YYSYMBOL_SEMIC = 27,                     /* ";"  */
  YYSYMBOL_LBRAC = 28,                     /* "["  */
  YYSYMBOL_RBRAC = 29,                     /* "]"  */
  YYSYMBOL_TYPE_NUM = 30,                  /* TYPE_NUM  */
  YYSYMBOL_TYPE_BOOL = 31,                 /* TYPE_BOOL  */
  YYSYMBOL_TYPE_NTER = 32,                 /* TYPE_NTER  */
  YYSYMBOL_TYPE_TEXTU = 33,                /* TYPE_TEXTU  */
  YYSYMBOL_DIVOLVI = 34,                   /* "divolvi"  */
  YYSYMBOL_PA = 35,                        /* "pa"  */
  YYSYMBOL_NKUANTU = 36,                   /* "nkuantu"  */
  YYSYMBOL_SI = 37,                        /* "si"  */
  YYSYMBOL_SINON = 38,                     /* "sinon"  */
  YYSYMBOL_IMPRISTAN = 39,                 /* "inpristan"  */
  YYSYMBOL_PARA = 40,                      /* "para"  */
  YYSYMBOL_CONTINUA = 41,                  /* "kontinua"  */
  YYSYMBOL_DOT = 42,                       /* "."  */
  YYSYMBOL_RPAR = 43,                      /* ")"  */
  YYSYMBOL_LPAR = 44,                      /* "("  */
  YYSYMBOL_FN = 45,                        /* "fn"  */
  YYSYMBOL_NOT = 46,                       /* "!"  */
  YYSYMBOL_SAI = 47,                       /* "sai"  */
  YYSYMBOL_KONFIRMA = 48,                  /* "konfirma"  */
  YYSYMBOL_DIPOZ = 49,                     /* "dipoz"  */
  YYSYMBOL_FSTR_START = 50,                /* "f-string"  */
  YYSYMBOL_FSTR_END = 51,                  /* "end of f-string"  */
  YYSYMBOL_FSTR_LBRACE = 52,               /* "start of interpolation"  */
  YYSYMBOL_FSTR_RBRACE = 53,               /* "end of interpolation"  */
  YYSYMBOL_KOMU = 54,                      /* "komu"  */
  YYSYMBOL_NADA = 55,                      /* "nada"  */
  YYSYMBOL_MOLDA = 56,                     /* "molda"  */
  YYSYMBOL_IMPL = 57,                      /* "impl"  */
  YYSYMBOL_AMP = 58,                       /* "&"  */
  YYSYMBOL_COLON = 59,                     /* ":"  */
  YYSYMBOL_UMINUS = 60,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 61,                  /* $accept  */
  YYSYMBOL_program = 62,                   /* program  */
  YYSYMBOL_type_specifier = 63,            /* type_specifier  */
  YYSYMBOL_type_name = 64,                 /* type_name  */
  YYSYMBOL_constant = 65,                  /* constant  */
  YYSYMBOL_fstring = 66,                   /* fstring  */
  YYSYMBOL_fstring_parts = 67,             /* fstring_parts  */
  YYSYMBOL_identifier = 68,                /* identifier  */
  YYSYMBOL_declarator = 69,                /* declarator  */
  YYSYMBOL_declaration = 70,               /* declaration  */
  YYSYMBOL_array_declarator = 71,          /* array_declarator  */
  YYSYMBOL_initializer = 72,               /* initializer  */
  YYSYMBOL_value_expression = 73,          /* value_expression  */
  YYSYMBOL_array_initializer = 74,         /* array_initializer  */
  YYSYMBOL_array_initializer_elements = 75, /* array_initializer_elements  */
  YYSYMBOL_expression = 76,                /* expression  */
  YYSYMBOL_constant_expression = 77,       /* constant_expression  */
  YYSYMBOL_logical_or_expressions = 78,    /* logical_or_expressions  */
  YYSYMBOL_logical_and_expressions = 79,   /* logical_and_expressions  */
  YYSYMBOL_equality_expression = 80,       /* equality_expression  */
  YYSYMBOL_relational_expression = 81,     /* relational_expression  */
  YYSYMBOL_additive_expression = 82,       /* additive_expression  */
  YYSYMBOL_multiplicative_expression = 83, /* multiplicative_expression  */
  YYSYMBOL_unary_expression = 84,          /* unary_expression  */
  YYSYMBOL_primary_expression = 85,        /* primary_expression  */
  YYSYMBOL_assignment_expression = 86,     /* assignment_expression  */
  YYSYMBOL_assignment_operator = 87,       /* assignment_operator  */
  YYSYMBOL_function_declaration = 88,      /* function_declaration  */
  YYSYMBOL_parameter_optional_list = 89,   /* parameter_optional_list  */
  YYSYMBOL_parameter_list = 90,            /* parameter_list  */
  YYSYMBOL_parameter_declaration = 91,     /* parameter_declaration  */
  YYSYMBOL_argument_list = 92,             /* argument_list  */
  YYSYMBOL_mostra_func_call = 93,          /* mostra_func_call  */
  YYSYMBOL_statements = 94,                /* statements  */
  YYSYMBOL_statement = 95,                 /* statement  */
  YYSYMBOL_import_statement = 96,          /* import_statement  */
  YYSYMBOL_single_import = 97,             /* single_import  */
  YYSYMBOL_expression_statement = 98,      /* expression_statement  */
  YYSYMBOL_compound_statement = 99,        /* compound_statement  */
  YYSYMBOL_selection_statement = 100,      /* selection_statement  */
  YYSYMBOL_else_then = 101,                /* else_then  */
  YYSYMBOL_iteration_statement = 102,      /* iteration_statement  */
  YYSYMBOL_jump_statement = 103,           /* jump_statement  */
  YYSYMBOL_struct_declaration = 104,       /* struct_declaration  */
  YYSYMBOL_struct_fields = 105,            /* struct_fields  */
  YYSYMBOL_struct_field_list = 106,        /* struct_field_list  */
  YYSYMBOL_impl_declaration = 107,         /* impl_declaration  */
  YYSYMBOL_struct_methods = 108,           /* struct_methods  */
  YYSYMBOL_struct_method_list = 109,       /* struct_method_list  */
  YYSYMBOL_struct_literal = 110,           /* struct_literal  */
  YYSYMBOL_struct_inits = 111,             /* struct_inits  */
  YYSYMBOL_struct_init_list = 112          /* struct_init_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  101
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   686

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  61
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  52
/* YYNRULES -- Number of rules.  */
#define YYNRULES  136
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  247

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   315


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    84,    84,    85,    88,    89,    90,    91,    94,    95,
      96,    99,   100,   101,   102,   103,   104,   107,   110,   111,
     112,   115,   118,   121,   122,   123,   124,   127,   130,   131,
     132,   135,   138,   141,   142,   145,   148,   151,   152,   155,
     156,   159,   160,   161,   164,   165,   166,   167,   168,   171,
     172,   173,   176,   177,   178,   181,   182,   183,   184,   185,
     186,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   201,   202,   203,   206,   209,   210,   213,   214,   217,
     218,   221,   224,   225,   228,   229,   230,   231,   234,   235,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   251,   254,   257,   258,   261,   262,   265,   266,   269,
     270,   273,   274,   277,   278,   279,   280,   281,   282,   283,
     284,   287,   290,   291,   294,   295,   296,   299,   302,   303,
     306,   307,   310,   313,   314,   317,   318
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "\"valid identifier\"",
  "\"string literal\"", "\"f-string text\"", "\"mostra\"", "\"mostran\"",
  "\"integer literal\"", "\"floating-point literal \"",
  "\"boolean literal\"", "\"+\"", "\"-\"", "\"*\"", "\"/\"", "\"==\"",
  "\"!=\"", "\"<\"", "\"<=\"", "\">\"", "\">=\"", "\"&&\"", "\"||\"",
  "\"=\"", "\"{\"", "\"}\"", "\",\"", "\";\"", "\"[\"", "\"]\"",
  "TYPE_NUM", "TYPE_BOOL", "TYPE_NTER", "TYPE_TEXTU", "\"divolvi\"",
  "\"pa\"", "\"nkuantu\"", "\"si\"", "\"sinon\"", "\"inpristan\"",
  "\"para\"", "\"kontinua\"", "\".\"", "\")\"", "\"(\"", "\"fn\"", "\"!\"",
  "\"sai\"", "\"konfirma\"", "\"dipoz\"", "\"f-string\"",
  "\"end of f-string\"", "\"start of interpolation\"",
  "\"end of interpolation\"", "\"komu\"", "\"nada\"", "\"molda\"",
  "\"impl\"", "\"&\"", "\":\"", "UMINUS", "$accept", "program",
  "type_specifier", "type_name", "constant", "fstring", "fstring_parts",
  "identifier", "declarator", "declaration", "array_declarator",
  "initializer", "value_expression", "array_initializer",
  "array_initializer_elements", "expression", "constant_expression",
  "logical_or_expressions", "logical_and_expressions",
  "equality_expression", "relational_expression", "additive_expression",
  "multiplicative_expression", "unary_expression", "primary_expression",
  "assignment_expression", "assignment_operator", "function_declaration",
  "parameter_optional_list", "parameter_list", "parameter_declaration",
  "argument_list", "mostra_func_call", "statements", "statement",
  "import_statement", "single_import", "expression_statement",
  "compound_statement", "selection_statement", "else_then",
  "iteration_statement", "jump_statement", "struct_declaration",
  "struct_fields", "struct_field_list", "impl_declaration",
  "struct_methods", "struct_method_list", "struct_literal", "struct_inits",
  "struct_init_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-189)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-10)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     401,    -6,     7,  -189,   -14,    16,  -189,  -189,  -189,   615,
     615,   285,  -189,  -189,  -189,  -189,  -189,    34,   628,   615,
     615,    81,    92,   111,   628,    88,   615,    65,    68,    42,
    -189,  -189,   137,   139,   615,   152,  -189,    85,  -189,  -189,
    -189,   126,  -189,   135,   142,    99,     9,   118,   121,   104,
      11,  -189,  -189,  -189,   227,  -189,  -189,  -189,  -189,  -189,
    -189,  -189,  -189,  -189,  -189,  -189,   156,   615,    93,   457,
     470,    27,   104,   122,   104,    -6,  -189,   343,     8,  -189,
     140,   148,   153,   153,  -189,  -189,  -189,  -189,   136,  -189,
    -189,   134,   104,   483,   536,  -189,    85,    14,   157,   159,
     104,  -189,  -189,   172,   161,   162,  -189,   615,   615,   615,
     615,   615,   615,   615,   615,   615,   615,   615,   615,    42,
    -189,   183,   628,  -189,   128,   163,   166,   160,  -189,  -189,
    -189,    36,  -189,    38,  -189,    61,  -189,  -189,   615,  -189,
     155,  -189,    42,   168,   158,   169,   164,   173,   175,  -189,
    -189,   615,    42,   154,   174,   549,   549,   142,    99,     9,
       9,   118,   118,   118,   118,   121,   121,   104,   104,   193,
     167,  -189,   628,  -189,   205,  -189,   628,  -189,  -189,  -189,
     185,    86,    73,   170,   184,  -189,  -189,   187,  -189,   188,
    -189,  -189,   165,    73,   191,   196,  -189,   192,   154,    88,
     615,   197,   206,  -189,   198,   562,  -189,   179,  -189,   628,
    -189,  -189,  -189,  -189,   138,    42,  -189,  -189,  -189,  -189,
    -189,    42,  -189,  -189,  -189,  -189,    82,  -189,   215,  -189,
    -189,    64,   628,   153,    43,  -189,  -189,    73,   615,  -189,
    -189,  -189,  -189,  -189,  -189,  -189,  -189
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     3,    64,    14,     0,     0,    11,    12,    13,     0,
       0,     0,   104,     4,     5,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      18,    15,     0,     0,     0,     0,     8,     0,    66,    16,
      96,     0,    71,    36,    37,    39,    41,    44,    49,    52,
      55,    35,    95,    63,     0,    89,    97,    90,    91,    92,
      93,    94,    98,    99,    72,   100,   133,     0,     0,     0,
       0,    64,    57,    55,    59,     0,   106,     0,    64,   116,
       0,     0,     0,     0,   102,   101,   113,   114,     0,    21,
      22,     0,    56,     0,     0,     9,     0,     0,     0,     0,
      58,     1,    10,     0,     0,     0,   103,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      74,     0,     0,    88,     0,     0,   134,     0,    31,    62,
      83,     0,    85,     0,    87,     0,   105,   115,     0,   111,
     107,    67,    78,     0,     0,     0,     0,     0,     0,    19,
      17,     0,   122,   128,     0,     0,     0,    38,    40,    42,
      43,    45,    47,    46,    48,    50,    51,    53,    54,    60,
      68,    73,     0,   132,     0,    65,     0,    61,    84,    86,
       0,     0,     0,     0,    77,    79,   118,     0,   120,     0,
      25,    26,     0,     0,     0,   123,   130,     0,   129,     0,
       0,     0,    29,    28,     0,     0,   135,     0,    82,     0,
     109,   110,   108,    81,     0,     0,   117,   119,    20,   124,
     121,   126,   127,   131,    27,    33,     0,    23,     0,    24,
      70,     0,     0,     0,     0,    76,    80,     0,     0,    32,
      30,    69,   136,   112,    75,   125,    34
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -189,  -189,  -189,   -24,  -189,  -189,  -189,  -189,   -33,  -189,
     130,    87,  -188,  -189,  -189,   -11,   -18,  -189,   141,   133,
      22,    13,    31,    -1,     0,   107,  -189,  -150,  -189,  -189,
      29,   -55,  -189,   231,   -38,  -189,  -189,  -189,   -60,    66,
    -189,  -189,  -189,  -189,  -189,  -189,  -189,  -189,  -189,  -189,
    -189,  -189
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    35,    36,    37,    38,    39,    97,    90,    91,    40,
     105,   201,   127,   202,   226,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    73,    51,   122,    52,   183,   184,
     185,   131,    53,    54,    55,    56,    85,    57,    58,    59,
     212,    60,    61,    62,   194,   195,    63,   197,   198,    64,
     125,   126
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      50,    82,    83,   196,   104,    96,    80,    81,    72,    74,
      -9,    50,   225,    88,   133,   135,   123,    50,    50,   149,
      -9,    65,   139,   140,    50,    92,   111,   112,   113,   114,
      69,    66,    66,   100,   120,    67,    67,    78,     3,   123,
       4,     5,     6,     7,     8,    95,     9,    10,   223,   128,
     246,    68,    68,   121,    50,    67,   102,   130,   130,   130,
      70,    79,   176,   147,   176,   150,   151,    11,    50,    50,
      50,    68,    13,    14,    15,    16,    89,    50,    24,   177,
      26,   178,   144,   146,    30,    84,   102,   176,    89,    31,
     176,    89,    34,    50,    50,   169,    78,     3,   102,     4,
       5,     6,     7,     8,   179,     9,    10,   241,   238,    93,
      11,   239,    94,   103,   109,   110,   167,   168,   182,    86,
     180,   210,    50,    20,   161,   162,   163,   164,   193,   115,
     116,   159,   160,   192,   117,   118,   129,    24,    87,    26,
      98,    95,    99,    30,   203,   203,   165,   166,    31,   213,
     231,    34,   101,   106,   235,    50,    50,   107,   119,   124,
     219,   206,    11,   108,   121,   208,   224,   137,    13,    14,
      15,    16,    50,   243,   244,   138,    50,    11,   142,   141,
     154,   152,   128,   153,   155,   156,   170,   172,   173,   175,
     234,   182,   174,   181,   130,   186,   188,   237,   233,    25,
     190,   187,   191,   199,   245,    50,   102,   189,   207,    50,
     215,   205,   209,   214,   216,   217,   220,   222,   218,   228,
     128,   242,   221,   240,   227,   229,   148,    -2,    75,   171,
       2,     3,    50,     4,     5,     6,     7,     8,   232,     9,
      10,   158,    77,   204,   236,     0,     0,   211,   157,     0,
       0,    11,     0,     0,    12,     0,     0,    13,    14,    15,
      16,    17,    18,    19,    20,     0,    21,    22,    23,     0,
       0,    24,    25,    26,    27,    28,    29,    30,     0,     0,
       0,     0,    31,    32,    33,    34,    75,     0,     2,     3,
       0,     4,     5,     6,     7,     8,     0,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    11,
      76,     0,    12,     0,     0,    13,    14,    15,    16,    17,
      18,    19,    20,     0,    21,    22,    23,     0,     0,    24,
      25,    26,    27,    28,    29,    30,     0,     0,     0,     0,
      31,    32,    33,    34,    75,     0,     2,     3,     0,     4,
       5,     6,     7,     8,     0,     9,    10,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    11,   136,     0,
      12,     0,     0,    13,    14,    15,    16,    17,    18,    19,
      20,     0,    21,    22,    23,     0,     0,    24,    25,    26,
      27,    28,    29,    30,     0,     0,     0,     0,    31,    32,
      33,    34,     1,     0,     2,     3,     0,     4,     5,     6,
       7,     8,     0,     9,    10,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    11,     0,     0,    12,     0,
       0,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,     0,     0,    24,    25,    26,    27,    28,
      29,    30,     0,     0,     0,     0,    31,    32,    33,    34,
      78,     3,     0,     4,     5,     6,     7,     8,     0,     9,
      10,     0,     0,    78,     3,     0,     4,     5,     6,     7,
       8,     0,     9,    10,     0,     0,    78,     3,     0,     4,
       5,     6,     7,     8,     0,     9,    10,     0,     0,     0,
     132,    24,     0,    26,     0,     0,     0,    30,     0,     0,
       0,     0,    31,   134,    24,    34,    26,     0,     0,     0,
      30,     0,     0,     0,     0,    31,   143,    24,    34,    26,
       0,     0,     0,    30,     0,     0,     0,     0,    31,    78,
       3,    34,     4,     5,     6,     7,     8,     0,     9,    10,
       0,     0,    78,     3,     0,     4,     5,     6,     7,     8,
       0,     9,    10,     0,     0,    78,     3,     0,     4,     5,
       6,     7,     8,     0,     9,    10,     0,   200,     0,   145,
      24,     0,    26,     0,     0,     0,    30,     0,     0,     0,
       0,    31,     0,    24,    34,    26,     0,     0,     0,    30,
       0,     0,     0,     0,    31,   230,    24,    34,    26,     0,
       0,     0,    30,     0,     0,     0,     0,    31,    71,     3,
      34,     4,     5,     6,     7,     8,     0,     9,    10,     0,
       0,    78,     3,     0,     4,     5,     6,     7,     8,     0,
       9,    10,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    24,
       0,    26,     0,     0,     0,    30,     0,     0,     0,     0,
      31,     0,    24,    34,    26,     0,     0,     0,    30,     0,
       0,     0,     0,    31,     0,     0,    34
};

static const yytype_int16 yycheck[] =
{
       0,    19,    20,   153,    37,    29,    17,    18,     9,    10,
       3,    11,   200,    24,    69,    70,    54,    17,    18,     5,
      13,    27,    82,    83,    24,    26,    17,    18,    19,    20,
      44,    24,    24,    34,    23,    28,    28,     3,     4,    77,
       6,     7,     8,     9,    10,     3,    12,    13,   198,    67,
     238,    44,    44,    42,    54,    28,    13,    68,    69,    70,
      44,    27,    26,    96,    26,    51,    52,    24,    68,    69,
      70,    44,    30,    31,    32,    33,     3,    77,    44,    43,
      46,    43,    93,    94,    50,     4,    13,    26,     3,    55,
      26,     3,    58,    93,    94,   119,     3,     4,    13,     6,
       7,     8,     9,    10,    43,    12,    13,    43,    26,    44,
      24,    29,    44,    28,    15,    16,   117,   118,   142,    27,
     138,   181,   122,    37,   111,   112,   113,   114,   152,    11,
      12,   109,   110,   151,    13,    14,    43,    44,    27,    46,
       3,     3,     3,    50,   155,   156,   115,   116,    55,   182,
     205,    58,     0,    27,   214,   155,   156,    22,    54,     3,
     193,   172,    24,    21,    42,   176,   199,    27,    30,    31,
      32,    33,   172,   233,   234,    27,   176,    24,    44,    43,
       8,    24,   200,    24,    23,    23,     3,    59,    25,    29,
     214,   215,    26,    38,   205,    27,    27,   221,   209,    45,
      27,    43,    27,    29,   237,   205,    13,    43,     3,   209,
      26,    44,    27,    43,    27,    27,    25,    25,    53,    13,
     238,   232,    26,     8,    27,    27,    96,     0,     1,   122,
       3,     4,   232,     6,     7,     8,     9,    10,    59,    12,
      13,   108,    11,   156,   215,    -1,    -1,   181,   107,    -1,
      -1,    24,    -1,    -1,    27,    -1,    -1,    30,    31,    32,
      33,    34,    35,    36,    37,    -1,    39,    40,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    -1,    -1,
      -1,    -1,    55,    56,    57,    58,     1,    -1,     3,     4,
      -1,     6,     7,     8,     9,    10,    -1,    12,    13,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      25,    -1,    27,    -1,    -1,    30,    31,    32,    33,    34,
      35,    36,    37,    -1,    39,    40,    41,    -1,    -1,    44,
      45,    46,    47,    48,    49,    50,    -1,    -1,    -1,    -1,
      55,    56,    57,    58,     1,    -1,     3,     4,    -1,     6,
       7,     8,     9,    10,    -1,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    -1,
      27,    -1,    -1,    30,    31,    32,    33,    34,    35,    36,
      37,    -1,    39,    40,    41,    -1,    -1,    44,    45,    46,
      47,    48,    49,    50,    -1,    -1,    -1,    -1,    55,    56,
      57,    58,     1,    -1,     3,     4,    -1,     6,     7,     8,
       9,    10,    -1,    12,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    -1,    -1,    27,    -1,
      -1,    30,    31,    32,    33,    34,    35,    36,    37,    -1,
      39,    40,    41,    -1,    -1,    44,    45,    46,    47,    48,
      49,    50,    -1,    -1,    -1,    -1,    55,    56,    57,    58,
       3,     4,    -1,     6,     7,     8,     9,    10,    -1,    12,
      13,    -1,    -1,     3,     4,    -1,     6,     7,     8,     9,
      10,    -1,    12,    13,    -1,    -1,     3,     4,    -1,     6,
       7,     8,     9,    10,    -1,    12,    13,    -1,    -1,    -1,
      43,    44,    -1,    46,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    55,    43,    44,    58,    46,    -1,    -1,    -1,
      50,    -1,    -1,    -1,    -1,    55,    43,    44,    58,    46,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,     3,
       4,    58,     6,     7,     8,     9,    10,    -1,    12,    13,
      -1,    -1,     3,     4,    -1,     6,     7,     8,     9,    10,
      -1,    12,    13,    -1,    -1,     3,     4,    -1,     6,     7,
       8,     9,    10,    -1,    12,    13,    -1,    28,    -1,    43,
      44,    -1,    46,    -1,    -1,    -1,    50,    -1,    -1,    -1,
      -1,    55,    -1,    44,    58,    46,    -1,    -1,    -1,    50,
      -1,    -1,    -1,    -1,    55,    43,    44,    58,    46,    -1,
      -1,    -1,    50,    -1,    -1,    -1,    -1,    55,     3,     4,
      58,     6,     7,     8,     9,    10,    -1,    12,    13,    -1,
      -1,     3,     4,    -1,     6,     7,     8,     9,    10,    -1,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,
      -1,    46,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,
      55,    -1,    44,    58,    46,    -1,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    58
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,     3,     4,     6,     7,     8,     9,    10,    12,
      13,    24,    27,    30,    31,    32,    33,    34,    35,    36,
      37,    39,    40,    41,    44,    45,    46,    47,    48,    49,
      50,    55,    56,    57,    58,    62,    63,    64,    65,    66,
      70,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    88,    93,    94,    95,    96,    98,    99,   100,
     102,   103,   104,   107,   110,    27,    24,    28,    44,    44,
      44,     3,    84,    85,    84,     1,    25,    94,     3,    27,
      76,    76,    77,    77,     4,    97,    27,    27,    76,     3,
      68,    69,    84,    44,    44,     3,    64,    67,     3,     3,
      84,     0,    13,    28,    69,    71,    27,    22,    21,    15,
      16,    17,    18,    19,    20,    11,    12,    13,    14,    54,
      23,    42,    87,    95,     3,   111,   112,    73,    77,    43,
      76,    92,    43,    92,    43,    92,    25,    27,    27,    99,
      99,    43,    44,    43,    76,    43,    76,    69,    71,     5,
      51,    52,    24,    24,     8,    23,    23,    79,    80,    81,
      81,    82,    82,    82,    82,    83,    83,    84,    84,    64,
       3,    86,    59,    25,    26,    29,    26,    43,    43,    43,
      77,    38,    64,    89,    90,    91,    27,    43,    27,    43,
      27,    27,    77,    64,   105,   106,    88,   108,   109,    29,
      28,    72,    74,    76,    72,    44,    76,     3,    76,    27,
      99,   100,   101,    69,    43,    26,    27,    27,    53,    69,
      25,    26,    25,    88,    69,    73,    75,    27,    13,    27,
      43,    92,    59,    76,    64,    99,    91,    64,    26,    29,
       8,    43,    76,    99,    99,    69,    73
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    61,    62,    62,    63,    63,    63,    63,    64,    64,
      64,    65,    65,    65,    65,    65,    65,    66,    67,    67,
      67,    68,    69,    70,    70,    70,    70,    71,    72,    72,
      72,    73,    74,    75,    75,    76,    77,    78,    78,    79,
      79,    80,    80,    80,    81,    81,    81,    81,    81,    82,
      82,    82,    83,    83,    83,    84,    84,    84,    84,    84,
      84,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    86,    86,    86,    87,    88,    88,    89,    89,    90,
      90,    91,    92,    92,    93,    93,    93,    93,    94,    94,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    96,    97,    98,    98,    99,    99,   100,   100,   101,
     101,   102,   102,   103,   103,   103,   103,   103,   103,   103,
     103,   104,   105,   105,   106,   106,   106,   107,   108,   108,
     109,   109,   110,   111,   111,   112,   112
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     3,     0,     2,
       4,     1,     1,     5,     5,     4,     4,     4,     1,     1,
       3,     1,     3,     1,     3,     1,     1,     1,     3,     1,
       3,     1,     3,     3,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     1,     2,     2,     2,     2,
       3,     4,     3,     1,     1,     4,     1,     3,     3,     6,
       5,     1,     1,     3,     1,     7,     6,     1,     0,     1,
       3,     2,     3,     1,     4,     3,     4,     3,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     1,     2,     1,     3,     2,     3,     5,     1,
       1,     3,     7,     2,     2,     3,     2,     5,     4,     5,
       4,     5,     0,     1,     2,     4,     2,     5,     0,     1,
       1,     2,     4,     0,     1,     3,     5
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (Program, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Program); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, kriol::ast::BlockSttmt** Program)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (Program);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, kriol::ast::BlockSttmt** Program)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, Program);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, kriol::ast::BlockSttmt** Program)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], Program);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, Program); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, kriol::ast::BlockSttmt** Program)
{
  YY_USE (yyvaluep);
  YY_USE (Program);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_IDENT: /* "valid identifier"  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1471 "parser.cc"
        break;

    case YYSYMBOL_STR_LIT: /* "string literal"  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1477 "parser.cc"
        break;

    case YYSYMBOL_FSTR_TEXT: /* "f-string text"  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1483 "parser.cc"
        break;

    case YYSYMBOL_INT_LIT: /* "integer literal"  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).integer); }
#line 1489 "parser.cc"
        break;

    case YYSYMBOL_FLOAT_LIT: /* "floating-point literal "  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).floatingpoint); }
#line 1495 "parser.cc"
        break;

    case YYSYMBOL_BOOL_LIT: /* "boolean literal"  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1501 "parser.cc"
        break;

    case YYSYMBOL_TYPE_NUM: /* TYPE_NUM  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1507 "parser.cc"
        break;

    case YYSYMBOL_TYPE_BOOL: /* TYPE_BOOL  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1513 "parser.cc"
        break;

    case YYSYMBOL_TYPE_NTER: /* TYPE_NTER  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1519 "parser.cc"
        break;

    case YYSYMBOL_TYPE_TEXTU: /* TYPE_TEXTU  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1525 "parser.cc"
        break;

    case YYSYMBOL_type_specifier: /* type_specifier  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1531 "parser.cc"
        break;

    case YYSYMBOL_type_name: /* type_name  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1537 "parser.cc"
        break;

    case YYSYMBOL_constant: /* constant  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1543 "parser.cc"
        break;

    case YYSYMBOL_fstring: /* fstring  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1549 "parser.cc"
        break;

    case YYSYMBOL_fstring_parts: /* fstring_parts  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1555 "parser.cc"
        break;

    case YYSYMBOL_identifier: /* identifier  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1561 "parser.cc"
        break;

    case YYSYMBOL_declarator: /* declarator  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1567 "parser.cc"
        break;

    case YYSYMBOL_declaration: /* declaration  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1573 "parser.cc"
        break;

    case YYSYMBOL_array_declarator: /* array_declarator  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).vardecl); }
#line 1579 "parser.cc"
        break;

    case YYSYMBOL_initializer: /* initializer  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1585 "parser.cc"
        break;

    case YYSYMBOL_value_expression: /* value_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1591 "parser.cc"
        break;

    case YYSYMBOL_array_initializer: /* array_initializer  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1597 "parser.cc"
        break;

    case YYSYMBOL_array_initializer_elements: /* array_initializer_elements  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1603 "parser.cc"
        break;

    case YYSYMBOL_expression: /* expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1609 "parser.cc"
        break;

    case YYSYMBOL_constant_expression: /* constant_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1615 "parser.cc"
        break;

    case YYSYMBOL_logical_or_expressions: /* logical_or_expressions  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1621 "parser.cc"
        break;

    case YYSYMBOL_logical_and_expressions: /* logical_and_expressions  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1627 "parser.cc"
        break;

    case YYSYMBOL_equality_expression: /* equality_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1633 "parser.cc"
        break;

    case YYSYMBOL_relational_expression: /* relational_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1639 "parser.cc"
        break;

    case YYSYMBOL_additive_expression: /* additive_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1645 "parser.cc"
        break;

    case YYSYMBOL_multiplicative_expression: /* multiplicative_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1651 "parser.cc"
        break;

    case YYSYMBOL_unary_expression: /* unary_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1657 "parser.cc"
        break;

    case YYSYMBOL_primary_expression: /* primary_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1663 "parser.cc"
        break;

    case YYSYMBOL_assignment_expression: /* assignment_expression  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1669 "parser.cc"
        break;

    case YYSYMBOL_assignment_operator: /* assignment_operator  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1675 "parser.cc"
        break;

    case YYSYMBOL_function_declaration: /* function_declaration  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1681 "parser.cc"
        break;

    case YYSYMBOL_parameter_optional_list: /* parameter_optional_list  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).params); }
#line 1687 "parser.cc"
        break;

    case YYSYMBOL_parameter_list: /* parameter_list  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).params); }
#line 1693 "parser.cc"
        break;

    case YYSYMBOL_parameter_declaration: /* parameter_declaration  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).vardecl); }
#line 1699 "parser.cc"
        break;

    case YYSYMBOL_argument_list: /* argument_list  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).args); }
#line 1705 "parser.cc"
        break;

    case YYSYMBOL_mostra_func_call: /* mostra_func_call  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1711 "parser.cc"
        break;

    case YYSYMBOL_statements: /* statements  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).block); }
#line 1717 "parser.cc"
        break;

    case YYSYMBOL_statement: /* statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1723 "parser.cc"
        break;

    case YYSYMBOL_import_statement: /* import_statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1729 "parser.cc"
        break;

    case YYSYMBOL_single_import: /* single_import  */
#line 36 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1735 "parser.cc"
        break;

    case YYSYMBOL_expression_statement: /* expression_statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1741 "parser.cc"
        break;

    case YYSYMBOL_compound_statement: /* compound_statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).block); }
#line 1747 "parser.cc"
        break;

    case YYSYMBOL_selection_statement: /* selection_statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1753 "parser.cc"
        break;

    case YYSYMBOL_else_then: /* else_then  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).block); }
#line 1759 "parser.cc"
        break;

    case YYSYMBOL_iteration_statement: /* iteration_statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1765 "parser.cc"
        break;

    case YYSYMBOL_jump_statement: /* jump_statement  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1771 "parser.cc"
        break;

    case YYSYMBOL_struct_declaration: /* struct_declaration  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1777 "parser.cc"
        break;

    case YYSYMBOL_struct_fields: /* struct_fields  */
#line 38 "rules/parser.y"
            { delete ((*yyvaluep).struct_fields); }
#line 1783 "parser.cc"
        break;

    case YYSYMBOL_struct_field_list: /* struct_field_list  */
#line 38 "rules/parser.y"
            { delete ((*yyvaluep).struct_fields); }
#line 1789 "parser.cc"
        break;

    case YYSYMBOL_impl_declaration: /* impl_declaration  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1795 "parser.cc"
        break;

    case YYSYMBOL_struct_methods: /* struct_methods  */
#line 38 "rules/parser.y"
            { delete ((*yyvaluep).struct_methods); }
#line 1801 "parser.cc"
        break;

    case YYSYMBOL_struct_method_list: /* struct_method_list  */
#line 38 "rules/parser.y"
            { delete ((*yyvaluep).struct_methods); }
#line 1807 "parser.cc"
        break;

    case YYSYMBOL_struct_literal: /* struct_literal  */
#line 37 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1813 "parser.cc"
        break;

    case YYSYMBOL_struct_inits: /* struct_inits  */
#line 38 "rules/parser.y"
            { delete ((*yyvaluep).struct_inits); }
#line 1819 "parser.cc"
        break;

    case YYSYMBOL_struct_init_list: /* struct_init_list  */
#line 38 "rules/parser.y"
            { delete ((*yyvaluep).struct_inits); }
#line 1825 "parser.cc"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (kriol::ast::BlockSttmt** Program)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: statements  */
#line 84 "rules/parser.y"
                     { *Program = (yyvsp[0].block); }
#line 2098 "parser.cc"
    break;

  case 3: /* program: error  */
#line 85 "rules/parser.y"
                { *Program = nullptr; }
#line 2104 "parser.cc"
    break;

  case 4: /* type_specifier: TYPE_NUM  */
#line 88 "rules/parser.y"
                          { (yyval.string) = (yyvsp[0].string); }
#line 2110 "parser.cc"
    break;

  case 5: /* type_specifier: TYPE_BOOL  */
#line 89 "rules/parser.y"
                           { (yyval.string) = (yyvsp[0].string); }
#line 2116 "parser.cc"
    break;

  case 6: /* type_specifier: TYPE_NTER  */
#line 90 "rules/parser.y"
                           { (yyval.string) = (yyvsp[0].string); }
#line 2122 "parser.cc"
    break;

  case 7: /* type_specifier: TYPE_TEXTU  */
#line 91 "rules/parser.y"
                            { (yyval.string) = (yyvsp[0].string); }
#line 2128 "parser.cc"
    break;

  case 8: /* type_name: type_specifier  */
#line 94 "rules/parser.y"
                           { (yyval.string) = (yyvsp[0].string); }
#line 2134 "parser.cc"
    break;

  case 9: /* type_name: "valid identifier"  */
#line 95 "rules/parser.y"
                  { (yyval.string) = (yyvsp[0].string); }
#line 2140 "parser.cc"
    break;

  case 10: /* type_name: type_name "*"  */
#line 96 "rules/parser.y"
                          { (yyval.string) = new std::string(*(yyvsp[-1].string) + "*"); delete (yyvsp[-1].string); }
#line 2146 "parser.cc"
    break;

  case 11: /* constant: "integer literal"  */
#line 99 "rules/parser.y"
                     { auto lit = new ast::LiteralExpr("nter",  *(yyvsp[0].integer)); lit->LineNum = yylineno; (yyval.expr) = lit; delete (yyvsp[0].integer); }
#line 2152 "parser.cc"
    break;

  case 12: /* constant: "floating-point literal "  */
#line 100 "rules/parser.y"
                      { auto lit = new ast::LiteralExpr("num",   *(yyvsp[0].floatingpoint)); lit->LineNum = yylineno; (yyval.expr) = lit; delete (yyvsp[0].floatingpoint); }
#line 2158 "parser.cc"
    break;

  case 13: /* constant: "boolean literal"  */
#line 101 "rules/parser.y"
                      { auto lit = new ast::LiteralExpr("bool",  *(yyvsp[0].string)); lit->LineNum = yylineno; (yyval.expr) = lit; delete (yyvsp[0].string); }
#line 2164 "parser.cc"
    break;

  case 14: /* constant: "string literal"  */
#line 102 "rules/parser.y"
                      { auto lit = new ast::LiteralExpr("char*", *(yyvsp[0].string)); lit->LineNum = yylineno; (yyval.expr) = lit; delete (yyvsp[0].string); }
#line 2170 "parser.cc"
    break;

  case 15: /* constant: "nada"  */
#line 103 "rules/parser.y"
                      { auto lit = new ast::LiteralExpr("nada", "nada"); lit->LineNum = yylineno; (yyval.expr) = lit; }
#line 2176 "parser.cc"
    break;

  case 16: /* constant: fstring  */
#line 104 "rules/parser.y"
                      { (yyval.expr) = (yyvsp[0].expr); }
#line 2182 "parser.cc"
    break;

  case 17: /* fstring: "f-string" fstring_parts "end of f-string"  */
#line 107 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[-1].expr); }
#line 2188 "parser.cc"
    break;

  case 18: /* fstring_parts: %empty  */
#line 110 "rules/parser.y"
                       { auto fs = new ast::FStringExpr(); fs->LineNum = yylineno; (yyval.expr) = fs; }
#line 2194 "parser.cc"
    break;

  case 19: /* fstring_parts: fstring_parts "f-string text"  */
#line 111 "rules/parser.y"
                                        { static_cast<ast::FStringExpr*>((yyvsp[-1].expr))->addText(*(yyvsp[0].string)); delete (yyvsp[0].string); (yyval.expr) = (yyvsp[-1].expr); }
#line 2200 "parser.cc"
    break;

  case 20: /* fstring_parts: fstring_parts "start of interpolation" constant_expression "end of interpolation"  */
#line 112 "rules/parser.y"
                                                                          { static_cast<ast::FStringExpr*>((yyvsp[-3].expr))->addExpr(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); (yyval.expr) = (yyvsp[-3].expr); }
#line 2206 "parser.cc"
    break;

  case 21: /* identifier: "valid identifier"  */
#line 115 "rules/parser.y"
                   { (yyval.string) = (yyvsp[0].string); }
#line 2212 "parser.cc"
    break;

  case 22: /* declarator: identifier  */
#line 118 "rules/parser.y"
                        { (yyval.string) = (yyvsp[0].string); }
#line 2218 "parser.cc"
    break;

  case 23: /* declaration: type_name declarator "=" initializer ";"  */
#line 121 "rules/parser.y"
                                                            { auto d = new ast::VarDeclSttmt(*(yyvsp[-4].string), *(yyvsp[-3].string), std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); d->LineNum = yylineno; (yyval.sttmt) = d; delete (yyvsp[-4].string); delete (yyvsp[-3].string); }
#line 2224 "parser.cc"
    break;

  case 24: /* declaration: type_name array_declarator "=" initializer ";"  */
#line 122 "rules/parser.y"
                                                                  { (yyvsp[-3].vardecl)->SetType(*(yyvsp[-4].string) + "[" + std::to_string((yyvsp[-3].vardecl)->ArraySize) + "]"); (yyvsp[-3].vardecl)->Value = std::unique_ptr<ast::Expr>((yyvsp[-1].expr)); (yyvsp[-3].vardecl)->LineNum = yylineno; (yyval.sttmt) = (yyvsp[-3].vardecl); delete (yyvsp[-4].string); }
#line 2230 "parser.cc"
    break;

  case 25: /* declaration: "dipoz" type_name declarator ";"  */
#line 123 "rules/parser.y"
                                               { auto d = new ast::VarDeclSttmt(*(yyvsp[-2].string), *(yyvsp[-1].string), nullptr); d->LineNum = yylineno; (yyval.sttmt) = d; delete (yyvsp[-2].string); delete (yyvsp[-1].string); }
#line 2236 "parser.cc"
    break;

  case 26: /* declaration: "dipoz" type_name array_declarator ";"  */
#line 124 "rules/parser.y"
                                                     { (yyvsp[-1].vardecl)->SetType(*(yyvsp[-2].string) + "[" + std::to_string((yyvsp[-1].vardecl)->ArraySize) + "]"); (yyvsp[-1].vardecl)->LineNum = yylineno; (yyval.sttmt) = (yyvsp[-1].vardecl); delete (yyvsp[-2].string); }
#line 2242 "parser.cc"
    break;

  case 27: /* array_declarator: "[" "integer literal" "]" declarator  */
#line 127 "rules/parser.y"
                                                  { (yyval.vardecl) = new ast::VarDeclSttmt("void", *(yyvsp[0].string), nullptr); (yyval.vardecl)->IsArray = true; (yyval.vardecl)->ArraySize = std::stoul(*(yyvsp[-2].integer)); delete (yyvsp[-2].integer); delete (yyvsp[0].string); }
#line 2248 "parser.cc"
    break;

  case 28: /* initializer: expression  */
#line 130 "rules/parser.y"
                         { (yyval.expr) = (yyvsp[0].expr); }
#line 2254 "parser.cc"
    break;

  case 29: /* initializer: array_initializer  */
#line 131 "rules/parser.y"
                                { (yyval.expr) = (yyvsp[0].expr); }
#line 2260 "parser.cc"
    break;

  case 30: /* initializer: array_initializer "*" "integer literal"  */
#line 132 "rules/parser.y"
                                            { auto* lit = static_cast<ast::ArrayLiteralExpr*>((yyvsp[-2].expr)); auto fill = std::move(lit->Elements[0]); delete lit; auto n = new ast::ArrayRepeatExpr(std::move(fill), std::stoul(*(yyvsp[0].integer))); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[0].integer); }
#line 2266 "parser.cc"
    break;

  case 31: /* value_expression: constant_expression  */
#line 135 "rules/parser.y"
                                       { (yyval.expr) = (yyvsp[0].expr); }
#line 2272 "parser.cc"
    break;

  case 32: /* array_initializer: "[" array_initializer_elements "]"  */
#line 138 "rules/parser.y"
                                                           { (yyval.expr) = (yyvsp[-1].expr); }
#line 2278 "parser.cc"
    break;

  case 33: /* array_initializer_elements: value_expression  */
#line 141 "rules/parser.y"
                                              { auto n = new ast::ArrayLiteralExpr(); n->LineNum = yylineno; n->addElement(std::unique_ptr<ast::Expr>((yyvsp[0].expr))); (yyval.expr) = n; }
#line 2284 "parser.cc"
    break;

  case 34: /* array_initializer_elements: array_initializer_elements "," value_expression  */
#line 142 "rules/parser.y"
                                                                               { static_cast<ast::ArrayLiteralExpr*>((yyvsp[-2].expr))->addElement(std::unique_ptr<ast::Expr>((yyvsp[0].expr))); (yyval.expr) = (yyvsp[-2].expr); }
#line 2290 "parser.cc"
    break;

  case 35: /* expression: assignment_expression  */
#line 145 "rules/parser.y"
                                   { (yyval.expr) = (yyvsp[0].expr); }
#line 2296 "parser.cc"
    break;

  case 36: /* constant_expression: logical_or_expressions  */
#line 148 "rules/parser.y"
                                             { (yyval.expr) = (yyvsp[0].expr); }
#line 2302 "parser.cc"
    break;

  case 37: /* logical_or_expressions: logical_and_expressions  */
#line 151 "rules/parser.y"
                                                 { (yyval.expr) = (yyvsp[0].expr); }
#line 2308 "parser.cc"
    break;

  case 38: /* logical_or_expressions: logical_or_expressions "||" logical_and_expressions  */
#line 152 "rules/parser.y"
                                                                           { auto n = new ast::BinExpr("||", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2314 "parser.cc"
    break;

  case 39: /* logical_and_expressions: equality_expression  */
#line 155 "rules/parser.y"
                                              { (yyval.expr) = (yyvsp[0].expr); }
#line 2320 "parser.cc"
    break;

  case 40: /* logical_and_expressions: logical_and_expressions "&&" equality_expression  */
#line 156 "rules/parser.y"
                                                                          { auto n = new ast::BinExpr("&&", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2326 "parser.cc"
    break;

  case 41: /* equality_expression: relational_expression  */
#line 159 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[0].expr); }
#line 2332 "parser.cc"
    break;

  case 42: /* equality_expression: equality_expression "==" relational_expression  */
#line 160 "rules/parser.y"
                                                                   { auto n = new ast::BinExpr("==", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2338 "parser.cc"
    break;

  case 43: /* equality_expression: equality_expression "!=" relational_expression  */
#line 161 "rules/parser.y"
                                                                   { auto n = new ast::BinExpr("!=", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2344 "parser.cc"
    break;

  case 44: /* relational_expression: additive_expression  */
#line 164 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[0].expr); }
#line 2350 "parser.cc"
    break;

  case 45: /* relational_expression: relational_expression "<" additive_expression  */
#line 165 "rules/parser.y"
                                                                     { auto n = new ast::BinExpr("<", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2356 "parser.cc"
    break;

  case 46: /* relational_expression: relational_expression ">" additive_expression  */
#line 166 "rules/parser.y"
                                                                     { auto n = new ast::BinExpr(">", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2362 "parser.cc"
    break;

  case 47: /* relational_expression: relational_expression "<=" additive_expression  */
#line 167 "rules/parser.y"
                                                                     { auto n = new ast::BinExpr("<=", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2368 "parser.cc"
    break;

  case 48: /* relational_expression: relational_expression ">=" additive_expression  */
#line 168 "rules/parser.y"
                                                                     { auto n = new ast::BinExpr(">=", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2374 "parser.cc"
    break;

  case 49: /* additive_expression: multiplicative_expression  */
#line 171 "rules/parser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 2380 "parser.cc"
    break;

  case 50: /* additive_expression: additive_expression "+" multiplicative_expression  */
#line 172 "rules/parser.y"
                                                                         { auto n = new ast::BinExpr("+", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2386 "parser.cc"
    break;

  case 51: /* additive_expression: additive_expression "-" multiplicative_expression  */
#line 173 "rules/parser.y"
                                                                          { auto n = new ast::BinExpr("-", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2392 "parser.cc"
    break;

  case 52: /* multiplicative_expression: unary_expression  */
#line 176 "rules/parser.y"
                                             { (yyval.expr) = (yyvsp[0].expr); }
#line 2398 "parser.cc"
    break;

  case 53: /* multiplicative_expression: multiplicative_expression "*" unary_expression  */
#line 177 "rules/parser.y"
                                                                           { auto n = new ast::BinExpr("*", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2404 "parser.cc"
    break;

  case 54: /* multiplicative_expression: multiplicative_expression "/" unary_expression  */
#line 178 "rules/parser.y"
                                                                           { auto n = new ast::BinExpr("/", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2410 "parser.cc"
    break;

  case 55: /* unary_expression: primary_expression  */
#line 181 "rules/parser.y"
                                                               { (yyval.expr) = (yyvsp[0].expr); }
#line 2416 "parser.cc"
    break;

  case 56: /* unary_expression: "!" unary_expression  */
#line 182 "rules/parser.y"
                                                                { auto n = new ast::UnaryExpr("!", std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2422 "parser.cc"
    break;

  case 57: /* unary_expression: "-" unary_expression  */
#line 183 "rules/parser.y"
                                                                { auto n = new ast::UnaryExpr("-", std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2428 "parser.cc"
    break;

  case 58: /* unary_expression: "&" unary_expression  */
#line 184 "rules/parser.y"
                                                                { auto n = new ast::UnaryExpr("&", std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2434 "parser.cc"
    break;

  case 59: /* unary_expression: "*" unary_expression  */
#line 185 "rules/parser.y"
                                                                { auto n = new ast::UnaryExpr("*", std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2440 "parser.cc"
    break;

  case 60: /* unary_expression: unary_expression "komu" type_name  */
#line 186 "rules/parser.y"
                                                                { auto n = new ast::CastExpr(std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), *(yyvsp[0].string)); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[0].string); }
#line 2446 "parser.cc"
    break;

  case 61: /* primary_expression: "valid identifier" "(" argument_list ")"  */
#line 189 "rules/parser.y"
                                                   { auto n = new ast::FunCallExpr(*(yyvsp[-3].string), std::unique_ptr<ast::FuncCallArgs>((yyvsp[-1].args))); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[-3].string); }
#line 2452 "parser.cc"
    break;

  case 62: /* primary_expression: "valid identifier" "(" ")"  */
#line 190 "rules/parser.y"
                                                   { auto n = new ast::FunCallExpr(*(yyvsp[-2].string), nullptr); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[-2].string); }
#line 2458 "parser.cc"
    break;

  case 63: /* primary_expression: mostra_func_call  */
#line 191 "rules/parser.y"
                                                    { (yyval.expr) = (yyvsp[0].expr); }
#line 2464 "parser.cc"
    break;

  case 64: /* primary_expression: "valid identifier"  */
#line 192 "rules/parser.y"
                                                    { auto n = new ast::IdentExpr(*(yyvsp[0].string)); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[0].string); }
#line 2470 "parser.cc"
    break;

  case 65: /* primary_expression: "valid identifier" "[" value_expression "]"  */
#line 193 "rules/parser.y"
                                                          { auto n = new ast::ArrayAccessExpr(*(yyvsp[-3].string), std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[-3].string); }
#line 2476 "parser.cc"
    break;

  case 66: /* primary_expression: constant  */
#line 194 "rules/parser.y"
                                                    { (yyval.expr) = (yyvsp[0].expr); }
#line 2482 "parser.cc"
    break;

  case 67: /* primary_expression: "(" expression ")"  */
#line 195 "rules/parser.y"
                                                    { auto n = new ast::ParExpr(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2488 "parser.cc"
    break;

  case 68: /* primary_expression: primary_expression "." "valid identifier"  */
#line 196 "rules/parser.y"
                                                    { auto n = new ast::FieldAccessExpr(std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), *(yyvsp[0].string)); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[0].string); }
#line 2494 "parser.cc"
    break;

  case 69: /* primary_expression: primary_expression "." "valid identifier" "(" argument_list ")"  */
#line 197 "rules/parser.y"
                                                                          { auto n = new ast::MethodCallExpr(std::unique_ptr<ast::Expr>((yyvsp[-5].expr)), *(yyvsp[-3].string), std::unique_ptr<ast::FuncCallArgs>((yyvsp[-1].args))); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[-3].string); }
#line 2500 "parser.cc"
    break;

  case 70: /* primary_expression: primary_expression "." "valid identifier" "(" ")"  */
#line 198 "rules/parser.y"
                                                                          { auto n = new ast::MethodCallExpr(std::unique_ptr<ast::Expr>((yyvsp[-4].expr)), *(yyvsp[-2].string), nullptr); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[-2].string); }
#line 2506 "parser.cc"
    break;

  case 71: /* assignment_expression: constant_expression  */
#line 201 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[0].expr); }
#line 2512 "parser.cc"
    break;

  case 72: /* assignment_expression: struct_literal  */
#line 202 "rules/parser.y"
                                       { (yyval.expr) = (yyvsp[0].expr); }
#line 2518 "parser.cc"
    break;

  case 73: /* assignment_expression: primary_expression assignment_operator assignment_expression  */
#line 203 "rules/parser.y"
                                                                                     { auto n = new ast::AssignExpr(*(yyvsp[-1].string), std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); n->LineNum = yylineno; (yyval.expr) = n; delete (yyvsp[-1].string); }
#line 2524 "parser.cc"
    break;

  case 74: /* assignment_operator: "="  */
#line 206 "rules/parser.y"
                             { (yyval.string) = new std::string("=", 2); }
#line 2530 "parser.cc"
    break;

  case 75: /* function_declaration: "fn" declarator "(" parameter_optional_list ")" type_name compound_statement  */
#line 209 "rules/parser.y"
                                                                                                    { auto n = new ast::FuncDeclSttmt(*(yyvsp[-1].string), *(yyvsp[-5].string), std::unique_ptr<ast::FuncArgs>((yyvsp[-3].params)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); n->LineNum = yylineno; (yyval.sttmt) = n; delete (yyvsp[-1].string); delete (yyvsp[-5].string); }
#line 2536 "parser.cc"
    break;

  case 76: /* function_declaration: "fn" declarator "(" parameter_optional_list ")" compound_statement  */
#line 210 "rules/parser.y"
                                                                                          { auto n = new ast::FuncDeclSttmt("vaziu", *(yyvsp[-4].string), std::unique_ptr<ast::FuncArgs>((yyvsp[-2].params)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); n->LineNum = yylineno; (yyval.sttmt) = n; delete (yyvsp[-4].string); }
#line 2542 "parser.cc"
    break;

  case 77: /* parameter_optional_list: parameter_list  */
#line 213 "rules/parser.y"
                                         { (yyval.params) = (yyvsp[0].params); }
#line 2548 "parser.cc"
    break;

  case 78: /* parameter_optional_list: %empty  */
#line 214 "rules/parser.y"
                                 { (yyval.params) = new ast::FuncArgs(); }
#line 2554 "parser.cc"
    break;

  case 79: /* parameter_list: parameter_declaration  */
#line 217 "rules/parser.y"
                                       { (yyval.params) = new ast::FuncArgs(); (yyval.params)->AddArg(std::unique_ptr<ast::VarDeclSttmt>((yyvsp[0].vardecl))); }
#line 2560 "parser.cc"
    break;

  case 80: /* parameter_list: parameter_list "," parameter_declaration  */
#line 218 "rules/parser.y"
                                                            { (yyvsp[-2].params)->AddArg(std::unique_ptr<ast::VarDeclSttmt>((yyvsp[0].vardecl))); (yyval.params) = (yyvsp[-2].params); }
#line 2566 "parser.cc"
    break;

  case 81: /* parameter_declaration: type_name declarator  */
#line 221 "rules/parser.y"
                                             { (yyval.vardecl) = new ast::VarDeclSttmt(*(yyvsp[-1].string), *(yyvsp[0].string), nullptr); (yyval.vardecl)->IsParam = true; (yyval.vardecl)->LineNum = yylineno; delete (yyvsp[-1].string); delete (yyvsp[0].string); }
#line 2572 "parser.cc"
    break;

  case 82: /* argument_list: argument_list "," expression  */
#line 224 "rules/parser.y"
                                               { (yyvsp[-2].args)->AddArg(std::unique_ptr<ast::Expr>((yyvsp[0].expr))); (yyval.args) = (yyvsp[-2].args); }
#line 2578 "parser.cc"
    break;

  case 83: /* argument_list: expression  */
#line 225 "rules/parser.y"
                           { (yyval.args) = new ast::FuncCallArgs(); (yyval.args)->AddArg(std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 2584 "parser.cc"
    break;

  case 84: /* mostra_func_call: "mostra" "(" argument_list ")"  */
#line 228 "rules/parser.y"
                                                  { auto n = new ast::MostraFunCallExpr(std::unique_ptr<ast::FuncCallArgs>((yyvsp[-1].args))); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2590 "parser.cc"
    break;

  case 85: /* mostra_func_call: "mostra" "(" ")"  */
#line 229 "rules/parser.y"
                                    { auto n = new ast::MostraFunCallExpr(nullptr); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2596 "parser.cc"
    break;

  case 86: /* mostra_func_call: "mostran" "(" argument_list ")"  */
#line 230 "rules/parser.y"
                                                   { auto n = new ast::MostraFunCallExpr(std::unique_ptr<ast::FuncCallArgs>((yyvsp[-1].args)), true); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2602 "parser.cc"
    break;

  case 87: /* mostra_func_call: "mostran" "(" ")"  */
#line 231 "rules/parser.y"
                                     { auto n = new ast::MostraFunCallExpr(nullptr, true); n->LineNum = yylineno; (yyval.expr) = n; }
#line 2608 "parser.cc"
    break;

  case 88: /* statements: statements statement  */
#line 234 "rules/parser.y"
                                  { (yyvsp[-1].block)->AddSttmt(std::unique_ptr<ast::Sttmt>((yyvsp[0].sttmt))); (yyval.block) = (yyvsp[-1].block); }
#line 2614 "parser.cc"
    break;

  case 89: /* statements: statement  */
#line 235 "rules/parser.y"
                       { (yyval.block) = new ast::BlockSttmt(); (yyval.block)->AddSttmt(std::unique_ptr<ast::Sttmt>((yyvsp[0].sttmt))); }
#line 2620 "parser.cc"
    break;

  case 90: /* statement: expression_statement  */
#line 238 "rules/parser.y"
                                 { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2626 "parser.cc"
    break;

  case 91: /* statement: compound_statement  */
#line 239 "rules/parser.y"
                               { (yyval.sttmt) = (yyvsp[0].block); }
#line 2632 "parser.cc"
    break;

  case 92: /* statement: selection_statement  */
#line 240 "rules/parser.y"
                                { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2638 "parser.cc"
    break;

  case 93: /* statement: iteration_statement  */
#line 241 "rules/parser.y"
                                { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2644 "parser.cc"
    break;

  case 94: /* statement: jump_statement  */
#line 242 "rules/parser.y"
                           { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2650 "parser.cc"
    break;

  case 95: /* statement: function_declaration  */
#line 243 "rules/parser.y"
                                 { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2656 "parser.cc"
    break;

  case 96: /* statement: declaration  */
#line 244 "rules/parser.y"
                        { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2662 "parser.cc"
    break;

  case 97: /* statement: import_statement  */
#line 245 "rules/parser.y"
                             { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2668 "parser.cc"
    break;

  case 98: /* statement: struct_declaration  */
#line 246 "rules/parser.y"
                               { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2674 "parser.cc"
    break;

  case 99: /* statement: impl_declaration  */
#line 247 "rules/parser.y"
                             { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2680 "parser.cc"
    break;

  case 100: /* statement: error ";"  */
#line 248 "rules/parser.y"
                        { (yyval.sttmt) = nullptr; }
#line 2686 "parser.cc"
    break;

  case 101: /* import_statement: "inpristan" single_import  */
#line 251 "rules/parser.y"
                                           { (yyval.sttmt) = new ast::ImportSttmt(*(yyvsp[0].string)); delete (yyvsp[0].string); }
#line 2692 "parser.cc"
    break;

  case 102: /* single_import: "string literal"  */
#line 254 "rules/parser.y"
                        { (yyval.string) = (yyvsp[0].string); }
#line 2698 "parser.cc"
    break;

  case 103: /* expression_statement: expression ";"  */
#line 257 "rules/parser.y"
                                        { (yyval.sttmt) = new ast::ExprSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); }
#line 2704 "parser.cc"
    break;

  case 104: /* expression_statement: ";"  */
#line 258 "rules/parser.y"
                             { (yyval.sttmt) = new ast::ExprSttmt(nullptr); }
#line 2710 "parser.cc"
    break;

  case 105: /* compound_statement: "{" statements "}"  */
#line 261 "rules/parser.y"
                                              { (yyval.block) = (yyvsp[-1].block); (yyval.block)->UseBrackets(); }
#line 2716 "parser.cc"
    break;

  case 106: /* compound_statement: "{" "}"  */
#line 262 "rules/parser.y"
                                   { (yyval.block) = new ast::BlockSttmt(); (yyval.block)->UseBrackets(); }
#line 2722 "parser.cc"
    break;

  case 107: /* selection_statement: "si" constant_expression compound_statement  */
#line 265 "rules/parser.y"
                                                                { auto n = new ast::IfSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block)), nullptr); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2728 "parser.cc"
    break;

  case 108: /* selection_statement: "si" constant_expression compound_statement "sinon" else_then  */
#line 266 "rules/parser.y"
                                                                                { auto n = new ast::IfSttmt(std::unique_ptr<ast::Expr>((yyvsp[-3].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[-2].block)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2734 "parser.cc"
    break;

  case 109: /* else_then: compound_statement  */
#line 269 "rules/parser.y"
                               { (yyval.block) = (yyvsp[0].block); }
#line 2740 "parser.cc"
    break;

  case 110: /* else_then: selection_statement  */
#line 270 "rules/parser.y"
                                { (yyval.block) = new ast::BlockSttmt(); (yyval.block)->AddSttmt(std::unique_ptr<ast::Sttmt>((yyvsp[0].sttmt))); }
#line 2746 "parser.cc"
    break;

  case 111: /* iteration_statement: "nkuantu" constant_expression compound_statement  */
#line 273 "rules/parser.y"
                                                                     { auto n = new ast::WhileSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2752 "parser.cc"
    break;

  case 112: /* iteration_statement: "pa" expression ";" constant_expression ";" expression compound_statement  */
#line 274 "rules/parser.y"
                                                                                                  { auto n = new ast::ForSttmt(std::unique_ptr<ast::Expr>((yyvsp[-5].expr)), std::unique_ptr<ast::Expr>((yyvsp[-3].expr)), std::unique_ptr<ast::Expr>((yyvsp[-1].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2758 "parser.cc"
    break;

  case 113: /* jump_statement: "para" ";"  */
#line 277 "rules/parser.y"
                            { auto n = new ast::JumpSttmt("break"); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2764 "parser.cc"
    break;

  case 114: /* jump_statement: "kontinua" ";"  */
#line 278 "rules/parser.y"
                                { auto n = new ast::JumpSttmt("continue"); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2770 "parser.cc"
    break;

  case 115: /* jump_statement: "divolvi" expression ";"  */
#line 279 "rules/parser.y"
                                          { auto n = new ast::ReturnSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2776 "parser.cc"
    break;

  case 116: /* jump_statement: "divolvi" ";"  */
#line 280 "rules/parser.y"
                               { auto n = new ast::ReturnSttmt(nullptr); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2782 "parser.cc"
    break;

  case 117: /* jump_statement: "sai" "(" expression ")" ";"  */
#line 281 "rules/parser.y"
                                                { auto n = new ast::SaiSttmt(std::unique_ptr<ast::Expr>((yyvsp[-2].expr))); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2788 "parser.cc"
    break;

  case 118: /* jump_statement: "sai" "(" ")" ";"  */
#line 282 "rules/parser.y"
                                     { auto n = new ast::SaiSttmt(nullptr); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2794 "parser.cc"
    break;

  case 119: /* jump_statement: "konfirma" "(" expression ")" ";"  */
#line 283 "rules/parser.y"
                                                     { auto n = new ast::KonfirmaSttmt(std::unique_ptr<ast::Expr>((yyvsp[-2].expr))); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2800 "parser.cc"
    break;

  case 120: /* jump_statement: "konfirma" "(" ")" ";"  */
#line 284 "rules/parser.y"
                                          { auto n = new ast::KonfirmaSttmt(nullptr); n->LineNum = yylineno; (yyval.sttmt) = n; }
#line 2806 "parser.cc"
    break;

  case 121: /* struct_declaration: "molda" "valid identifier" "{" struct_fields "}"  */
#line 287 "rules/parser.y"
                                                             { (yyval.sttmt) = new ast::StructDeclSttmt(*(yyvsp[-3].string), *(yyvsp[-1].struct_fields)); (yyval.sttmt)->LineNum = yylineno; delete (yyvsp[-3].string); delete (yyvsp[-1].struct_fields); }
#line 2812 "parser.cc"
    break;

  case 122: /* struct_fields: %empty  */
#line 290 "rules/parser.y"
                       { (yyval.struct_fields) = new std::vector<ast::StructField>(); }
#line 2818 "parser.cc"
    break;

  case 123: /* struct_fields: struct_field_list  */
#line 291 "rules/parser.y"
                                  { (yyval.struct_fields) = (yyvsp[0].struct_fields); }
#line 2824 "parser.cc"
    break;

  case 124: /* struct_field_list: type_name declarator  */
#line 294 "rules/parser.y"
                                         { (yyval.struct_fields) = new std::vector<ast::StructField>(); (yyval.struct_fields)->push_back(ast::StructField(*(yyvsp[-1].string), *(yyvsp[0].string))); delete (yyvsp[-1].string); delete (yyvsp[0].string); }
#line 2830 "parser.cc"
    break;

  case 125: /* struct_field_list: struct_field_list "," type_name declarator  */
#line 295 "rules/parser.y"
                                                                 { (yyvsp[-3].struct_fields)->push_back(ast::StructField(*(yyvsp[-1].string), *(yyvsp[0].string))); (yyval.struct_fields) = (yyvsp[-3].struct_fields); delete (yyvsp[-1].string); delete (yyvsp[0].string); }
#line 2836 "parser.cc"
    break;

  case 126: /* struct_field_list: struct_field_list ","  */
#line 296 "rules/parser.y"
                                            { (yyval.struct_fields) = (yyvsp[-1].struct_fields); }
#line 2842 "parser.cc"
    break;

  case 127: /* impl_declaration: "impl" "valid identifier" "{" struct_methods "}"  */
#line 299 "rules/parser.y"
                                                           { (yyval.sttmt) = new ast::ImplSttmt(*(yyvsp[-3].string), std::move(*(yyvsp[-1].struct_methods))); (yyval.sttmt)->LineNum = yylineno; delete (yyvsp[-3].string); delete (yyvsp[-1].struct_methods); }
#line 2848 "parser.cc"
    break;

  case 128: /* struct_methods: %empty  */
#line 302 "rules/parser.y"
                        { (yyval.struct_methods) = new std::vector<std::unique_ptr<ast::FuncDeclSttmt>>(); }
#line 2854 "parser.cc"
    break;

  case 129: /* struct_methods: struct_method_list  */
#line 303 "rules/parser.y"
                                    { (yyval.struct_methods) = (yyvsp[0].struct_methods); }
#line 2860 "parser.cc"
    break;

  case 130: /* struct_method_list: function_declaration  */
#line 306 "rules/parser.y"
                                          { (yyval.struct_methods) = new std::vector<std::unique_ptr<ast::FuncDeclSttmt>>(); (yyval.struct_methods)->push_back(std::unique_ptr<ast::FuncDeclSttmt>(static_cast<ast::FuncDeclSttmt*>((yyvsp[0].sttmt)))); }
#line 2866 "parser.cc"
    break;

  case 131: /* struct_method_list: struct_method_list function_declaration  */
#line 307 "rules/parser.y"
                                                             { (yyvsp[-1].struct_methods)->push_back(std::unique_ptr<ast::FuncDeclSttmt>(static_cast<ast::FuncDeclSttmt*>((yyvsp[0].sttmt)))); (yyval.struct_methods) = (yyvsp[-1].struct_methods); }
#line 2872 "parser.cc"
    break;

  case 132: /* struct_literal: "valid identifier" "{" struct_inits "}"  */
#line 310 "rules/parser.y"
                                                  { (yyval.expr) = new ast::StructLiteralExpr(*(yyvsp[-3].string), std::move(*(yyvsp[-1].struct_inits))); (yyval.expr)->LineNum = yylineno; delete (yyvsp[-3].string); delete (yyvsp[-1].struct_inits); }
#line 2878 "parser.cc"
    break;

  case 133: /* struct_inits: %empty  */
#line 313 "rules/parser.y"
                      { (yyval.struct_inits) = new std::vector<ast::StructLiteralExpr::FieldInit>(); }
#line 2884 "parser.cc"
    break;

  case 134: /* struct_inits: struct_init_list  */
#line 314 "rules/parser.y"
                                { (yyval.struct_inits) = (yyvsp[0].struct_inits); }
#line 2890 "parser.cc"
    break;

  case 135: /* struct_init_list: "valid identifier" ":" expression  */
#line 317 "rules/parser.y"
                                          { (yyval.struct_inits) = new std::vector<ast::StructLiteralExpr::FieldInit>(); (yyval.struct_inits)->push_back({*(yyvsp[-2].string), std::unique_ptr<ast::Expr>((yyvsp[0].expr))}); delete (yyvsp[-2].string); }
#line 2896 "parser.cc"
    break;

  case 136: /* struct_init_list: struct_init_list "," "valid identifier" ":" expression  */
#line 318 "rules/parser.y"
                                                                 { (yyvsp[-4].struct_inits)->push_back({*(yyvsp[-2].string), std::unique_ptr<ast::Expr>((yyvsp[0].expr))}); (yyval.struct_inits) = (yyvsp[-4].struct_inits); delete (yyvsp[-2].string); }
#line 2902 "parser.cc"
    break;


#line 2906 "parser.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (Program, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, Program);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, Program);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (Program, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, Program);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, Program);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 320 "rules/parser.y"


void yyerror(kriol::ast::BlockSttmt** Program, const char* err) {
    kriol::cli::PrintErr(kriol::cli::GetSourceFile(), yylineno, err, 1);
}
