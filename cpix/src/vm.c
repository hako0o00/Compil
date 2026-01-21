#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "vm.h"
#include "quad.h"
#include "ConsolePix.h"

typedef enum { V_UNDEF, V_INT, V_DBL, V_STR, V_CHAR } vkind_t;

typedef struct {
    vkind_t kind;
    int i;
    double d;
    char c;
    char* s;
} value_t;

typedef struct {
    char* name;
    value_t v;
} binding_t;

static binding_t* env = NULL;
static int env_n = 0;
static int env_cap = 0;

/* param stack for CALLN */
static value_t* pstack = NULL;
static int ptop = 0;
static int pcap = 0;

static char* sdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n);
    return p;
}

static void value_free(value_t* v) {
    if (v->kind == V_STR) free(v->s);
    v->kind = V_UNDEF;
    v->i = 0;
    v->d = 0.0;
    v->c = 0;
    v->s = NULL;
}

static value_t value_int(int x) { value_t v={0}; v.kind=V_INT; v.i=x; return v; }
static value_t value_dbl(double x){ value_t v={0}; v.kind=V_DBL; v.d=x; return v; }
static value_t value_char(char x){ value_t v={0}; v.kind=V_CHAR; v.c=x; return v; }
static value_t value_str(const char* s){ value_t v={0}; v.kind=V_STR; v.s=sdup(s); return v; }

