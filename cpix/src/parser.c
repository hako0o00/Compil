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
#line 1 "src/parser.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"

extern int yylex(void);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;
extern const char* g_filename;

void yyerror(const char* s);

typedef struct {
    char* Lstep; 
    char* Lcond; 
    char* Lend;  
} loopctx_t;

static loopctx_t loopstk[256];
static int loopsp = 0;

static void loop_push3(char* Lstep, char* Lcond, char* Lend) {
    if (loopsp >= 256) { fprintf(stderr, "parser error: loop nesting too deep\n"); exit(1); }
    loopstk[loopsp].Lstep = Lstep;
    loopstk[loopsp].Lcond = Lcond;
    loopstk[loopsp].Lend  = Lend;
    loopsp++;
}

static void loop_pop3(void) {
    if (loopsp <= 0) { fprintf(stderr, "parser error: loop stack underflow\n"); exit(1); }
    loopctx_t c = loopstk[--loopsp];
    free(c.Lstep);
    free(c.Lcond);
    free(c.Lend);
}

static char* xstrdup2(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n);
    return p;
}

/* Immediates encoding (must match your VM conventions)
   - int    : "#123"
   - double : "~3.14"
   - string : "$hello"
   - bool   : "@1" / "@0"
   - char   : "^a" (payload from lexer)
*/
static char* imm_int(const char* n) {
    size_t ln = strlen(n);
    char* r = (char*)malloc(ln + 2);
    if (!r) { perror("malloc"); exit(1); }
    r[0] = '#';
    memcpy(r + 1, n, ln + 1);
    return r;
}

static char* imm_dbl(const char* n) {
    size_t ln = strlen(n);
    char* r = (char*)malloc(ln + 2);
    if (!r) { perror("malloc"); exit(1); }
    r[0] = '~';
    memcpy(r + 1, n, ln + 1);
    return r;
}

static char* imm_str(const char* s) {
    size_t ln = strlen(s);
    char* r = (char*)malloc(ln + 2);
    if (!r) { perror("malloc"); exit(1); }
    r[0] = '$';
    memcpy(r + 1, s, ln + 1);
    return r;
}

/* Only these are allowed to return a value in expressions */
static int call_has_return(const char* name) {
    return (strcmp(name, "GetMouseX") == 0) || (strcmp(name, "GetMouseY") == 0);
}

/* ---------------- IF/ELSE codegen context stack ---------------- */

typedef struct {
    char* Lfalse;
    char* Lend;
    int has_else;
} ifctx_t;

static ifctx_t ifstk[256];
static int ifsp = 0;

static void if_push(char* cond) {
    if (ifsp >= 256) { fprintf(stderr, "parser error: if nesting too deep\n"); exit(1); }

    char* Lfalse = quad_new_label();
    char* Lend   = quad_new_label();

    /* if cond is false -> jump to Lfalse */
    quad_emit_jz(cond, Lfalse);

    /* store context */
    ifstk[ifsp].Lfalse = Lfalse;
    ifstk[ifsp].Lend   = Lend;
    ifstk[ifsp].has_else = 0;
    ifsp++;

    free(cond);
}

static void if_begin_else(void) {
    if (ifsp <= 0) { fprintf(stderr, "parser error: else without if\n"); exit(1); }
    ifctx_t* c = &ifstk[ifsp - 1];

    /* after then-block: jump over else, and mark else entry */
    quad_emit_jmp(c->Lend);
    quad_emit_label(c->Lfalse);
    c->has_else = 1;
}

static void if_end_no_else(void) {
    if (ifsp <= 0) { fprintf(stderr, "parser error: internal if stack underflow\n"); exit(1); }
    ifctx_t c = ifstk[--ifsp];

    /* no else: false label closes the if */
    quad_emit_label(c.Lfalse);

    free(c.Lfalse);
    free(c.Lend);
}

static void if_end_with_else(void) {
    if (ifsp <= 0) { fprintf(stderr, "parser error: internal if stack underflow\n"); exit(1); }
    ifctx_t c = ifstk[--ifsp];

    /* with else: end label closes the whole if/else */
    quad_emit_label(c.Lend);

    free(c.Lfalse);
    free(c.Lend);
}


