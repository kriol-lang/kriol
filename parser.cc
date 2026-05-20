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
    #include "include/kriol/codegen.hh"

    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <memory>

    extern int yylex();
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
  YYSYMBOL_IDENT = 3,                      /* IDENT  */
  YYSYMBOL_STR_LIT = 4,                    /* STR_LIT  */
  YYSYMBOL_MOSTRA = 5,                     /* MOSTRA  */
  YYSYMBOL_INT_LIT = 6,                    /* INT_LIT  */
  YYSYMBOL_FLOAT_LIT = 7,                  /* FLOAT_LIT  */
  YYSYMBOL_BOOL_LIT = 8,                   /* BOOL_LIT  */
  YYSYMBOL_PLUS = 9,                       /* PLUS  */
  YYSYMBOL_MINUS = 10,                     /* MINUS  */
  YYSYMBOL_MUL = 11,                       /* MUL  */
  YYSYMBOL_DIV = 12,                       /* DIV  */
  YYSYMBOL_EQ = 13,                        /* EQ  */
  YYSYMBOL_NE = 14,                        /* NE  */
  YYSYMBOL_LT = 15,                        /* LT  */
  YYSYMBOL_LE = 16,                        /* LE  */
  YYSYMBOL_GT = 17,                        /* GT  */
  YYSYMBOL_GE = 18,                        /* GE  */
  YYSYMBOL_AND = 19,                       /* AND  */
  YYSYMBOL_OR = 20,                        /* OR  */
  YYSYMBOL_ASSIGN = 21,                    /* ASSIGN  */
  YYSYMBOL_LCURLY = 22,                    /* LCURLY  */
  YYSYMBOL_RCURLY = 23,                    /* RCURLY  */
  YYSYMBOL_COMMA = 24,                     /* COMMA  */
  YYSYMBOL_SEMIC = 25,                     /* SEMIC  */
  YYSYMBOL_LBRAC = 26,                     /* LBRAC  */
  YYSYMBOL_RBRAC = 27,                     /* RBRAC  */
  YYSYMBOL_TYPE_NUM = 28,                  /* TYPE_NUM  */
  YYSYMBOL_TYPE_BOOL = 29,                 /* TYPE_BOOL  */
  YYSYMBOL_TYPE_VOID = 30,                 /* TYPE_VOID  */
  YYSYMBOL_DIVOLVI = 31,                   /* DIVOLVI  */
  YYSYMBOL_DI = 32,                        /* DI  */
  YYSYMBOL_STRUT = 33,                     /* STRUT  */
  YYSYMBOL_NKUANTU = 34,                   /* NKUANTU  */
  YYSYMBOL_SI = 35,                        /* SI  */
  YYSYMBOL_SINON = 36,                     /* SINON  */
  YYSYMBOL_IMPRISTAN = 37,                 /* IMPRISTAN  */
  YYSYMBOL_PARA = 38,                      /* PARA  */
  YYSYMBOL_CONTINUA = 39,                  /* CONTINUA  */
  YYSYMBOL_DOT = 40,                       /* DOT  */
  YYSYMBOL_RPAR = 41,                      /* RPAR  */
  YYSYMBOL_LPAR = 42,                      /* LPAR  */
  YYSYMBOL_UMINUS = 43,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 44,                  /* $accept  */
  YYSYMBOL_program = 45,                   /* program  */
  YYSYMBOL_type_specifier = 46,            /* type_specifier  */
  YYSYMBOL_constant = 47,                  /* constant  */
  YYSYMBOL_identifier = 48,                /* identifier  */
  YYSYMBOL_declarator = 49,                /* declarator  */
  YYSYMBOL_declaration = 50,               /* declaration  */
  YYSYMBOL_init_declarator = 51,           /* init_declarator  */
  YYSYMBOL_initializer = 52,               /* initializer  */
  YYSYMBOL_expression = 53,                /* expression  */
  YYSYMBOL_constant_expression = 54,       /* constant_expression  */
  YYSYMBOL_logical_or_expressions = 55,    /* logical_or_expressions  */
  YYSYMBOL_logical_and_expressions = 56,   /* logical_and_expressions  */
  YYSYMBOL_equality_expression = 57,       /* equality_expression  */
  YYSYMBOL_relational_expression = 58,     /* relational_expression  */
  YYSYMBOL_additive_expression = 59,       /* additive_expression  */
  YYSYMBOL_multiplicative_expression = 60, /* multiplicative_expression  */
  YYSYMBOL_unary_expression = 61,          /* unary_expression  */
  YYSYMBOL_primary_expression = 62,        /* primary_expression  */
  YYSYMBOL_assignment_expression = 63,     /* assignment_expression  */
  YYSYMBOL_assignment_operator = 64,       /* assignment_operator  */
  YYSYMBOL_function_declaration = 65,      /* function_declaration  */
  YYSYMBOL_parameter_optional_list = 66,   /* parameter_optional_list  */
  YYSYMBOL_parameter_list = 67,            /* parameter_list  */
  YYSYMBOL_parameter_declaration = 68,     /* parameter_declaration  */
  YYSYMBOL_argument_list = 69,             /* argument_list  */
  YYSYMBOL_function_call = 70,             /* function_call  */
  YYSYMBOL_mostra_func_call = 71,          /* mostra_func_call  */
  YYSYMBOL_statements = 72,                /* statements  */
  YYSYMBOL_statement = 73,                 /* statement  */
  YYSYMBOL_import_statement = 74,          /* import_statement  */
  YYSYMBOL_single_import = 75,             /* single_import  */
  YYSYMBOL_expression_statement = 76,      /* expression_statement  */
  YYSYMBOL_compound_statement = 77,        /* compound_statement  */
  YYSYMBOL_selection_statement = 78,       /* selection_statement  */
  YYSYMBOL_else_then = 79,                 /* else_then  */
  YYSYMBOL_iteration_statement = 80,       /* iteration_statement  */
  YYSYMBOL_jump_statement = 81             /* jump_statement  */
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
#define YYFINAL  63
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   249

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  38
/* YYNRULES -- Number of rules.  */
#define YYNRULES  84
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  135

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   298


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
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    73,    73,    74,    77,    78,    79,    82,    83,    84,
      85,    88,    91,    94,    97,    98,   101,   104,   105,   108,
     111,   112,   115,   116,   119,   120,   121,   124,   125,   126,
     127,   128,   131,   132,   133,   136,   137,   138,   141,   144,
     145,   146,   149,   150,   153,   156,   159,   160,   163,   164,
     167,   170,   171,   174,   175,   176,   179,   180,   183,   184,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   198,
     201,   204,   205,   208,   209,   212,   213,   216,   217,   220,
     221,   224,   225,   226,   227
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
  "\"end of file\"", "error", "\"invalid token\"", "IDENT", "STR_LIT",
  "MOSTRA", "INT_LIT", "FLOAT_LIT", "BOOL_LIT", "PLUS", "MINUS", "MUL",
  "DIV", "EQ", "NE", "LT", "LE", "GT", "GE", "AND", "OR", "ASSIGN",
  "LCURLY", "RCURLY", "COMMA", "SEMIC", "LBRAC", "RBRAC", "TYPE_NUM",
  "TYPE_BOOL", "TYPE_VOID", "DIVOLVI", "DI", "STRUT", "NKUANTU", "SI",
  "SINON", "IMPRISTAN", "PARA", "CONTINUA", "DOT", "RPAR", "LPAR",
  "UMINUS", "$accept", "program", "type_specifier", "constant",
  "identifier", "declarator", "declaration", "init_declarator",
  "initializer", "expression", "constant_expression",
  "logical_or_expressions", "logical_and_expressions",
  "equality_expression", "relational_expression", "additive_expression",
  "multiplicative_expression", "unary_expression", "primary_expression",
  "assignment_expression", "assignment_operator", "function_declaration",
  "parameter_optional_list", "parameter_list", "parameter_declaration",
  "argument_list", "function_call", "mostra_func_call", "statements",
  "statement", "import_statement", "single_import", "expression_statement",
  "compound_statement", "selection_statement", "else_then",
  "iteration_statement", "jump_statement", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-60)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-3)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     207,    22,   -60,   -60,    -4,   -60,   -60,   -60,   127,   -60,
     -60,   -60,   -60,     9,    25,    25,    25,    66,    54,    61,
      25,    89,    93,   -60,    65,   -60,    83,   -60,    90,    94,
      13,    57,    43,    70,   -60,    99,   -60,   -60,   -60,   -60,
      87,   -60,   -60,   -60,   -60,   -60,   -60,   -60,   -60,    36,
      22,   -60,   167,   -60,    98,   102,    92,    92,   -60,   -60,
     -60,   -60,    95,   -60,   -60,   -17,   -60,   135,   -60,    42,
      42,    42,    42,    42,    42,    42,    42,    42,    42,    42,
      42,   -60,    42,   -60,   -60,   -60,   -19,   -60,   -60,    25,
     -60,   101,   -60,    25,   -10,   -60,   -18,   -60,    94,   -60,
      13,    57,    57,    43,    43,    43,    43,    70,    70,   -60,
     -60,   -60,    25,   -60,   119,     2,   -60,   -60,    93,   104,
     122,   -60,   -60,   -60,    25,   -60,   -60,   -60,   -60,    92,
     -10,    92,   -60,   -60,   -60
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     3,    11,    10,     0,     7,     8,     9,     0,    72,
       4,     6,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    40,    39,    66,     0,    42,    19,    20,
      22,    24,    27,    32,    35,    38,    17,    65,    18,    55,
       0,    59,    67,    60,    61,    62,    63,    64,    68,     0,
       0,    74,     0,    84,     0,     0,     0,     0,    70,    69,
      81,    82,     0,     1,    12,    14,    13,     0,    71,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    44,     0,    58,    57,    52,     0,    73,    83,     0,
      79,    75,    41,     0,    47,    54,     0,    39,    21,    38,
      23,    25,    26,    28,    30,    29,    31,    33,    34,    36,
      37,    43,     0,    56,     0,     0,    15,    16,     0,     0,
      46,    48,    53,    51,     0,    77,    78,    76,    50,     0,
       0,     0,    45,    49,    80
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -60,   -60,   -59,   -60,   -14,    29,   -60,   -60,   -60,   -13,
     -60,   -60,    79,    81,    26,    27,    28,   -60,   147,    71,
     -60,   -60,   -60,   -60,    24,    96,   -60,   -60,   152,   -31,
     -60,   -60,   -60,   -46,    52,   -60,   -60,   -60
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    21,    22,    23,    24,    65,    25,    66,   116,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      82,    37,   119,   120,   121,    86,    38,    39,    40,    41,
      42,    59,    43,    44,    45,   127,    46,    47
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      54,    55,    56,    57,    93,   112,   112,    62,    64,    83,
      90,    91,     2,     3,     4,     5,     6,     7,    10,    11,
      12,    83,   113,   122,     8,    94,    71,    72,     2,     3,
       4,     5,     6,     7,    53,   118,    85,    16,    49,     2,
       3,     4,     5,     6,     7,     2,     3,    48,     5,     6,
       7,    20,    77,    78,    85,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    20,    97,   125,
      58,   118,    73,    74,    75,    76,   114,    84,    20,    60,
     117,    79,    80,   132,    20,   134,    61,    -2,    50,    63,
       2,     3,     4,     5,     6,     7,     2,   101,   102,   123,
     103,   104,   105,   106,    64,   107,   108,    67,    68,     8,
      69,   131,     9,    70,     8,    10,    11,    12,    13,    14,
      81,    15,    16,    88,    17,    18,    19,    89,    50,    20,
       2,     3,     4,     5,     6,     7,    92,   115,     2,     3,
       4,     5,     6,     7,   124,   129,   130,   128,    98,     8,
      51,   100,     9,   111,   133,    10,    11,    12,    13,    14,
      52,    15,    16,    96,    17,    18,    19,   126,    50,    20,
       2,     3,     4,     5,     6,     7,    95,    20,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
      87,     0,     9,     0,     0,    10,    11,    12,    13,    14,
       0,    15,    16,     0,    17,    18,    19,     0,     1,    20,
       2,     3,     4,     5,     6,     7,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,   109,   110,     0,     8,
       0,     0,     9,     0,     0,    10,    11,    12,    13,    14,
       0,    15,    16,     0,    17,    18,    19,     0,     0,    20
};

