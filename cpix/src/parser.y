%{
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

%}

%union { char* s; int i; }

%token <s> STRING
%token <s> IDENT
%token <s> INTNUM
%token <s> DBLNUM
%token <s> BOOL_LIT
%token <s> CHAR_LIT

%token PRINT
%token NEWLINE
%token START UPDATE
%token LBRACE RBRACE
%token LBRACKET RBRACKET
%token ASSIGN
%token COMMA
%token SEMI
%token TYPE_INT TYPE_DOUBLE TYPE_BOOL TYPE_CHAR TYPE_STRING
%token PLUS MINUS STAR SLASH PERCENT IDIV
%token LPAREN RPAREN
%token WHILE FOR

/* logic + comparisons */
%token AND OR NOT
%token EQ NE LT LE GT GE

/* if/else */
%token IF ELSE

%type <s> type print_arg literal array_opt
%type <s> expr logic_or logic_and equality rel arith term unary primary call_expr
%type <i> arglist arglist_opt
%type <s> opt_expr
%type <i> simple_stmt_opt

/* precedence (lowest->highest) */
%left OR
%left AND
%nonassoc EQ NE LT LE GT GE
%left PLUS MINUS
%left STAR SLASH PERCENT IDIV
%right NOT
%right UMINUS

%%

program
    : preamble_opt
      START  LBRACE { quad_emit_label("START");  } block_lines RBRACE opt_newlines
      UPDATE LBRACE { quad_emit_label("UPDATE"); } block_lines RBRACE opt_newlines
    ;

preamble_opt
    : /* empty */
    | preamble_opt preamble_line
    ;

preamble_line
    : NEWLINE
    | declaration NEWLINE
    ;

declaration
    : type IDENT array_opt
      {
          char* full = (char*)malloc(strlen($1) + strlen($3) + 1);
          if (!full) { perror("malloc"); exit(1); }
          strcpy(full, $1);
          strcat(full, $3);

          quad_emit_decl($2, full);

          free(full);
          free($1);
          free($2);
          free($3);
      }
    | type IDENT array_opt ASSIGN literal
      {
          char* full = (char*)malloc(strlen($1) + strlen($3) + 1);
          if (!full) { perror("malloc"); exit(1); }
          strcpy(full, $1);
          strcat(full, $3);

          quad_emit_decl($2, full);
          quad_emit_assign($2, $5);

          free(full);
          free($1);
          free($2);
          free($3);
          free($5);
      }
    ;

type
    : TYPE_INT    { $$ = xstrdup2("int"); }
    | TYPE_DOUBLE { $$ = xstrdup2("double"); }
    | TYPE_BOOL   { $$ = xstrdup2("bool"); }
    | TYPE_CHAR   { $$ = xstrdup2("char"); }
    | TYPE_STRING { $$ = xstrdup2("string"); }
    ;

array_opt
    : /* empty */        { $$ = xstrdup2(""); }
    | LBRACKET RBRACKET  { $$ = xstrdup2("[]"); }
    | LBRACKET INTNUM RBRACKET
      {
          size_t ln = strlen($2);
          char* r = (char*)malloc(ln + 3);
          if (!r) { perror("malloc"); exit(1); }
          r[0] = '[';
          memcpy(r + 1, $2, ln);
          r[ln + 1] = ']';
          r[ln + 2] = '\0';
          $$ = r;
          free($2);
      }
    ;

literal
    : INTNUM    { $$ = imm_int($1); free($1); }
    | DBLNUM    { $$ = imm_dbl($1); free($1); }
    | STRING    { $$ = imm_str($1); free($1); }
    | BOOL_LIT
      {
          if (strcmp($1, "true") == 0) $$ = xstrdup2("@1");
          else $$ = xstrdup2("@0");
          free($1);
      }
    | CHAR_LIT
      {
          size_t ln = strlen($1);
          char* r = (char*)malloc(ln + 2);
          if (!r) { perror("malloc"); exit(1); }
          r[0] = '^';
          memcpy(r + 1, $1, ln + 1);
          $$ = r;
          free($1);
      }
    ;

block_lines
    : /* empty */
    | block_lines block_line
    ;

/* IMPORTANT:
   - keep if_stmt as its own block_line alternative (no trailing NEWLINE required)
   - other statements stay newline-terminated
*/
block_line
    : NEWLINE
    | statement NEWLINE
    | if_stmt
    | while_stmt
    | for_stmt
    ;

opt_newlines
    : /* empty */
    | opt_newlines NEWLINE
    ;

statement
    : PRINT LPAREN print_arg RPAREN
      {
          quad_emit_print($3);
          free($3);
      }
    | IDENT ASSIGN expr
      {
          quad_emit_assign($1, $3);
          free($1);
          free($3);
      }
    | IDENT LPAREN arglist_opt RPAREN
      {
          quad_emit_calln($1, $3, NULL);
          free($1);
      }
    ;

print_arg
    : literal { $$ = $1; }
    | IDENT   { $$ = $1; }
    | expr    { $$ = $1; }
    ;

arglist_opt
    : /* empty */ { $$ = 0; }
    | arglist     { $$ = $1; }
    ;

arglist
    : expr
      {
          quad_emit_param($1);
          free($1);
          $$ = 1;
      }
    | arglist COMMA expr
      {
          quad_emit_param($3);
          free($3);
          $$ = $1 + 1;
      }
    ;

/* ---------------- IF / ELSE ---------------- */

if_stmt
    : IF LPAREN expr RPAREN LBRACE
      {
          /* emits: JZ cond, Lfalse  and pushes (Lfalse, Lend) */
          if_push($3);
      }
      block_lines RBRACE
      if_tail
    ;