#line 220 "src/parser.c"

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

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_STRING = 3,                     /* STRING  */
  YYSYMBOL_IDENT = 4,                      /* IDENT  */
  YYSYMBOL_INTNUM = 5,                     /* INTNUM  */
  YYSYMBOL_DBLNUM = 6,                     /* DBLNUM  */
  YYSYMBOL_BOOL_LIT = 7,                   /* BOOL_LIT  */
  YYSYMBOL_CHAR_LIT = 8,                   /* CHAR_LIT  */
  YYSYMBOL_PRINT = 9,                      /* PRINT  */
  YYSYMBOL_NEWLINE = 10,                   /* NEWLINE  */
  YYSYMBOL_START = 11,                     /* START  */
  YYSYMBOL_UPDATE = 12,                    /* UPDATE  */
  YYSYMBOL_LBRACE = 13,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 14,                    /* RBRACE  */
  YYSYMBOL_LBRACKET = 15,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 16,                  /* RBRACKET  */
  YYSYMBOL_ASSIGN = 17,                    /* ASSIGN  */
  YYSYMBOL_COMMA = 18,                     /* COMMA  */
  YYSYMBOL_SEMI = 19,                      /* SEMI  */
  YYSYMBOL_TYPE_INT = 20,                  /* TYPE_INT  */
  YYSYMBOL_TYPE_DOUBLE = 21,               /* TYPE_DOUBLE  */
  YYSYMBOL_TYPE_BOOL = 22,                 /* TYPE_BOOL  */
  YYSYMBOL_TYPE_CHAR = 23,                 /* TYPE_CHAR  */
  YYSYMBOL_TYPE_STRING = 24,               /* TYPE_STRING  */
  YYSYMBOL_PLUS = 25,                      /* PLUS  */
  YYSYMBOL_MINUS = 26,                     /* MINUS  */
  YYSYMBOL_STAR = 27,                      /* STAR  */
  YYSYMBOL_SLASH = 28,                     /* SLASH  */
  YYSYMBOL_PERCENT = 29,                   /* PERCENT  */
  YYSYMBOL_IDIV = 30,                      /* IDIV  */
  YYSYMBOL_LPAREN = 31,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 32,                    /* RPAREN  */
  YYSYMBOL_WHILE = 33,                     /* WHILE  */
  YYSYMBOL_FOR = 34,                       /* FOR  */
  YYSYMBOL_AND = 35,                       /* AND  */
  YYSYMBOL_OR = 36,                        /* OR  */
  YYSYMBOL_NOT = 37,                       /* NOT  */
  YYSYMBOL_EQ = 38,                        /* EQ  */
  YYSYMBOL_NE = 39,                        /* NE  */
  YYSYMBOL_LT = 40,                        /* LT  */
  YYSYMBOL_LE = 41,                        /* LE  */
  YYSYMBOL_GT = 42,                        /* GT  */
  YYSYMBOL_GE = 43,                        /* GE  */
  YYSYMBOL_IF = 44,                        /* IF  */
  YYSYMBOL_ELSE = 45,                      /* ELSE  */
  YYSYMBOL_UMINUS = 46,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 47,                  /* $accept  */
  YYSYMBOL_program = 48,                   /* program  */
  YYSYMBOL_49_1 = 49,                      /* $@1  */
  YYSYMBOL_50_2 = 50,                      /* $@2  */
  YYSYMBOL_preamble_opt = 51,              /* preamble_opt  */
  YYSYMBOL_preamble_line = 52,             /* preamble_line  */
  YYSYMBOL_declaration = 53,               /* declaration  */
  YYSYMBOL_type = 54,                      /* type  */
  YYSYMBOL_array_opt = 55,                 /* array_opt  */
  YYSYMBOL_literal = 56,                   /* literal  */
  YYSYMBOL_block_lines = 57,               /* block_lines  */
  YYSYMBOL_block_line = 58,                /* block_line  */
  YYSYMBOL_opt_newlines = 59,              /* opt_newlines  */
  YYSYMBOL_statement = 60,                 /* statement  */
  YYSYMBOL_print_arg = 61,                 /* print_arg  */
  YYSYMBOL_arglist_opt = 62,               /* arglist_opt  */
  YYSYMBOL_arglist = 63,                   /* arglist  */
  YYSYMBOL_if_stmt = 64,                   /* if_stmt  */
  YYSYMBOL_65_3 = 65,                      /* $@3  */
  YYSYMBOL_if_tail = 66,                   /* if_tail  */
  YYSYMBOL_67_4 = 67,                      /* $@4  */
  YYSYMBOL_expr = 68,                      /* expr  */
  YYSYMBOL_logic_or = 69,                  /* logic_or  */
  YYSYMBOL_logic_and = 70,                 /* logic_and  */
  YYSYMBOL_equality = 71,                  /* equality  */
  YYSYMBOL_rel = 72,                       /* rel  */
  YYSYMBOL_arith = 73,                     /* arith  */
  YYSYMBOL_term = 74,                      /* term  */
  YYSYMBOL_unary = 75,                     /* unary  */
  YYSYMBOL_primary = 76,                   /* primary  */
  YYSYMBOL_call_expr = 77,                 /* call_expr  */
  YYSYMBOL_while_stmt = 78,                /* while_stmt  */
  YYSYMBOL_79_5 = 79,                      /* $@5  */
  YYSYMBOL_80_6 = 80,                      /* $@6  */
  YYSYMBOL_opt_expr = 81,                  /* opt_expr  */
  YYSYMBOL_simple_stmt_opt = 82,           /* simple_stmt_opt  */
  YYSYMBOL_for_stmt = 83,                  /* for_stmt  */
  YYSYMBOL_84_7 = 84,                      /* $@7  */
  YYSYMBOL_85_8 = 85                       /* $@8  */
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

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   165

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  87
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  156

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   301


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
      45,    46
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   198,   198,   199,   197,   203,   204,   208,   209,   213,
     227,   246,   247,   248,   249,   250,   254,   255,   256,   271,
     272,   273,   274,   280,   293,   294,   302,   303,   304,   305,
     306,   310,   311,   315,   320,   326,   334,   335,   336,   340,
     341,   345,   351,   363,   362,   373,   378,   377,   392,   396,
     403,   407,   414,   418,   425,   432,   436,   443,   450,   457,
     464,   468,   475,   482,   486,   493,   500,   507,   514,   518,
     525,   532,   536,   537,   538,   539,   543,   559,   567,   558,
     583,   584,   588,   589,   596,   609,   623,   606
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "STRING", "IDENT",
  "INTNUM", "DBLNUM", "BOOL_LIT", "CHAR_LIT", "PRINT", "NEWLINE", "START",
  "UPDATE", "LBRACE", "RBRACE", "LBRACKET", "RBRACKET", "ASSIGN", "COMMA",
  "SEMI", "TYPE_INT", "TYPE_DOUBLE", "TYPE_BOOL", "TYPE_CHAR",
  "TYPE_STRING", "PLUS", "MINUS", "STAR", "SLASH", "PERCENT", "IDIV",
  "LPAREN", "RPAREN", "WHILE", "FOR", "AND", "OR", "NOT", "EQ", "NE", "LT",
  "LE", "GT", "GE", "IF", "ELSE", "UMINUS", "$accept", "program", "$@1",
  "$@2", "preamble_opt", "preamble_line", "declaration", "type",
  "array_opt", "literal", "block_lines", "block_line", "opt_newlines",
  "statement", "print_arg", "arglist_opt", "arglist", "if_stmt", "$@3",
  "if_tail", "$@4", "expr", "logic_or", "logic_and", "equality", "rel",
  "arith", "term", "unary", "primary", "call_expr", "while_stmt", "$@5",
  "$@6", "opt_expr", "simple_stmt_opt", "for_stmt", "$@7", "$@8", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-69)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-38)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -69,    34,   108,   -69,   -69,    28,   -69,   -69,   -69,   -69,
     -69,   -69,    36,    44,   -69,   -69,    43,   -69,     0,    47,
       3,    53,   -69,   130,   -13,    40,   -69,   -69,   -69,    48,
      57,   -69,    85,   -69,   -69,   -69,   -69,   -69,   -69,   -69,
     -69,   -69,   -69,    70,    70,    77,    10,    68,   101,    70,
     -69,    82,    70,    70,    70,   -69,   -69,    79,    81,   -14,
     -69,    84,   112,   -69,   -69,   -69,    90,    99,   -69,    25,
      91,   102,   -69,   -69,   134,    70,    23,   129,   117,    70,
     -69,   118,   -69,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,   -69,    70,   -69,
     -69,   119,    70,    70,   -69,   139,   121,   -69,    81,   -14,
     -69,   -69,   112,   112,    61,    61,    61,    61,   -69,   -69,
     -69,   -69,   -69,   -69,   -69,   -69,   122,   101,   -69,   -69,
       5,   142,   -69,   137,   -69,   -69,   -69,    70,    17,   147,
      19,   -69,   126,   114,   -69,   -69,   148,   -69,   149,   -69,
     -69,   -69,    56,    58,   -69,   -69
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       5,     0,     0,     1,     7,     0,    11,    12,    13,    14,
      15,     6,     0,     0,     2,     8,    16,    24,     0,     9,
       0,     0,    17,     0,     0,     0,    26,    31,    77,     0,
       0,    25,     0,    28,    29,    30,    18,    21,    19,    20,
      22,    23,    10,     0,    39,     0,     0,     0,    82,     0,
      27,    74,     0,     0,     0,    73,    34,    48,    50,    52,
      55,    60,    63,    68,    71,    75,     0,    40,    41,    74,
      73,     0,    38,    32,     0,     0,     0,     0,     0,    39,
      70,     0,    69,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    35,     0,    33,
       3,     0,     0,    39,    85,     0,     0,    72,    49,    51,
      53,    54,    61,    62,    56,    57,    58,    59,    64,    65,
      67,    66,    42,    24,    78,    83,     0,    82,    43,    76,
       0,     0,    84,     0,    24,    31,    24,    80,     0,     4,
       0,    81,     0,    45,    79,    86,     0,    44,     0,    46,
      24,    24,     0,     0,    87,    47
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -69,   -69,   -69,   -69,   -69,   -69,   -69,   -69,   -69,   -15,
     -30,   -69,    29,   -69,   -69,   -68,   -69,   -69,   -69,   -69,
     -69,   -43,   -69,    80,    76,    12,    54,    24,   -51,   -69,
     -69,   -69,   -69,   -69,   -69,    38,   -69,   -69,   -69
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,    17,   123,     2,    11,    12,    13,    19,    55,
      20,    31,    46,    32,    71,    66,    67,    33,   134,   147,
     151,    68,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    34,    47,   131,   142,    77,    35,   127,   148
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      56,    80,    72,    82,    43,    21,    78,    24,    42,    24,
      81,   106,    25,    26,    25,    26,    22,    27,    44,   135,
      73,    24,    74,    24,    85,    86,    25,    26,    25,    26,
      70,   143,   101,   144,     3,   126,    28,    29,    28,    29,
     102,    14,   118,   119,   120,   121,    15,    30,    16,    30,
      28,    29,    28,    29,   103,   122,    79,   -37,    18,   125,
      24,    30,    24,    30,    23,    25,    26,    25,    26,    36,
     154,    45,   155,    37,    51,    38,    39,    40,    41,    48,
      37,    69,    38,    39,    40,    41,    87,    88,    49,    28,
      29,    28,    29,   130,   141,    50,    52,   110,   111,    75,
      30,    53,    30,    52,   138,    76,   140,    54,    53,    87,
      88,   112,   113,    79,    54,    83,    84,    98,     4,     5,
     152,   153,    97,   -36,    89,    90,    91,    92,     6,     7,
       8,     9,    10,    37,    99,    38,    39,    40,    41,    93,
      94,    95,    96,   114,   115,   116,   117,   100,   104,   105,
     107,   124,   128,   129,   132,   136,   137,    73,   145,   146,
     109,   149,   150,   108,   139,   133
};