static int parse_int(const char* s, int* out) {
    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (!end || *end != '\0') return 0;
    if (v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static int parse_dbl(const char* s, double* out) {
    char* end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (errno != 0) return 0;
    if (!end || *end != '\0') return 0;
    *out = v;
    return 1;
}

/* ^<payload> where payload is a single char or \n \t \\ \' \r */
static int parse_char_payload(const char* s, char* out) {
    if (!s || !*s) return 0;
    if (s[0] != '\\') {
        if (s[1] != '\0') return 0;
        *out = s[0];
        return 1;
    }
    if (s[1] == 'n' && s[2] == '\0') { *out = '\n'; return 1; }
    if (s[1] == 't' && s[2] == '\0') { *out = '\t'; return 1; }
    if (s[1] == '\\'&& s[2] == '\0') { *out = '\\'; return 1; }
    if (s[1] == '\''&& s[2] == '\0') { *out = '\''; return 1; }
    if (s[1] == 'r' && s[2] == '\0') { *out = '\r'; return 1; }
    return 0;
}

static int env_find(const char* name) {
    for (int i = 0; i < env_n; i++) {
        if (strcmp(env[i].name, name) == 0) return i;
    }
    return -1;
}

static int env_ensure(const char* name) {
    int idx = env_find(name);
    if (idx >= 0) return idx;

    if (env_n >= env_cap) {
        env_cap = (env_cap == 0) ? 32 : env_cap * 2;
        env = (binding_t*)realloc(env, (size_t)env_cap * sizeof(binding_t));
        if (!env) { perror("realloc"); exit(1); }
    }
    env[env_n].name = sdup(name);
    env[env_n].v.kind = V_UNDEF;
    env[env_n].v.s = NULL;
    return env_n++;
}

static value_t eval_operand(const char* opnd) {
    if (!opnd) { value_t v={0}; v.kind=V_UNDEF; return v; }

    if (opnd[0] == '#') {
        int x = 0;
        if (!parse_int(opnd + 1, &x)) { value_t v={0}; v.kind=V_UNDEF; return v; }
        return value_int(x);
    }
    if (opnd[0] == '~') {
        double x = 0.0;
        if (!parse_dbl(opnd + 1, &x)) { value_t v={0}; v.kind=V_UNDEF; return v; }
        return value_dbl(x);
    }
    if (opnd[0] == '@') {
        int x = 0;
        if (!parse_int(opnd + 1, &x)) { value_t v={0}; v.kind=V_UNDEF; return v; }
        return value_int(x); /* bool as int */
    }
    if (opnd[0] == '^') {
        char ch = 0;
        if (!parse_char_payload(opnd + 1, &ch)) { value_t v={0}; v.kind=V_UNDEF; return v; }
        return value_char(ch);
    }
    if (opnd[0] == '$') {
        return value_str(opnd + 1);
    }

    int idx = env_find(opnd);
    if (idx < 0) { value_t v={0}; v.kind=V_UNDEF; return v; }

    /* return a copy */
    value_t src = env[idx].v;
    if (src.kind == V_INT)  return value_int(src.i);
    if (src.kind == V_DBL)  return value_dbl(src.d);
    if (src.kind == V_CHAR) return value_char(src.c);
    if (src.kind == V_STR)  return value_str(src.s);
    { value_t v={0}; v.kind=V_UNDEF; return v; }
}

static void store_value(const char* name, value_t v) {
    int idx = env_ensure(name);
    value_free(&env[idx].v);

    if (v.kind == V_INT) env[idx].v = value_int(v.i);
    else if (v.kind == V_DBL) env[idx].v = value_dbl(v.d);
    else if (v.kind == V_CHAR) env[idx].v = value_char(v.c);
    else if (v.kind == V_STR) env[idx].v = value_str(v.s);
    else env[idx].v.kind = V_UNDEF;

    value_free(&v);
}

/* int-only arithmetic (matches your current parser) */
static int binop_int(const char* op, int a, int b, int* out) {
    if (strcmp(op, "ADD") == 0) { *out = a + b; return 1; }
    if (strcmp(op, "SUB") == 0) { *out = a - b; return 1; }
    if (strcmp(op, "MUL") == 0) { *out = a * b; return 1; }
    if (strcmp(op, "DIV") == 0 || strcmp(op, "IDIV") == 0) {
        if (b == 0) return 0;
        *out = a / b;
        return 1;
    }
    if (strcmp(op, "MOD") == 0) {
        if (b == 0) return 0;
        *out = a % b;
        return 1;
    }
    return 0;
}

/* ---------------- Native functions (minimal set) ---------------- */

typedef value_t (*native_fn_t)(value_t* args, int argc);

typedef struct {
    const char* name;
    int argc;
    native_fn_t fn;
    int has_return; /* 0=void, 1=returns value_t */
} native_t;

static int as_int(value_t* v, int* out) {
    if (v->kind == V_INT) { *out = v->i; return 1; }
    return 0;
}

static int as_str(value_t* v, const char** out) {
    if (v->kind == V_STR) { *out = v->s; return 1; }
    return 0;
}

static value_t n_Fill(value_t* args, int argc) {
    (void)argc;
    int x,y,w,h,col;
    if (!as_int(&args[0], &x) || !as_int(&args[1], &y) || !as_int(&args[2], &w) ||
        !as_int(&args[3], &h) || !as_int(&args[4], &col)) {
        value_t v={0}; v.kind=V_UNDEF; return v;
    }
    Fill(x,y,w,h,(WORD)col);
    value_t v={0}; v.kind=V_UNDEF; return v;
}

static value_t n_FillCircle(value_t* args, int argc) {
    (void)argc;
    int x,y,r,col;
    if (!as_int(&args[0], &x) || !as_int(&args[1], &y) || !as_int(&args[2], &r) || !as_int(&args[3], &col)) {
        value_t v={0}; v.kind=V_UNDEF; return v;
    }
    // char msg[256];
    // snprintf(msg, sizeof(msg),"Filling Circle x : %d, y: %d, r: %d , col : %d",x,y,r,col);
    // MessageBoxA(NULL, msg, "cpix", MB_OK | MB_ICONINFORMATION);

    FillCircle(x,y,r,(WORD)col);
    value_t v={0}; v.kind=V_UNDEF; return v;
}

static value_t n_GetMouseX(value_t* args, int argc) {
    (void)args; (void)argc;
    return value_int(GetMouseX());
}

static value_t n_GetMouseY(value_t* args, int argc) {
    (void)args; (void)argc;
    return value_int(GetMouseY());
}

static value_t n_WriteStringScaled(value_t* args, int argc) {
    (void)argc;
    int x,y,col,scale;
    const char* s = NULL;
    if (!as_int(&args[0], &x) || !as_int(&args[1], &y) || !as_int(&args[2], &col) ||
        !as_int(&args[3], &scale) || !as_str(&args[4], &s)) {
        value_t v={0}; v.kind=V_UNDEF; return v;
    }
    WriteStringScaled(x, y, (WORD)col, scale, (char*)s);
    value_t v={0}; v.kind=V_UNDEF; return v;
}

static native_t natives[] = {
    { "Fill",             5, n_Fill,             0 },
    { "FillCircle",       4, n_FillCircle,       0 },
    { "GetMouseX",        0, n_GetMouseX,        1 },
    { "GetMouseY",        0, n_GetMouseY,        1 },
    { "WriteStringScaled",5, n_WriteStringScaled,0 },
};

static int natives_count(void) {
    return (int)(sizeof(natives)/sizeof(natives[0]));
}

static native_t* native_lookup(const char* name) {
    for (int i = 0; i < natives_count(); i++) {
        if (strcmp(natives[i].name, name) == 0) return &natives[i];
    }
    return NULL;
}

/* ---------------- Param stack helpers ---------------- */

static void ppush(value_t v) {
    if (ptop >= pcap) {
        pcap = (pcap == 0) ? 16 : pcap * 2;
        pstack = (value_t*)realloc(pstack, (size_t)pcap * sizeof(value_t));
        if (!pstack) { perror("realloc"); exit(1); }
    }
    pstack[ptop++] = v;
}

static value_t ppop(void) {
    if (ptop <= 0) { value_t v={0}; v.kind=V_UNDEF; return v; }
    return pstack[--ptop];
}

/* ---------------- VM API ---------------- */

void vm_init(void) {
    /* keep env between frames; just reset param stack */
    ptop = 0;
}

static int find_label_pc(const char* label) {
    int count = quad_count();
    for (int i = 0; i < count; i++) {
        quad_t* q = quad_at(i);
        if (q && q->op == Q_LABEL && q->a1 && strcmp(q->a1, label) == 0) {
            return i + 1; /* run after label */
        }
    }
    return -1;
}

int vm_run_block(const char* label) {

    int pc = find_label_pc(label);
    if (pc < 0) {
        fprintf(stderr, "runtime error: missing block label '%s'\n", label);
        return 1;
    }

    int count = quad_count();
    for (int i = pc; i < count; i++) {
        quad_t* q = quad_at(i);
        if (!q) continue;

        /* stop at next label => this block ends */
        if (q->op == Q_LABEL) break;

        switch (q->op) {
        case Q_DECL: {
            env_ensure(q->a1);
            break;
        }

        case Q_ASSIGN: {
            value_t rhs = eval_operand(q->a2);
            if (rhs.kind == V_UNDEF) {
                fprintf(stderr, "runtime error: undefined value in ASSIGN (%s = %s)\n", q->a1, q->a2);
                return 1;
            }
            store_value(q->a1, rhs);
            break;
        }

        case Q_PRINT: {
            value_t v = eval_operand(q->a1);
            if (v.kind == V_INT) printf("%d\n", v.i);
            else if (v.kind == V_DBL) printf("%g\n", v.d);
            else if (v.kind == V_STR) printf("%s\n", v.s);
            else if (v.kind == V_CHAR) printf("%c\n", v.c);
            else {
                fprintf(stderr, "runtime error: undefined value in PRINT (%s)\n", q->a1);
                value_free(&v);
                return 1;
            }
            value_free(&v);
            break;
        }

        case Q_UNOP: {
            value_t a = eval_operand(q->a3);
            if (a.kind != V_INT) {
                fprintf(stderr, "runtime error: UNOP expects int\n");
                value_free(&a);
                return 1;
            }
            if (strcmp(q->a1, "NEG") == 0) {
                store_value(q->a2, value_int(-a.i));
            } else {
                fprintf(stderr, "runtime error: unknown UNOP %s\n", q->a1);
                value_free(&a);
                return 1;
            }
            value_free(&a);
            break;
        }

        case Q_BINOP: {
            value_t a = eval_operand(q->a3);
            value_t b = eval_operand(q->a4);
            if (a.kind != V_INT || b.kind != V_INT) {
                fprintf(stderr, "runtime error: BINOP expects ints\n");
                value_free(&a);
                value_free(&b);
                return 1;
            }
            int r = 0;
            if (!binop_int(q->a1, a.i, b.i, &r)) {
                fprintf(stderr, "runtime error: invalid BINOP (%s) or division by zero\n", q->a1);
                value_free(&a);
                value_free(&b);
                return 1;
            }
            store_value(q->a2, value_int(r));
            value_free(&a);
            value_free(&b);
            break;
        }

        case Q_PARAM: {
            value_t v = eval_operand(q->a1);
            if (v.kind == V_UNDEF) {
                fprintf(stderr, "runtime error: undefined value in PARAM (%s)\n", q->a1);
                return 1;
            }
            ppush(v);
            break;
        }

        case Q_CALLN: {
            const char* fname = q->a1;
            int argc = 0;
            if (!parse_int(q->a2 ? q->a2 : "0", &argc)) argc = 0;

            native_t* nf = native_lookup(fname);
            if (!nf) {
                fprintf(stderr, "runtime error: unknown native function '%s'\n", fname);
                return 1;
            }
            if (argc != nf->argc) {
                fprintf(stderr, "runtime error: native '%s' expects %d args, got %d\n", fname, nf->argc, argc);
                return 1;
            }
            // char msg[256];
            // snprintf(msg, sizeof(msg),"called my ling ling : label '%s'",label);
            // MessageBoxA(NULL, msg, "cpix", MB_OK | MB_ICONINFORMATION);

            /* pop args into array in correct order */
            value_t* args = NULL;
            if (argc > 0) {
                args = (value_t*)calloc((size_t)argc, sizeof(value_t));
                if (!args) { perror("calloc"); exit(1); }
                for (int k = argc - 1; k >= 0; k--) {
                    args[k] = ppop();
                }
            }

            value_t ret = nf->fn(args, argc);

            /* free args */
            for (int k = 0; k < argc; k++) value_free(&args[k]);
            free(args);

            if (q->a3 && q->a3[0] != '\0') {
                /* store return into temp/var */
                if (!nf->has_return) {
                    fprintf(stderr, "runtime error: assigning void return of '%s'\n", fname);
                    value_free(&ret);
                    return 1;
                }
                store_value(q->a3, ret);
            } else {
                value_free(&ret);
            }
            break;
        }

        default:
            /* ignore labels here (we already stop on them) */
            fprintf(stderr, "runtime error: unknown quad op\n");
            return 1;
        }
    }

    return 0;
}

void vm_shutdown(void) {
    /* free env */
    for (int i = 0; i < env_n; i++) {
        free(env[i].name);
        value_free(&env[i].v);
    }
    free(env);
    env = NULL;
    env_n = 0;
    env_cap = 0;

    /* free param stack */
    for (int i = 0; i < ptop; i++) value_free(&pstack[i]);
    free(pstack);
    pstack = NULL;
    ptop = 0;
    pcap = 0;
}