if_tail
    : /* empty */
      {
          /* no else: place Lfalse and pop */
          if_end_no_else();
      }
    | ELSE LBRACE
      {
          /* after then-block: emit JMP Lend; LABEL Lfalse */
          if_begin_else();
      }
      block_lines RBRACE
      {
          /* end else: LABEL Lend and pop */
          if_end_with_else();
      }
    ;

/* ---------------- Layered expressions ---------------- */

expr
    : logic_or { $$ = $1; }
    ;

logic_or
    : logic_or OR logic_and
      {
          char* t = quad_new_temp();
          quad_emit_binop("OR", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | logic_and { $$ = $1; }
    ;

logic_and
    : logic_and AND equality
      {
          char* t = quad_new_temp();
          quad_emit_binop("AND", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | equality { $$ = $1; }
    ;

equality
    : equality EQ rel
      {
          char* t = quad_new_temp();
          quad_emit_binop("EQ", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | equality NE rel
      {
          char* t = quad_new_temp();
          quad_emit_binop("NE", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | rel { $$ = $1; }
    ;

rel
    : arith LT arith
      {
          char* t = quad_new_temp();
          quad_emit_binop("LT", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | arith LE arith
      {
          char* t = quad_new_temp();
          quad_emit_binop("LE", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | arith GT arith
      {
          char* t = quad_new_temp();
          quad_emit_binop("GT", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | arith GE arith
      {
          char* t = quad_new_temp();
          quad_emit_binop("GE", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | arith { $$ = $1; }
    ;

arith
    : arith PLUS term
      {
          char* t = quad_new_temp();
          quad_emit_binop("ADD", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | arith MINUS term
      {
          char* t = quad_new_temp();
          quad_emit_binop("SUB", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | term { $$ = $1; }
    ;

term
    : term STAR unary
      {
          char* t = quad_new_temp();
          quad_emit_binop("MUL", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | term SLASH unary
      {
          char* t = quad_new_temp();
          quad_emit_binop("DIV", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | term IDIV unary
      {
          char* t = quad_new_temp();
          quad_emit_binop("IDIV", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | term PERCENT unary
      {
          char* t = quad_new_temp();
          quad_emit_binop("MOD", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | unary { $$ = $1; }
    ;

unary
    : NOT unary
      {
          char* t = quad_new_temp();
          quad_emit_unop("NOT", t, $2);
          free($2);
          $$ = t;
      }
    | MINUS unary %prec UMINUS
      {
          char* t = quad_new_temp();
          quad_emit_unop("NEG", t, $2);
          free($2);
          $$ = t;
      }
    | primary { $$ = $1; }
    ;

primary
    : LPAREN expr RPAREN { $$ = $2; }
    | literal            { $$ = $1; }
    | IDENT              { $$ = $1; }
    | call_expr          { $$ = $1; }
    ;

call_expr
    : IDENT LPAREN arglist_opt RPAREN
      {
          if (!call_has_return($1)) {
              fprintf(stderr, "File \"%s\", line %d, character %d: '%s' has no return value\n",
                      g_filename, yylineno, yycolumn, $1);
              exit(1);
          }
          char* t = quad_new_temp();
          quad_emit_calln($1, $3, t);
          free($1);
          $$ = t;
      }
    ;

while_stmt
    : WHILE
      {
          char* Lbegin = quad_new_label();
          char* Lend   = quad_new_label();
          quad_emit_label(Lbegin);
          /* while: step=cond=begin */
          loop_push3(Lbegin, xstrdup2(Lbegin), Lend);
      }
      LPAREN expr RPAREN
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_jz($4, c->Lend);
          free($4);
      }
      LBRACE block_lines RBRACE
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_jmp(c->Lcond);   /* back to condition label (same as begin) */
          quad_emit_label(c->Lend);
          loop_pop3();
      }
    ;


opt_expr
    : /* empty */ { $$ = xstrdup2("@1"); } /* default TRUE */
    | expr        { $$ = $1; }
    ;
/* emits quads like statement, but no NEWLINE required */
simple_stmt_opt
    : /* empty */ { $$ = 0; }
    | IDENT ASSIGN expr
      {
          quad_emit_assign($1, $3);
          free($1);
          free($3);
          $$ = 1;
      }
    | IDENT LPAREN arglist_opt RPAREN
      {
          quad_emit_calln($1, $3, NULL);
          free($1);
          $$ = 1;
      }
    ;


for_stmt
    : FOR LPAREN
        simple_stmt_opt
        SEMI
      {
          char* Lstep = quad_new_label();
          char* Lcond = quad_new_label();
          char* Lend  = quad_new_label();

          quad_emit_jmp(Lcond);     /* first entry skips step */
          quad_emit_label(Lstep);   /* step code will be emitted here */

          loop_push3(Lstep, Lcond, Lend);
      }
        simple_stmt_opt
        SEMI
        opt_expr
      RPAREN
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_label(c->Lcond);     /* condition label */
          quad_emit_jz($8, c->Lend);     /* if false => exit */
          free($8);
      }
      LBRACE block_lines RBRACE
      {
          loopctx_t* c = &loopstk[loopsp - 1];
          quad_emit_jmp(c->Lstep);       /* do step then re-check condition */
          quad_emit_label(c->Lend);
          loop_pop3();
      }
    ;



%%

void yyerror(const char* s) {
    (void)s;
    fprintf(stderr, "File \"%s\", line %d, character %d: syntax error\n",
            g_filename, yylineno, yycolumn);
}