static const yytype_uint8 yycheck[] =
{
      43,    52,    45,    54,    17,     5,    49,     4,    23,     4,
      53,    79,     9,    10,     9,    10,    16,    14,    31,    14,
      10,     4,    12,     4,    38,    39,     9,    10,     9,    10,
      45,    14,    75,    14,     0,   103,    33,    34,    33,    34,
      17,    13,    93,    94,    95,    96,    10,    44,     4,    44,
      33,    34,    33,    34,    31,    98,    31,    32,    15,   102,
       4,    44,     4,    44,    17,     9,    10,     9,    10,    16,
      14,    31,    14,     3,     4,     5,     6,     7,     8,    31,
       3,     4,     5,     6,     7,     8,    25,    26,    31,    33,
      34,    33,    34,   123,   137,    10,    26,    85,    86,    31,
      44,    31,    44,    26,   134,     4,   136,    37,    31,    25,
      26,    87,    88,    31,    37,    36,    35,    18,    10,    11,
     150,   151,    32,    32,    40,    41,    42,    43,    20,    21,
      22,    23,    24,     3,    32,     5,     6,     7,     8,    27,
      28,    29,    30,    89,    90,    91,    92,    13,    19,    32,
      32,    32,    13,    32,    32,    13,    19,    10,    32,    45,
      84,    13,    13,    83,   135,   127
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    48,    51,     0,    10,    11,    20,    21,    22,    23,
      24,    52,    53,    54,    13,    10,     4,    49,    15,    55,
      57,     5,    16,    17,     4,     9,    10,    14,    33,    34,
      44,    58,    60,    64,    78,    83,    16,     3,     5,     6,
       7,     8,    56,    17,    31,    31,    59,    79,    31,    31,
      10,     4,    26,    31,    37,    56,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    62,    63,    68,     4,
      56,    61,    68,    10,    12,    31,     4,    82,    68,    31,
      75,    68,    75,    36,    35,    38,    39,    25,    26,    40,
      41,    42,    43,    27,    28,    29,    30,    32,    18,    32,
      13,    68,    17,    31,    19,    32,    62,    32,    70,    71,
      72,    72,    74,    74,    73,    73,    73,    73,    75,    75,
      75,    75,    68,    50,    32,    68,    62,    84,    13,    32,
      57,    80,    32,    82,    65,    14,    13,    19,    57,    59,
      57,    68,    81,    14,    14,    32,    45,    66,    85,    13,
      13,    67,    57,    57,    14,    14
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    47,    49,    50,    48,    51,    51,    52,    52,    53,
      53,    54,    54,    54,    54,    54,    55,    55,    55,    56,
      56,    56,    56,    56,    57,    57,    58,    58,    58,    58,
      58,    59,    59,    60,    60,    60,    61,    61,    61,    62,
      62,    63,    63,    65,    64,    66,    67,    66,    68,    69,
      69,    70,    70,    71,    71,    71,    72,    72,    72,    72,
      72,    73,    73,    73,    74,    74,    74,    74,    74,    75,
      75,    75,    76,    76,    76,    76,    77,    79,    80,    78,
      81,    81,    82,    82,    82,    84,    85,    83
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,    13,     0,     2,     1,     2,     3,
       5,     1,     1,     1,     1,     1,     0,     2,     3,     1,
       1,     1,     1,     1,     0,     2,     1,     2,     1,     1,
       1,     0,     2,     4,     3,     4,     1,     1,     1,     0,
       1,     1,     3,     0,     9,     0,     0,     5,     1,     3,
       1,     3,     1,     3,     3,     1,     3,     3,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     3,     1,     2,
       2,     1,     3,     1,     1,     1,     4,     0,     0,     9,
       0,     1,     0,     3,     4,     0,     0,    13
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
        yyerror (YY_("syntax error: cannot back up")); \
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
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
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
                 int yyrule)
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
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
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
yyparse (void)
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
  case 2: /* $@1: %empty  */
