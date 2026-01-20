%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"

extern int yylex(void);
extern int yyparse(void);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;
extern const char* g_filename;

void yyerror(const char* s);

static char* xstrdup2(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n);
    return p;
}

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
%}

%union { char* s; }

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

%token TYPE_INT TYPE_DOUBLE TYPE_BOOL TYPE_CHAR TYPE_STRING

%token PLUS MINUS STAR SLASH PERCENT IDIV
%token LPAREN RPAREN

%type <s> type expr print_arg literal
%type <s> array_opt


%left PLUS MINUS
%left STAR SLASH PERCENT IDIV
%right UMINUS

%%

program
    : preamble_opt START LBRACE block_lines RBRACE opt_newlines
      UPDATE LBRACE block_lines RBRACE opt_newlines
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
    : INTNUM
      { $$ = imm_int($1); free($1); }
    | DBLNUM
      { $$ = imm_dbl($1); free($1); }
    | STRING
      { $$ = imm_str($1); free($1); }
    | BOOL_LIT
      {
          if (strcmp($1, "true") == 0) $$ = xstrdup2("@1");
          else $$ = xstrdup2("@0");
          free($1);
      }
    | CHAR_LIT
      {
          /* CHAR_LIT is stored without quotes by lexer, like A or \n */
          /* encode as ^<payload> so VM knows it’s a char immediate */
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

block_line
    : NEWLINE
    | statement NEWLINE
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
    ;

print_arg
    : STRING
      { $$ = imm_str($1); free($1); }
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
    | IDENT
      { $$ = $1; }
    | expr
      { $$ = $1; }
    ;


expr
    : INTNUM
      { $$ = imm_int($1); free($1); }
    | IDENT
      { $$ = $1; }
    | LPAREN expr RPAREN
      { $$ = $2; }
    | MINUS expr %prec UMINUS
      {
          char* t = quad_new_temp();
          quad_emit_unop("NEG", t, $2);
          free($2);
          $$ = t;
      }
    | expr PLUS expr
      {
          char* t = quad_new_temp();
          quad_emit_binop("ADD", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | expr MINUS expr
      {
          char* t = quad_new_temp();
          quad_emit_binop("SUB", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | expr STAR expr
      {
          char* t = quad_new_temp();
          quad_emit_binop("MUL", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | expr SLASH expr
      {
          char* t = quad_new_temp();
          quad_emit_binop("DIV", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | expr IDIV expr
      {
          char* t = quad_new_temp();
          quad_emit_binop("IDIV", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    | expr PERCENT expr
      {
          char* t = quad_new_temp();
          quad_emit_binop("MOD", t, $1, $3);
          free($1); free($3);
          $$ = t;
      }
    ;

%%

void yyerror(const char* s) {
    (void)s;
    fprintf(stderr, "File \"%s\", line %d, character %d: syntax error\n",
            g_filename, yylineno, yycolumn);
}
