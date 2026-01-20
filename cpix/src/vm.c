#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "vm.h"
#include "quad.h"

typedef enum { V_UNDEF, V_INT, V_DBL, V_STR, V_CHAR, V_ARR } vkind_t;

typedef struct value value_t;

struct value {
    vkind_t kind;
    int i;
    double d;
    char c;
    char* s;

    /* arrays */
    value_t* arr;
    int arr_len;
};

typedef struct {
    char* name;
    value_t v;
} binding_t;

static binding_t* env = NULL;
static int env_n = 0;
static int env_cap = 0;

static char* sdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n);
    return p;
}

static void value_free(value_t* v) {
    if (v->kind == V_STR) free(v->s);
    if (v->kind == V_ARR) {
        for (int i = 0; i < v->arr_len; i++) value_free(&v->arr[i]);
        free(v->arr);
    }
    v->kind = V_UNDEF;
    v->i = 0;
    v->d = 0.0;
    v->c = 0;
    v->s = NULL;
    v->arr = NULL;
    v->arr_len = 0;
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
    env[env_n].v.i = 0;
    env[env_n].v.d = 0.0;
    env[env_n].v.c = 0;
    env[env_n].v.s = NULL;
    env[env_n].v.arr = NULL;
    env[env_n].v.arr_len = 0;
    return env_n++;
}

static value_t value_int(int x)   { value_t v={0}; v.kind=V_INT;  v.i=x; return v; }
static value_t value_dbl(double x){ value_t v={0}; v.kind=V_DBL;  v.d=x; return v; }
static value_t value_char(char x) { value_t v={0}; v.kind=V_CHAR; v.c=x; return v; }
static value_t value_str(const char* s){ value_t v={0}; v.kind=V_STR; v.s=sdup(s); return v; }

static int parse_int(const char* s, int* out) {
    char* end = NULL;
    long v = strtol(s, &end, 10);
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

/* parse ^<payload> where payload is either:
   - a single character: ^A
   - escape sequences: ^\n, ^\t, ^\\, ^\' 
*/
static int parse_char_payload(const char* s, char* out) {
    if (!s || !*s) return 0;
    if (s[0] != '\\') {
        if (s[1] != '\0') return 0; /* must be exactly one char */
        *out = s[0];
        return 1;
    }
    /* escape */
    if (s[1] == 'n' && s[2] == '\0') { *out = '\n'; return 1; }
    if (s[1] == 't' && s[2] == '\0') { *out = '\t'; return 1; }
    if (s[1] == '\\'&& s[2] == '\0') { *out = '\\'; return 1; }
    if (s[1] == '\''&& s[2] == '\0') { *out = '\''; return 1; }
    if (s[1] == 'r' && s[2] == '\0') { *out = '\r'; return 1; }
    return 0;
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
        return value_int(x); /* bool stored as int 0/1 */
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

    value_t src = env[idx].v;

    if (src.kind == V_INT)  return value_int(src.i);
    if (src.kind == V_DBL)  return value_dbl(src.d);
    if (src.kind == V_CHAR) return value_char(src.c);
    if (src.kind == V_STR)  return value_str(src.s);

    /* arrays aren't first-class values here yet */
    { value_t v={0}; v.kind=V_UNDEF; return v; }
}

static void store_value(const char* name, value_t v) {
    int idx = env_ensure(name);

    /* don't allow overwriting an array with scalar yet */
    if (env[idx].v.kind == V_ARR) {
        fprintf(stderr, "runtime error: cannot assign scalar to array (%s)\n", name);
        value_free(&v);
        return;
    }

    value_free(&env[idx].v);

    if (v.kind == V_INT)      env[idx].v = value_int(v.i);
    else if (v.kind == V_DBL) env[idx].v = value_dbl(v.d);
    else if (v.kind == V_CHAR)env[idx].v = value_char(v.c);
    else if (v.kind == V_STR) { env[idx].v.kind = V_STR; env[idx].v.s = sdup(v.s); }
    else { env[idx].v.kind = V_UNDEF; }

    value_free(&v);
}

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

/* parse "int[10]" / "double[]" etc. size returned, -1 for unsized, 0 for non-array */
static int parse_array_size(const char* type) {
    const char* lb = strchr(type, '[');
    if (!lb) return 0;
    const char* rb = strchr(lb, ']');
    if (!rb) return 0;
    if (rb == lb + 1) return -1; /* [] */
    int n = 0;
    char tmp[64];
    size_t len = (size_t)(rb - (lb + 1));
    if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
    memcpy(tmp, lb + 1, len);
    tmp[len] = '\0';
    if (!parse_int(tmp, &n)) return 0;
    return n;
}

static void ensure_array(const char* name, int n) {
    int idx = env_ensure(name);
    value_free(&env[idx].v);
    env[idx].v.kind = V_ARR;
    env[idx].v.arr_len = (n < 0) ? 0 : n; /* unsized => 0 for now */
    if (env[idx].v.arr_len > 0) {
        env[idx].v.arr = (value_t*)calloc((size_t)env[idx].v.arr_len, sizeof(value_t));
        if (!env[idx].v.arr) { perror("calloc"); exit(1); }
        for (int i = 0; i < env[idx].v.arr_len; i++) env[idx].v.arr[i].kind = V_UNDEF;
    } else {
        env[idx].v.arr = NULL;
    }
}

int vm_run(void) {
    int count = quad_count();

    for (int i = 0; i < count; i++) {
        quad_t* q = quad_at(i);

        switch (q->op) {
        case Q_DECL: {
            /* q->a1 = name, q->a2 = type (maybe with [N]) */
            int sz = parse_array_size(q->a2 ? q->a2 : "");
            if (sz != 0) ensure_array(q->a1, sz);
            else env_ensure(q->a1);
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

        default:
            fprintf(stderr, "runtime error: unknown quad op\n");
            return 1;
        }
    }

    for (int i = 0; i < env_n; i++) {
        free(env[i].name);
        value_free(&env[i].v);
    }
    free(env);
    env = NULL;
    env_n = 0;
    env_cap = 0;

    return 0;
}