#line 198 "src/parser.y"
                    { quad_emit_label("START");  }
#line 1401 "src/parser.c"
    break;

  case 3: /* $@2: %empty  */
#line 199 "src/parser.y"
                    { quad_emit_label("UPDATE"); }
#line 1407 "src/parser.c"
    break;

  case 9: /* declaration: type IDENT array_opt  */
#line 214 "src/parser.y"
      {
          char* full = (char*)malloc(strlen((yyvsp[-2].s)) + strlen((yyvsp[0].s)) + 1);
          if (!full) { perror("malloc"); exit(1); }
          strcpy(full, (yyvsp[-2].s));
          strcat(full, (yyvsp[0].s));

          quad_emit_decl((yyvsp[-1].s), full);

          free(full);
          free((yyvsp[-2].s));
          free((yyvsp[-1].s));
          free((yyvsp[0].s));
      }
#line 1425 "src/parser.c"
    break;

  case 10: /* declaration: type IDENT array_opt ASSIGN literal  */
#line 228 "src/parser.y"
      {
          char* full = (char*)malloc(strlen((yyvsp[-4].s)) + strlen((yyvsp[-2].s)) + 1);
          if (!full) { perror("malloc"); exit(1); }
          strcpy(full, (yyvsp[-4].s));
          strcat(full, (yyvsp[-2].s));

          quad_emit_decl((yyvsp[-3].s), full);
          quad_emit_assign((yyvsp[-3].s), (yyvsp[0].s));

          free(full);
          free((yyvsp[-4].s));
          free((yyvsp[-3].s));
          free((yyvsp[-2].s));
          free((yyvsp[0].s));
      }