static const yytype_int16 yycheck[] =
{
      13,    14,    15,    16,    21,    24,    24,    20,    22,    40,
      56,    57,     3,     4,     5,     6,     7,     8,    28,    29,
      30,    52,    41,    41,    22,    42,    13,    14,     3,     4,
       5,     6,     7,     8,    25,    94,    49,    35,    42,     3,
       4,     5,     6,     7,     8,     3,     4,    25,     6,     7,
       8,    42,     9,    10,    67,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    42,    82,   115,
       4,   130,    15,    16,    17,    18,    89,    41,    42,    25,
      93,    11,    12,   129,    42,   131,    25,     0,     1,     0,
       3,     4,     5,     6,     7,     8,     3,    71,    72,   112,
      73,    74,    75,    76,   118,    77,    78,    42,    25,    22,
      20,   124,    25,    19,    22,    28,    29,    30,    31,    32,
      21,    34,    35,    25,    37,    38,    39,    25,     1,    42,
       3,     4,     5,     6,     7,     8,    41,    36,     3,     4,
       5,     6,     7,     8,    25,    41,    24,   118,    69,    22,
      23,    70,    25,    82,   130,    28,    29,    30,    31,    32,
       8,    34,    35,    67,    37,    38,    39,   115,     1,    42,
       3,     4,     5,     6,     7,     8,    41,    42,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    -1,    25,    -1,    -1,    28,    29,    30,    31,    32,
      -1,    34,    35,    -1,    37,    38,    39,    -1,     1,    42,
       3,     4,     5,     6,     7,     8,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    -1,    22,
      -1,    -1,    25,    -1,    -1,    28,    29,    30,    31,    32,
      -1,    34,    35,    -1,    37,    38,    39,    -1,    -1,    42
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,     3,     4,     5,     6,     7,     8,    22,    25,
      28,    29,    30,    31,    32,    34,    35,    37,    38,    39,
      42,    45,    46,    47,    48,    50,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    65,    70,    71,
      72,    73,    74,    76,    77,    78,    80,    81,    25,    42,
       1,    23,    72,    25,    53,    53,    53,    53,     4,    75,
      25,    25,    53,     0,    48,    49,    51,    42,    25,    20,
      19,    13,    14,    15,    16,    17,    18,     9,    10,    11,
      12,    21,    64,    73,    41,    53,    69,    23,    25,    25,
      77,    77,    41,    21,    42,    41,    69,    48,    56,    62,
      57,    58,    58,    59,    59,    59,    59,    60,    60,    62,
      62,    63,    24,    41,    53,    36,    52,    53,    46,    66,
      67,    68,    41,    53,    25,    77,    78,    79,    49,    41,
      24,    53,    77,    68,    77
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    44,    45,    45,    46,    46,    46,    47,    47,    47,
      47,    48,    49,    50,    51,    51,    52,    53,    53,    54,
      55,    55,    56,    56,    57,    57,    57,    58,    58,    58,
      58,    58,    59,    59,    59,    60,    60,    60,    61,    62,
      62,    62,    63,    63,    64,    65,    66,    66,    67,    67,
      68,    69,    69,    70,    70,    70,    71,    71,    72,    72,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    74,
      75,    76,    76,    77,    77,    78,    78,    79,    79,    80,
      80,    81,    81,    81,    81
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     3,     1,     1,     1,     1,
       1,     3,     1,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     1,     3,     3,     1,     3,     3,     1,     1,
       1,     3,     1,     3,     1,     6,     1,     0,     1,     3,
       2,     3,     1,     4,     3,     1,     4,     3,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       1,     2,     1,     3,     2,     3,     5,     1,     1,     3,
       7,     2,     2,     3,     2
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
    case YYSYMBOL_IDENT: /* IDENT  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1287 "parser.cc"
        break;

    case YYSYMBOL_STR_LIT: /* STR_LIT  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1293 "parser.cc"
        break;

    case YYSYMBOL_INT_LIT: /* INT_LIT  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).integer); }
#line 1299 "parser.cc"
        break;

    case YYSYMBOL_FLOAT_LIT: /* FLOAT_LIT  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).floatingpoint); }
#line 1305 "parser.cc"
        break;

    case YYSYMBOL_BOOL_LIT: /* BOOL_LIT  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).boolean); }
#line 1311 "parser.cc"
        break;

    case YYSYMBOL_TYPE_NUM: /* TYPE_NUM  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1317 "parser.cc"
        break;

    case YYSYMBOL_TYPE_BOOL: /* TYPE_BOOL  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1323 "parser.cc"
        break;

    case YYSYMBOL_TYPE_VOID: /* TYPE_VOID  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1329 "parser.cc"
        break;

    case YYSYMBOL_type_specifier: /* type_specifier  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1335 "parser.cc"
        break;

    case YYSYMBOL_constant: /* constant  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1341 "parser.cc"
        break;

    case YYSYMBOL_identifier: /* identifier  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1347 "parser.cc"
        break;

    case YYSYMBOL_declarator: /* declarator  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1353 "parser.cc"
        break;

    case YYSYMBOL_declaration: /* declaration  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1359 "parser.cc"
        break;

    case YYSYMBOL_init_declarator: /* init_declarator  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).vardecl); }
#line 1365 "parser.cc"
        break;

    case YYSYMBOL_initializer: /* initializer  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1371 "parser.cc"
        break;

    case YYSYMBOL_expression: /* expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1377 "parser.cc"
        break;

    case YYSYMBOL_constant_expression: /* constant_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1383 "parser.cc"
        break;

    case YYSYMBOL_logical_or_expressions: /* logical_or_expressions  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1389 "parser.cc"
        break;

    case YYSYMBOL_logical_and_expressions: /* logical_and_expressions  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1395 "parser.cc"
        break;

    case YYSYMBOL_equality_expression: /* equality_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1401 "parser.cc"
        break;

    case YYSYMBOL_relational_expression: /* relational_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1407 "parser.cc"
        break;

    case YYSYMBOL_additive_expression: /* additive_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1413 "parser.cc"
        break;

    case YYSYMBOL_multiplicative_expression: /* multiplicative_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1419 "parser.cc"
        break;

    case YYSYMBOL_unary_expression: /* unary_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1425 "parser.cc"
        break;

    case YYSYMBOL_primary_expression: /* primary_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1431 "parser.cc"
        break;

    case YYSYMBOL_assignment_expression: /* assignment_expression  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1437 "parser.cc"
        break;

    case YYSYMBOL_assignment_operator: /* assignment_operator  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1443 "parser.cc"
        break;

    case YYSYMBOL_function_declaration: /* function_declaration  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1449 "parser.cc"
        break;

    case YYSYMBOL_parameter_optional_list: /* parameter_optional_list  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).params); }
#line 1455 "parser.cc"
        break;

    case YYSYMBOL_parameter_list: /* parameter_list  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).params); }
#line 1461 "parser.cc"
        break;

    case YYSYMBOL_parameter_declaration: /* parameter_declaration  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).vardecl); }
#line 1467 "parser.cc"
        break;

    case YYSYMBOL_argument_list: /* argument_list  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).args); }
#line 1473 "parser.cc"
        break;

    case YYSYMBOL_function_call: /* function_call  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1479 "parser.cc"
        break;

    case YYSYMBOL_mostra_func_call: /* mostra_func_call  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).expr); }
#line 1485 "parser.cc"
        break;

    case YYSYMBOL_statements: /* statements  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).block); }
#line 1491 "parser.cc"
        break;

    case YYSYMBOL_statement: /* statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1497 "parser.cc"
        break;

    case YYSYMBOL_import_statement: /* import_statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1503 "parser.cc"
        break;

    case YYSYMBOL_single_import: /* single_import  */
#line 34 "rules/parser.y"
            { delete ((*yyvaluep).string); }
#line 1509 "parser.cc"
        break;

    case YYSYMBOL_expression_statement: /* expression_statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1515 "parser.cc"
        break;

    case YYSYMBOL_compound_statement: /* compound_statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).block); }
#line 1521 "parser.cc"
        break;

    case YYSYMBOL_selection_statement: /* selection_statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1527 "parser.cc"
        break;

    case YYSYMBOL_else_then: /* else_then  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).block); }
#line 1533 "parser.cc"
        break;

    case YYSYMBOL_iteration_statement: /* iteration_statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1539 "parser.cc"
        break;

    case YYSYMBOL_jump_statement: /* jump_statement  */
#line 35 "rules/parser.y"
            { delete ((*yyvaluep).sttmt); }
#line 1545 "parser.cc"
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
#line 73 "rules/parser.y"
                     { *Program = (yyvsp[0].block); }
#line 1818 "parser.cc"
    break;

  case 3: /* program: error  */
#line 74 "rules/parser.y"
                { *Program = nullptr; }
#line 1824 "parser.cc"
    break;

  case 4: /* type_specifier: TYPE_NUM  */
#line 77 "rules/parser.y"
                          { (yyval.string) = (yyvsp[0].string); }
#line 1830 "parser.cc"
    break;

  case 5: /* type_specifier: TYPE_VOID  */
#line 78 "rules/parser.y"
                           { (yyval.string) = (yyvsp[0].string); }
#line 1836 "parser.cc"
    break;

  case 6: /* type_specifier: TYPE_BOOL  */
#line 79 "rules/parser.y"
                           { (yyval.string) = new std::string("unsigned short"); delete (yyvsp[0].string); }
#line 1842 "parser.cc"
    break;

  case 7: /* constant: INT_LIT  */
#line 82 "rules/parser.y"
                   { auto lit = new ast::LiteralExpr("int", *(yyvsp[0].integer)); lit->ActivateAutoCast(); (yyval.expr) = lit; delete (yyvsp[0].integer); }
#line 1848 "parser.cc"
    break;

  case 8: /* constant: FLOAT_LIT  */
#line 83 "rules/parser.y"
                     { auto lit = new ast::LiteralExpr("float", *(yyvsp[0].floatingpoint)); lit->ActivateAutoCast(); (yyval.expr) = lit; delete (yyvsp[0].floatingpoint); }
#line 1854 "parser.cc"
    break;

  case 9: /* constant: BOOL_LIT  */
#line 84 "rules/parser.y"
                    { auto lit = new ast::LiteralExpr("unsigned short", *(yyvsp[0].boolean)); lit->ActivateAutoCast(); (yyval.expr) = lit; delete (yyvsp[0].boolean); }
#line 1860 "parser.cc"
    break;

  case 10: /* constant: STR_LIT  */
#line 85 "rules/parser.y"
                   { auto lit = new ast::LiteralExpr("char*", *(yyvsp[0].string)); lit->DeactivateAutoCast(); (yyval.expr) = lit; delete (yyvsp[0].string); }
#line 1866 "parser.cc"
    break;

  case 11: /* identifier: IDENT  */
#line 88 "rules/parser.y"
                   { (yyval.string) = (yyvsp[0].string); }
#line 1872 "parser.cc"
    break;

  case 12: /* declarator: identifier  */
#line 91 "rules/parser.y"
                        { (yyval.string) = (yyvsp[0].string); }
#line 1878 "parser.cc"
    break;

  case 13: /* declaration: type_specifier init_declarator  */
#line 94 "rules/parser.y"
                                             { (yyvsp[0].vardecl)->SetType(*(yyvsp[-1].string)); (yyval.sttmt) = (yyvsp[0].vardecl); delete (yyvsp[-1].string); }
#line 1884 "parser.cc"
    break;

  case 14: /* init_declarator: declarator  */
#line 97 "rules/parser.y"
                             { (yyval.vardecl) = new ast::VarDeclSttmt("void", *(yyvsp[0].string), nullptr); delete (yyvsp[0].string); }
#line 1890 "parser.cc"
    break;

  case 15: /* init_declarator: declarator ASSIGN initializer  */
#line 98 "rules/parser.y"
                                                { (yyval.vardecl) = new ast::VarDeclSttmt("void", *(yyvsp[-2].string), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); delete (yyvsp[-2].string); }
#line 1896 "parser.cc"
    break;

  case 16: /* initializer: expression  */
#line 101 "rules/parser.y"
                         { (yyval.expr) = (yyvsp[0].expr); }
#line 1902 "parser.cc"
    break;

  case 17: /* expression: assignment_expression  */
#line 104 "rules/parser.y"
                                   { (yyval.expr) = (yyvsp[0].expr); }
#line 1908 "parser.cc"
    break;

  case 18: /* expression: function_call  */
#line 105 "rules/parser.y"
                           { (yyval.expr) = (yyvsp[0].expr); }
#line 1914 "parser.cc"
    break;

  case 19: /* constant_expression: logical_or_expressions  */
#line 108 "rules/parser.y"
                                             { (yyval.expr) = (yyvsp[0].expr); }
#line 1920 "parser.cc"
    break;

  case 20: /* logical_or_expressions: logical_and_expressions  */
#line 111 "rules/parser.y"
                                                 { (yyval.expr) = (yyvsp[0].expr); }
#line 1926 "parser.cc"
    break;

  case 21: /* logical_or_expressions: logical_or_expressions OR logical_and_expressions  */
#line 112 "rules/parser.y"
                                                                           { (yyval.expr) = new ast::BinExpr("||", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1932 "parser.cc"
    break;

  case 22: /* logical_and_expressions: equality_expression  */
#line 115 "rules/parser.y"
                                              { (yyval.expr) = (yyvsp[0].expr); }
#line 1938 "parser.cc"
    break;

  case 23: /* logical_and_expressions: logical_and_expressions AND equality_expression  */
#line 116 "rules/parser.y"
                                                                          { (yyval.expr) = new ast::BinExpr("&&", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1944 "parser.cc"
    break;

  case 24: /* equality_expression: relational_expression  */
#line 119 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[0].expr); }
#line 1950 "parser.cc"
    break;

  case 25: /* equality_expression: equality_expression EQ relational_expression  */
#line 120 "rules/parser.y"
                                                                   { (yyval.expr) = new ast::BinExpr("==", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1956 "parser.cc"
    break;

  case 26: /* equality_expression: equality_expression NE relational_expression  */
#line 121 "rules/parser.y"
                                                                   { (yyval.expr) = new ast::BinExpr("!=", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1962 "parser.cc"
    break;

  case 27: /* relational_expression: additive_expression  */
#line 124 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[0].expr); }
#line 1968 "parser.cc"
    break;

  case 28: /* relational_expression: relational_expression LT additive_expression  */
#line 125 "rules/parser.y"
                                                                     { (yyval.expr) = new ast::BinExpr("<", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1974 "parser.cc"
    break;

  case 29: /* relational_expression: relational_expression GT additive_expression  */
#line 126 "rules/parser.y"
                                                                     { (yyval.expr) = new ast::BinExpr(">", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1980 "parser.cc"
    break;

  case 30: /* relational_expression: relational_expression LE additive_expression  */
#line 127 "rules/parser.y"
                                                                     { (yyval.expr) = new ast::BinExpr("<=", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1986 "parser.cc"
    break;

  case 31: /* relational_expression: relational_expression GE additive_expression  */
#line 128 "rules/parser.y"
                                                                     { (yyval.expr) = new ast::BinExpr(">=", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 1992 "parser.cc"
    break;

  case 32: /* additive_expression: multiplicative_expression  */
#line 131 "rules/parser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 1998 "parser.cc"
    break;

  case 33: /* additive_expression: additive_expression PLUS multiplicative_expression  */
#line 132 "rules/parser.y"
                                                                         { (yyval.expr) = new ast::BinExpr("+", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 2004 "parser.cc"
    break;

  case 34: /* additive_expression: additive_expression MINUS multiplicative_expression  */
#line 133 "rules/parser.y"
                                                                          { (yyval.expr) = new ast::BinExpr("-", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 2010 "parser.cc"
    break;

  case 35: /* multiplicative_expression: unary_expression  */
#line 136 "rules/parser.y"
                                             { (yyval.expr) = (yyvsp[0].expr); }
#line 2016 "parser.cc"
    break;

  case 36: /* multiplicative_expression: multiplicative_expression MUL primary_expression  */
#line 137 "rules/parser.y"
                                                                             { (yyval.expr) = new ast::BinExpr("*", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 2022 "parser.cc"
    break;

  case 37: /* multiplicative_expression: multiplicative_expression DIV primary_expression  */
#line 138 "rules/parser.y"
                                                                             { (yyval.expr) = new ast::BinExpr("/", std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 2028 "parser.cc"
    break;

  case 38: /* unary_expression: primary_expression  */
#line 141 "rules/parser.y"
                                      { (yyval.expr) = (yyvsp[0].expr); }
#line 2034 "parser.cc"
    break;

  case 39: /* primary_expression: identifier  */
#line 144 "rules/parser.y"
                                { (yyval.expr) = new ast::IdentExpr(*(yyvsp[0].string)); delete (yyvsp[0].string); }
#line 2040 "parser.cc"
    break;

  case 40: /* primary_expression: constant  */
#line 145 "rules/parser.y"
                              { (yyval.expr) = (yyvsp[0].expr); }
#line 2046 "parser.cc"
    break;

  case 41: /* primary_expression: LPAR expression RPAR  */
#line 146 "rules/parser.y"
                                          { (yyval.expr) = new ast::ParExpr(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); }
#line 2052 "parser.cc"
    break;

  case 42: /* assignment_expression: constant_expression  */
#line 149 "rules/parser.y"
                                            { (yyval.expr) = (yyvsp[0].expr); }
#line 2058 "parser.cc"
    break;

  case 43: /* assignment_expression: primary_expression assignment_operator assignment_expression  */
#line 150 "rules/parser.y"
                                                                                     { (yyval.expr) = new ast::AssignExpr(*(yyvsp[-1].string), std::unique_ptr<ast::Expr>((yyvsp[-2].expr)), std::unique_ptr<ast::Expr>((yyvsp[0].expr))); delete (yyvsp[-1].string); }
#line 2064 "parser.cc"
    break;

  case 44: /* assignment_operator: ASSIGN  */
#line 153 "rules/parser.y"
                             { (yyval.string) = new std::string("=", 2); }
#line 2070 "parser.cc"
    break;

  case 45: /* function_declaration: type_specifier declarator LPAR parameter_optional_list RPAR compound_statement  */
#line 156 "rules/parser.y"
                                                                                                      { (yyval.sttmt) = new ast::FuncDeclSttmt(*(yyvsp[-5].string), *(yyvsp[-4].string), std::unique_ptr<ast::FuncArgs>((yyvsp[-2].params)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); delete (yyvsp[-5].string); delete (yyvsp[-4].string); }
#line 2076 "parser.cc"
    break;

  case 46: /* parameter_optional_list: parameter_list  */
#line 159 "rules/parser.y"
                                         { (yyval.params) = (yyvsp[0].params); }
#line 2082 "parser.cc"
    break;

  case 47: /* parameter_optional_list: %empty  */
#line 160 "rules/parser.y"
                                 { (yyval.params) = new ast::FuncArgs(); }
#line 2088 "parser.cc"
    break;

  case 48: /* parameter_list: parameter_declaration  */
#line 163 "rules/parser.y"
                                       { (yyval.params) = new ast::FuncArgs(); (yyval.params)->AddArg(std::unique_ptr<ast::VarDeclSttmt>((yyvsp[0].vardecl))); }
#line 2094 "parser.cc"
    break;

  case 49: /* parameter_list: parameter_list COMMA parameter_declaration  */
#line 164 "rules/parser.y"
                                                            { (yyvsp[-2].params)->AddArg(std::unique_ptr<ast::VarDeclSttmt>((yyvsp[0].vardecl))); (yyval.params) = (yyvsp[-2].params); }
#line 2100 "parser.cc"
    break;

  case 50: /* parameter_declaration: type_specifier declarator  */
#line 167 "rules/parser.y"
                                                  { (yyval.vardecl) = new ast::VarDeclSttmt(*(yyvsp[-1].string), *(yyvsp[0].string), nullptr); delete (yyvsp[-1].string); delete (yyvsp[0].string); }
#line 2106 "parser.cc"
    break;

  case 51: /* argument_list: argument_list COMMA expression  */
#line 170 "rules/parser.y"
                                               { (yyvsp[-2].args)->AddArg(std::unique_ptr<ast::Expr>((yyvsp[0].expr))); (yyval.args) = (yyvsp[-2].args); }
#line 2112 "parser.cc"
    break;

  case 52: /* argument_list: expression  */
#line 171 "rules/parser.y"
                           { (yyval.args) = new ast::FuncCallArgs(); (yyval.args)->AddArg(std::unique_ptr<ast::Expr>((yyvsp[0].expr))); }
#line 2118 "parser.cc"
    break;

  case 53: /* function_call: identifier LPAR argument_list RPAR  */
#line 174 "rules/parser.y"
                                                   { (yyval.expr) = new ast::FunCallExpr(*(yyvsp[-3].string), std::unique_ptr<ast::FuncCallArgs>((yyvsp[-1].args))); delete (yyvsp[-3].string); }
#line 2124 "parser.cc"
    break;

  case 54: /* function_call: identifier LPAR RPAR  */
#line 175 "rules/parser.y"
                                     { (yyval.expr) = new ast::FunCallExpr(*(yyvsp[-2].string), nullptr); delete (yyvsp[-2].string); }
#line 2130 "parser.cc"
    break;

  case 55: /* function_call: mostra_func_call  */
#line 176 "rules/parser.y"
                                 { (yyval.expr) = (yyvsp[0].expr); }
#line 2136 "parser.cc"
    break;

  case 56: /* mostra_func_call: MOSTRA LPAR argument_list RPAR  */
#line 179 "rules/parser.y"
                                                  { (yyval.expr) = new ast::MostraFunCallExpr(std::unique_ptr<ast::FuncCallArgs>((yyvsp[-1].args))); }
#line 2142 "parser.cc"
    break;

  case 57: /* mostra_func_call: MOSTRA LPAR RPAR  */
#line 180 "rules/parser.y"
                                    { (yyval.expr) = new ast::MostraFunCallExpr(nullptr); }
#line 2148 "parser.cc"
    break;

  case 58: /* statements: statements statement  */
#line 183 "rules/parser.y"
                                  { (yyvsp[-1].block)->AddSttmt(std::unique_ptr<ast::Sttmt>((yyvsp[0].sttmt))); (yyval.block) = (yyvsp[-1].block); }
#line 2154 "parser.cc"
    break;

  case 59: /* statements: statement  */
#line 184 "rules/parser.y"
                       { (yyval.block) = new ast::BlockSttmt(); (yyval.block)->AddSttmt(std::unique_ptr<ast::Sttmt>((yyvsp[0].sttmt))); }
#line 2160 "parser.cc"
    break;

  case 60: /* statement: expression_statement  */
#line 187 "rules/parser.y"
                                 { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2166 "parser.cc"
    break;

  case 61: /* statement: compound_statement  */
#line 188 "rules/parser.y"
                               { (yyval.sttmt) = (yyvsp[0].block); }
#line 2172 "parser.cc"
    break;

  case 62: /* statement: selection_statement  */
#line 189 "rules/parser.y"
                                { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2178 "parser.cc"
    break;

  case 63: /* statement: iteration_statement  */
#line 190 "rules/parser.y"
                                { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2184 "parser.cc"
    break;

  case 64: /* statement: jump_statement  */
#line 191 "rules/parser.y"
                           { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2190 "parser.cc"
    break;

  case 65: /* statement: function_declaration  */
#line 192 "rules/parser.y"
                                 { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2196 "parser.cc"
    break;

  case 66: /* statement: declaration  */
#line 193 "rules/parser.y"
                        { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2202 "parser.cc"
    break;

  case 67: /* statement: import_statement  */
#line 194 "rules/parser.y"
                             { (yyval.sttmt) = (yyvsp[0].sttmt); }
#line 2208 "parser.cc"
    break;

  case 68: /* statement: error SEMIC  */
#line 195 "rules/parser.y"
                        { (yyval.sttmt) = nullptr; }
#line 2214 "parser.cc"
    break;

  case 69: /* import_statement: IMPRISTAN single_import  */
#line 198 "rules/parser.y"
                                           { (yyval.sttmt) = new ast::ImportSttmt(*(yyvsp[0].string)); delete (yyvsp[0].string); }
#line 2220 "parser.cc"
    break;

  case 70: /* single_import: STR_LIT  */
#line 201 "rules/parser.y"
                        { (yyval.string) = (yyvsp[0].string); }
#line 2226 "parser.cc"
    break;

  case 71: /* expression_statement: expression SEMIC  */
#line 204 "rules/parser.y"
                                        { (yyval.sttmt) = new ast::ExprSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); }
#line 2232 "parser.cc"
    break;

  case 72: /* expression_statement: SEMIC  */
#line 205 "rules/parser.y"
                             { (yyval.sttmt) = new ast::ExprSttmt(nullptr); }
#line 2238 "parser.cc"
    break;

  case 73: /* compound_statement: LCURLY statements RCURLY  */
#line 208 "rules/parser.y"
                                              { (yyval.block) = (yyvsp[-1].block); (yyval.block)->UseBrackets(); }
#line 2244 "parser.cc"
    break;

  case 74: /* compound_statement: LCURLY RCURLY  */
#line 209 "rules/parser.y"
                                   { (yyval.block) = new ast::BlockSttmt(); (yyval.block)->UseBrackets(); }
#line 2250 "parser.cc"
    break;

  case 75: /* selection_statement: SI expression compound_statement  */
#line 212 "rules/parser.y"
                                                       { (yyval.sttmt) = new ast::IfSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block)), nullptr); }
#line 2256 "parser.cc"
    break;

  case 76: /* selection_statement: SI expression compound_statement SINON else_then  */
#line 213 "rules/parser.y"
                                                                       { (yyval.sttmt) = new ast::IfSttmt(std::unique_ptr<ast::Expr>((yyvsp[-3].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[-2].block)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); }
#line 2262 "parser.cc"
    break;

  case 77: /* else_then: compound_statement  */
#line 216 "rules/parser.y"
                               { (yyval.block) = (yyvsp[0].block); }
#line 2268 "parser.cc"
    break;

  case 78: /* else_then: selection_statement  */
#line 217 "rules/parser.y"
                                { (yyval.block) = new ast::BlockSttmt(); (yyval.block)->AddSttmt(std::unique_ptr<ast::Sttmt>((yyvsp[0].sttmt))); }
#line 2274 "parser.cc"
    break;

  case 79: /* iteration_statement: NKUANTU expression compound_statement  */
#line 220 "rules/parser.y"
                                                            { (yyval.sttmt) = new ast::WhileSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); }
#line 2280 "parser.cc"
    break;

  case 80: /* iteration_statement: DI expression SEMIC expression SEMIC expression compound_statement  */
#line 221 "rules/parser.y"
                                                                                         { (yyval.sttmt) = new ast::ForSttmt(std::unique_ptr<ast::Expr>((yyvsp[-5].expr)), std::unique_ptr<ast::Expr>((yyvsp[-3].expr)), std::unique_ptr<ast::Expr>((yyvsp[-1].expr)), std::unique_ptr<ast::BlockSttmt>((yyvsp[0].block))); }
#line 2286 "parser.cc"
    break;

  case 81: /* jump_statement: PARA SEMIC  */
#line 224 "rules/parser.y"
                            { (yyval.sttmt) = new ast::JumpSttmt("break"); }
#line 2292 "parser.cc"
    break;

  case 82: /* jump_statement: CONTINUA SEMIC  */
#line 225 "rules/parser.y"
                                { (yyval.sttmt) = new ast::JumpSttmt("continue"); }
#line 2298 "parser.cc"
    break;

  case 83: /* jump_statement: DIVOLVI expression SEMIC  */
#line 226 "rules/parser.y"
                                          { (yyval.sttmt) = new ast::ReturnSttmt(std::unique_ptr<ast::Expr>((yyvsp[-1].expr))); }
#line 2304 "parser.cc"
    break;

  case 84: /* jump_statement: DIVOLVI SEMIC  */
#line 227 "rules/parser.y"
                               { (yyval.sttmt) = new ast::ReturnSttmt(nullptr); }
#line 2310 "parser.cc"
    break;


#line 2314 "parser.cc"

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

#line 229 "rules/parser.y"


void yyerror(kriol::ast::BlockSttmt** Program, const char* err) {
    kriol::cli::PrintErr(err, 1);
}
