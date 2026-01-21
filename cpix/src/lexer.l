%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

extern int yycolumn;
extern const char* g_filename;

int yycolumn = 1;

static char* xstrdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n);
    return p;
}
%}

%option noyywrap
%option yylineno

%x PREAMBLE CODE

%{
#define YY_USER_INIT do { yycolumn = 1; BEGIN(PREAMBLE); } while(0)
%}

%%

<PREAMBLE>\xEF\xBB\xBF { yycolumn += yyleng; }

<PREAMBLE>Start   { yycolumn += yyleng; BEGIN(CODE); return START; }
<PREAMBLE>Update  { yycolumn += yyleng; BEGIN(CODE); return UPDATE; }


<PREAMBLE>print   { yycolumn += yyleng; return PRINT; }

<PREAMBLE>int     { yycolumn += yyleng; return TYPE_INT; }
<PREAMBLE>double  { yycolumn += yyleng; return TYPE_DOUBLE; }
<PREAMBLE>bool    { yycolumn += yyleng; return TYPE_BOOL; }
<PREAMBLE>char    { yycolumn += yyleng; return TYPE_CHAR; }
<PREAMBLE>string  { yycolumn += yyleng; return TYPE_STRING; }

<PREAMBLE>true|false { yycolumn += yyleng; yylval.s = xstrdup(yytext); return BOOL_LIT; }

<PREAMBLE>\"[^\"]*\" {
    int len = yyleng;
    yycolumn += len;
    yylval.s = (char*)malloc((size_t)len - 1);
    if (!yylval.s) { perror("malloc"); exit(1); }
    strncpy(yylval.s, yytext + 1, (size_t)len - 2);
    yylval.s[len - 2] = '\0';
    return STRING;
}

<PREAMBLE>\'([^\'\\]|\\.)\' {
    int len = yyleng;
    yycolumn += len;
    yylval.s = (char*)malloc((size_t)len - 1);
    if (!yylval.s) { perror("malloc"); exit(1); }
    strncpy(yylval.s, yytext + 1, (size_t)len - 2);
    yylval.s[len - 2] = '\0';
    return CHAR_LIT;
}

<PREAMBLE>0[xX][0-9A-Fa-f]+ { yycolumn += yyleng; yylval.s = xstrdup(yytext); return INTNUM; }
<CODE>0[xX][0-9A-Fa-f]+     { yycolumn += yyleng; yylval.s = xstrdup(yytext); return INTNUM; }

<PREAMBLE>[0-9]+\.[0-9]+ { yycolumn += yyleng; yylval.s = xstrdup(yytext); return DBLNUM; }
<PREAMBLE>[0-9]+         { yycolumn += yyleng; yylval.s = xstrdup(yytext); return INTNUM; }

<PREAMBLE>"//" { yycolumn += yyleng; return IDIV; }
<PREAMBLE>"+"  { yycolumn += yyleng; return PLUS; }
<PREAMBLE>"-"  { yycolumn += yyleng; return MINUS; }
<PREAMBLE>"*"  { yycolumn += yyleng; return STAR; }
<PREAMBLE>"/"  { yycolumn += yyleng; return SLASH; }
<PREAMBLE>"%"  { yycolumn += yyleng; return PERCENT; }
<PREAMBLE>"="  { yycolumn += yyleng; return ASSIGN; }
<PREAMBLE>"," { yycolumn += yyleng; return COMMA; }

<PREAMBLE>\( { yycolumn += yyleng; return LPAREN; }
<PREAMBLE>\) { yycolumn += yyleng; return RPAREN; }

<PREAMBLE>\{ { yycolumn += yyleng; return LBRACE; }
<PREAMBLE>\} { yycolumn += yyleng; return RBRACE; }
<PREAMBLE>\[ { yycolumn += yyleng; return LBRACKET; }
<PREAMBLE>\] { yycolumn += yyleng; return RBRACKET; }

<PREAMBLE>[ \t\r]+ { yycolumn += yyleng; }
<PREAMBLE>\r\n { yycolumn = 1; return NEWLINE; }
<PREAMBLE>\n   { yycolumn = 1; return NEWLINE; }