#line 1445 "src/parser.c"
    break;

  case 11: /* type: TYPE_INT  */
#line 246 "src/parser.y"
                  { (yyval.s) = xstrdup2("int"); }
#line 1451 "src/parser.c"
    break;

  case 12: /* type: TYPE_DOUBLE  */
#line 247 "src/parser.y"
                  { (yyval.s) = xstrdup2("double"); }
#line 1457 "src/parser.c"
    break;

  case 13: /* type: TYPE_BOOL  */
#line 248 "src/parser.y"
                  { (yyval.s) = xstrdup2("bool"); }
#line 1463 "src/parser.c"
    break;

  case 14: /* type: TYPE_CHAR  */
#line 249 "src/parser.y"
                  { (yyval.s) = xstrdup2("char"); }
#line 1469 "src/parser.c"
    break;

  case 15: /* type: TYPE_STRING  */
#line 250 "src/parser.y"
                  { (yyval.s) = xstrdup2("string"); }
#line 1475 "src/parser.c"
    break;

  case 16: /* array_opt: %empty  */
#line 254 "src/parser.y"
                         { (yyval.s) = xstrdup2(""); }
#line 1481 "src/parser.c"
    break;

  case 17: /* array_opt: LBRACKET RBRACKET  */
#line 255 "src/parser.y"
                         { (yyval.s) = xstrdup2("[]"); }
#line 1487 "src/parser.c"
    break;

  case 18: /* array_opt: LBRACKET INTNUM RBRACKET  */
#line 257 "src/parser.y"
      {
          size_t ln = strlen((yyvsp[-1].s));
          char* r = (char*)malloc(ln + 3);
          if (!r) { perror("malloc"); exit(1); }
          r[0] = '[';
          memcpy(r + 1, (yyvsp[-1].s), ln);
          r[ln + 1] = ']';
          r[ln + 2] = '\0';
          (yyval.s) = r;
          free((yyvsp[-1].s));
      }
#line 1503 "src/parser.c"
    break;

  case 19: /* literal: INTNUM  */
#line 271 "src/parser.y"
                { (yyval.s) = imm_int((yyvsp[0].s)); free((yyvsp[0].s)); }
#line 1509 "src/parser.c"
    break;

  case 20: /* literal: DBLNUM  */
#line 272 "src/parser.y"
                { (yyval.s) = imm_dbl((yyvsp[0].s)); free((yyvsp[0].s)); }
#line 1515 "src/parser.c"
    break;

  case 21: /* literal: STRING  */
#line 273 "src/parser.y"
                { (yyval.s) = imm_str((yyvsp[0].s)); free((yyvsp[0].s)); }
#line 1521 "src/parser.c"
    break;

  case 22: /* literal: BOOL_LIT  */
#line 275 "src/parser.y"
      {
          if (strcmp((yyvsp[0].s), "true") == 0) (yyval.s) = xstrdup2("@1");
          else (yyval.s) = xstrdup2("@0");
          free((yyvsp[0].s));
      }
#line 1531 "src/parser.c"
    break;

  case 23: /* literal: CHAR_LIT  */
#line 281 "src/parser.y"
      {
          size_t ln = strlen((yyvsp[0].s));
          char* r = (char*)malloc(ln + 2);
          if (!r) { perror("malloc"); exit(1); }
          r[0] = '^';
          memcpy(r + 1, (yyvsp[0].s), ln + 1);
          (yyval.s) = r;
          free((yyvsp[0].s));
      }
#line 1545 "src/parser.c"
    break;

  case 33: /* statement: PRINT LPAREN print_arg RPAREN  */
#line 316 "src/parser.y"
      {
          quad_emit_print((yyvsp[-1].s));
          free((yyvsp[-1].s));
      }
#line 1554 "src/parser.c"
    break;

  case 34: /* statement: IDENT ASSIGN expr  */
#line 321 "src/parser.y"
      {
          quad_emit_assign((yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s));
          free((yyvsp[0].s));
      }
#line 1564 "src/parser.c"
    break;

  case 35: /* statement: IDENT LPAREN arglist_opt RPAREN  */
#line 327 "src/parser.y"
      {
          quad_emit_calln((yyvsp[-3].s), (yyvsp[-1].i), NULL);
          free((yyvsp[-3].s));
      }
#line 1573 "src/parser.c"
    break;

  case 36: /* print_arg: literal  */
#line 334 "src/parser.y"
              { (yyval.s) = (yyvsp[0].s); }
#line 1579 "src/parser.c"
    break;

  case 37: /* print_arg: IDENT  */
#line 335 "src/parser.y"
              { (yyval.s) = (yyvsp[0].s); }
#line 1585 "src/parser.c"
    break;

  case 38: /* print_arg: expr  */
#line 336 "src/parser.y"
              { (yyval.s) = (yyvsp[0].s); }
#line 1591 "src/parser.c"
    break;

  case 39: /* arglist_opt: %empty  */
#line 340 "src/parser.y"
                  { (yyval.i) = 0; }
#line 1597 "src/parser.c"
    break;

  case 40: /* arglist_opt: arglist  */
#line 341 "src/parser.y"
                  { (yyval.i) = (yyvsp[0].i); }
#line 1603 "src/parser.c"
    break;

  case 41: /* arglist: expr  */
#line 346 "src/parser.y"
      {
          quad_emit_param((yyvsp[0].s));
          free((yyvsp[0].s));
          (yyval.i) = 1;
      }
#line 1613 "src/parser.c"
    break;

  case 42: /* arglist: arglist COMMA expr  */
#line 352 "src/parser.y"
      {
          quad_emit_param((yyvsp[0].s));
          free((yyvsp[0].s));
          (yyval.i) = (yyvsp[-2].i) + 1;
      }
#line 1623 "src/parser.c"
    break;

  case 43: /* $@3: %empty  */
#line 363 "src/parser.y"
      {
          /* emits: JZ cond, Lfalse  and pushes (Lfalse, Lend) */
          if_push((yyvsp[-2].s));
      }
#line 1632 "src/parser.c"
    break;

  case 45: /* if_tail: %empty  */
#line 373 "src/parser.y"
      {
          /* no else: place Lfalse and pop */
          if_end_no_else();
      }