<PREAMBLE>[A-Za-z_][A-Za-z0-9_]* { yycolumn += yyleng; yylval.s = xstrdup(yytext); return IDENT; }

<PREAMBLE>; {
    fprintf(stderr, "File \"%s\", line %d, character %d: lexical error\n",
            g_filename, yylineno, yycolumn);
    exit(1);
}

<PREAMBLE>. {
    fprintf(stderr, "File \"%s\", line %d, character %d: lexical error\n",
            g_filename, yylineno, yycolumn);
    exit(1);
}





<CODE>","     { yycolumn += yyleng; return COMMA; }


<CODE>Start   { yycolumn += yyleng; return START; }
<CODE>Update  { yycolumn += yyleng; return UPDATE; }

<CODE>print   { yycolumn += yyleng; return PRINT; }

<CODE>int     { yycolumn += yyleng; return TYPE_INT; }
<CODE>double  { yycolumn += yyleng; return TYPE_DOUBLE; }
<CODE>bool    { yycolumn += yyleng; return TYPE_BOOL; }
<CODE>char    { yycolumn += yyleng; return TYPE_CHAR; }
<CODE>string  { yycolumn += yyleng; return TYPE_STRING; }

<CODE>true|false { yycolumn += yyleng; yylval.s = xstrdup(yytext); return BOOL_LIT; }

<CODE>\"[^\"]*\" {
    int len = yyleng;
    yycolumn += len;
    yylval.s = (char*)malloc((size_t)len - 1);
    if (!yylval.s) { perror("malloc"); exit(1); }
    strncpy(yylval.s, yytext + 1, (size_t)len - 2);
    yylval.s[len - 2] = '\0';
    return STRING;
}

<CODE>\'([^\'\\]|\\.)\' {
    int len = yyleng;
    yycolumn += len;
    yylval.s = (char*)malloc((size_t)len - 1);
    if (!yylval.s) { perror("malloc"); exit(1); }
    strncpy(yylval.s, yytext + 1, (size_t)len - 2);
    yylval.s[len - 2] = '\0';
    return CHAR_LIT;
}

<CODE>[0-9]+\.[0-9]+ { yycolumn += yyleng; yylval.s = xstrdup(yytext); return DBLNUM; }
<CODE>[0-9]+         { yycolumn += yyleng; yylval.s = xstrdup(yytext); return INTNUM; }

<CODE>"//" { yycolumn += yyleng; return IDIV; }
<CODE>"+"  { yycolumn += yyleng; return PLUS; }
<CODE>"-"  { yycolumn += yyleng; return MINUS; }
<CODE>"*"  { yycolumn += yyleng; return STAR; }
<CODE>"/"  { yycolumn += yyleng; return SLASH; }
<CODE>"%"  { yycolumn += yyleng; return PERCENT; }
<CODE>"="  { yycolumn += yyleng; return ASSIGN; }

<CODE>\( { yycolumn += yyleng; return LPAREN; }
<CODE>\) { yycolumn += yyleng; return RPAREN; }

<CODE>\{ { yycolumn += yyleng; return LBRACE; }
<CODE>\} { yycolumn += yyleng; return RBRACE; }
<CODE>\[ { yycolumn += yyleng; return LBRACKET; }
<CODE>\] { yycolumn += yyleng; return RBRACKET; }

<CODE>[ \t\r]+ { yycolumn += yyleng; }
<CODE>\r\n { yycolumn = 1; return NEWLINE; }
<CODE>\n   { yycolumn = 1; return NEWLINE; }

<CODE>[A-Za-z_][A-Za-z0-9_]* { yycolumn += yyleng; yylval.s = xstrdup(yytext); return IDENT; }

<CODE>; {
    fprintf(stderr, "File \"%s\", line %d, character %d: lexical error\n",
            g_filename, yylineno, yycolumn);
    exit(1);
}

<CODE>. {
    fprintf(stderr, "File \"%s\", line %d, character %d: lexical error\n",
            g_filename, yylineno, yycolumn);
    exit(1);
}

%%