#line 1641 "src/parser.c"
    break;

  case 46: /* $@4: %empty  */
#line 378 "src/parser.y"
      {
          /* after then-block: emit JMP Lend; LABEL Lfalse */
          if_begin_else();
      }
#line 1650 "src/parser.c"
    break;

  case 47: /* if_tail: ELSE LBRACE $@4 block_lines RBRACE  */
#line 383 "src/parser.y"
      {
          /* end else: LABEL Lend and pop */
          if_end_with_else();
      }
#line 1659 "src/parser.c"
    break;

  case 48: /* expr: logic_or  */
#line 392 "src/parser.y"
               { (yyval.s) = (yyvsp[0].s); }
#line 1665 "src/parser.c"
    break;

  case 49: /* logic_or: logic_or OR logic_and  */
#line 397 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("OR", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1676 "src/parser.c"
    break;

  case 50: /* logic_or: logic_and  */
#line 403 "src/parser.y"
                { (yyval.s) = (yyvsp[0].s); }
#line 1682 "src/parser.c"
    break;

  case 51: /* logic_and: logic_and AND equality  */
#line 408 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("AND", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1693 "src/parser.c"
    break;

  case 52: /* logic_and: equality  */
#line 414 "src/parser.y"
               { (yyval.s) = (yyvsp[0].s); }
#line 1699 "src/parser.c"
    break;

  case 53: /* equality: equality EQ rel  */
#line 419 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("EQ", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1710 "src/parser.c"
    break;

  case 54: /* equality: equality NE rel  */
#line 426 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("NE", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1721 "src/parser.c"
    break;

  case 55: /* equality: rel  */
#line 432 "src/parser.y"
          { (yyval.s) = (yyvsp[0].s); }
#line 1727 "src/parser.c"
    break;

  case 56: /* rel: arith LT arith  */
#line 437 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("LT", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1738 "src/parser.c"
    break;

  case 57: /* rel: arith LE arith  */
#line 444 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("LE", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1749 "src/parser.c"
    break;

  case 58: /* rel: arith GT arith  */
#line 451 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("GT", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1760 "src/parser.c"
    break;

  case 59: /* rel: arith GE arith  */
#line 458 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("GE", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1771 "src/parser.c"
    break;

  case 60: /* rel: arith  */
#line 464 "src/parser.y"
            { (yyval.s) = (yyvsp[0].s); }
#line 1777 "src/parser.c"
    break;

  case 61: /* arith: arith PLUS term  */
#line 469 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("ADD", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1788 "src/parser.c"
    break;

  case 62: /* arith: arith MINUS term  */
#line 476 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("SUB", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1799 "src/parser.c"
    break;

  case 63: /* arith: term  */
#line 482 "src/parser.y"
           { (yyval.s) = (yyvsp[0].s); }
#line 1805 "src/parser.c"
    break;

  case 64: /* term: term STAR unary  */
#line 487 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("MUL", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1816 "src/parser.c"
    break;

  case 65: /* term: term SLASH unary  */
#line 494 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("DIV", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1827 "src/parser.c"
    break;

  case 66: /* term: term IDIV unary  */
#line 501 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("IDIV", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1838 "src/parser.c"
    break;

  case 67: /* term: term PERCENT unary  */
#line 508 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_binop("MOD", t, (yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s)); free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1849 "src/parser.c"
    break;

  case 68: /* term: unary  */
#line 514 "src/parser.y"
            { (yyval.s) = (yyvsp[0].s); }
#line 1855 "src/parser.c"
    break;

  case 69: /* unary: NOT unary  */
#line 519 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_unop("NOT", t, (yyvsp[0].s));
          free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1866 "src/parser.c"
    break;

  case 70: /* unary: MINUS unary  */
#line 526 "src/parser.y"
      {
          char* t = quad_new_temp();
          quad_emit_unop("NEG", t, (yyvsp[0].s));
          free((yyvsp[0].s));
          (yyval.s) = t;
      }
#line 1877 "src/parser.c"
    break;

  case 71: /* unary: primary  */
#line 532 "src/parser.y"
              { (yyval.s) = (yyvsp[0].s); }
#line 1883 "src/parser.c"
    break;

  case 72: /* primary: LPAREN expr RPAREN  */
#line 536 "src/parser.y"
                         { (yyval.s) = (yyvsp[-1].s); }
#line 1889 "src/parser.c"
    break;

  case 73: /* primary: literal  */
#line 537 "src/parser.y"
                         { (yyval.s) = (yyvsp[0].s); }
#line 1895 "src/parser.c"
    break;

  case 74: /* primary: IDENT  */
#line 538 "src/parser.y"
                         { (yyval.s) = (yyvsp[0].s); }
#line 1901 "src/parser.c"
    break;

  case 75: /* primary: call_expr  */
#line 539 "src/parser.y"
                         { (yyval.s) = (yyvsp[0].s); }
#line 1907 "src/parser.c"
    break;

  case 76: /* call_expr: IDENT LPAREN arglist_opt RPAREN  */
#line 544 "src/parser.y"
      {
          if (!call_has_return((yyvsp[-3].s))) {
              fprintf(stderr, "File \"%s\", line %d, character %d: '%s' has no return value\n",
                      g_filename, yylineno, yycolumn, (yyvsp[-3].s));
              exit(1);
          }
          char* t = quad_new_temp();
          quad_emit_calln((yyvsp[-3].s), (yyvsp[-1].i), t);
          free((yyvsp[-3].s));
          (yyval.s) = t;
      }
#line 1923 "src/parser.c"
    break;

  case 77: /* $@5: %empty  */
#line 559 "src/parser.y"
      {
          char* Lbegin = quad_new_label();
          char* Lend   = quad_new_label();
          quad_emit_label(Lbegin);
          /* while: step=cond=begin */
          loop_push3(Lbegin, xstrdup2(Lbegin), Lend);
      }
#line 1935 "src/parser.c"
    break;

  case 78: /* $@6: %empty  */
#line 567 "src/parser.y"
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_jz((yyvsp[-1].s), c->Lend);
          free((yyvsp[-1].s));
      }
#line 1945 "src/parser.c"
    break;

  case 79: /* while_stmt: WHILE $@5 LPAREN expr RPAREN $@6 LBRACE block_lines RBRACE  */
#line 573 "src/parser.y"
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_jmp(c->Lcond);   /* back to condition label (same as begin) */
          quad_emit_label(c->Lend);
          loop_pop3();
      }
#line 1956 "src/parser.c"
    break;

  case 80: /* opt_expr: %empty  */
#line 583 "src/parser.y"
                  { (yyval.s) = xstrdup2("@1"); }
#line 1962 "src/parser.c"
    break;

  case 81: /* opt_expr: expr  */
#line 584 "src/parser.y"
                  { (yyval.s) = (yyvsp[0].s); }
#line 1968 "src/parser.c"
    break;

  case 82: /* simple_stmt_opt: %empty  */
#line 588 "src/parser.y"
                  { (yyval.i) = 0; }
#line 1974 "src/parser.c"
    break;

  case 83: /* simple_stmt_opt: IDENT ASSIGN expr  */
#line 590 "src/parser.y"
      {
          quad_emit_assign((yyvsp[-2].s), (yyvsp[0].s));
          free((yyvsp[-2].s));
          free((yyvsp[0].s));
          (yyval.i) = 1;
      }
#line 1985 "src/parser.c"
    break;

  case 84: /* simple_stmt_opt: IDENT LPAREN arglist_opt RPAREN  */
#line 597 "src/parser.y"
      {
          quad_emit_calln((yyvsp[-3].s), (yyvsp[-1].i), NULL);
          free((yyvsp[-3].s));
          (yyval.i) = 1;
      }
#line 1995 "src/parser.c"
    break;

  case 85: /* $@7: %empty  */
#line 609 "src/parser.y"
      {
          char* Lstep = quad_new_label();
          char* Lcond = quad_new_label();
          char* Lend  = quad_new_label();

          quad_emit_jmp(Lcond);     /* first entry skips step */
          quad_emit_label(Lstep);   /* step code will be emitted here */

          loop_push3(Lstep, Lcond, Lend);
      }
#line 2010 "src/parser.c"
    break;

  case 86: /* $@8: %empty  */
#line 623 "src/parser.y"
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_label(c->Lcond);     /* condition label */
          quad_emit_jz((yyvsp[-1].s), c->Lend);     /* if false => exit */
          free((yyvsp[-1].s));
      }
#line 2021 "src/parser.c"
    break;

  case 87: /* for_stmt: FOR LPAREN simple_stmt_opt SEMI $@7 simple_stmt_opt SEMI opt_expr RPAREN $@8 LBRACE block_lines RBRACE  */
#line 630 "src/parser.y"
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_jmp(c->Lstep);       /* do step then re-check condition */
          quad_emit_label(c->Lend);
          loop_pop3();
      }
#line 2032 "src/parser.c"
    break;


#line 2036 "src/parser.c"

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
      yyerror (YY_("syntax error"));
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
                      yytoken, &yylval);
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
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
  yyerror (YY_("memory exhausted"));
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
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 640 "src/parser.y"


void yyerror(const char* s) {
    (void)s;
    fprintf(stderr, "File \"%s\", line %d, character %d: syntax error\n",
            g_filename, yylineno, yycolumn);
}
