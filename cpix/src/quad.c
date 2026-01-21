#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"

static quad_t* quads = NULL;
static int quad_capacity = 0;
static int quad_size = 0;
static int temp_id = 0;
static int label_id = 0;

static char* qdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, s, n);
    return p;
}

static void quad_grow_if_needed(void) {
    if (quad_size >= quad_capacity) {
        quad_capacity = (quad_capacity == 0) ? 16 : (quad_capacity * 2);
        quads = (quad_t*)realloc(quads, (size_t)quad_capacity * sizeof(quad_t));
        if (!quads) { perror("realloc"); exit(1); }
    }
}

static void quad_push(quad_op_t op, const char* a1, const char* a2, const char* a3, const char* a4) {
    quad_grow_if_needed();
    quads[quad_size].op = op;
    quads[quad_size].a1 = qdup(a1);
    quads[quad_size].a2 = qdup(a2);
    quads[quad_size].a3 = qdup(a3);
    quads[quad_size].a4 = qdup(a4);
    quad_size++;
}

void quad_reset(void) {
    for (int i = 0; i < quad_size; i++) {
        free(quads[i].a1);
        free(quads[i].a2);
        free(quads[i].a3);
        free(quads[i].a4);
    }
    free(quads);
    quads = NULL;
    quad_capacity = 0;
    label_id = 0;
    quad_size = 0;
    temp_id = 0;
}

void quad_emit_jmp(const char* label) {
    quad_push(Q_JMP, label, NULL, NULL, NULL);
}

void quad_emit_jz(const char* cond, const char* label) {
    quad_push(Q_JZ, cond, label, NULL, NULL);
}

char* quad_new_label(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "L%d", label_id++);
    return qdup(buf);
}


void quad_emit_decl(const char* name, const char* type) {
    quad_push(Q_DECL, name, type, NULL, NULL);
}

void quad_emit_assign(const char* dst, const char* src) {
    quad_push(Q_ASSIGN, dst, src, NULL, NULL);
}

void quad_emit_print(const char* operand) {
    quad_push(Q_PRINT, operand, NULL, NULL, NULL);
}

void quad_emit_binop(const char* op, const char* res, const char* a, const char* b) {
    quad_push(Q_BINOP, op, res, a, b);
}

void quad_emit_unop(const char* op, const char* res, const char* a) {
    quad_push(Q_UNOP, op, res, a, NULL);
}

/* new */
void quad_emit_label(const char* name) {
    quad_push(Q_LABEL, name, NULL, NULL, NULL);
}

void quad_emit_param(const char* operand) {
    quad_push(Q_PARAM, operand, NULL, NULL, NULL);
}

void quad_emit_calln(const char* funcName, int argc, const char* result_or_null) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", argc);
    quad_push(Q_CALLN, funcName, buf, result_or_null, NULL);
}

char* quad_new_temp(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "t%d", temp_id++);
    return qdup(buf);
}

int quad_count(void) {
    return quad_size;
}

quad_t* quad_at(int i) {
    if (i < 0 || i >= quad_size) return NULL;
    return &quads[i];
}

void quad_dump(FILE* out) {
    for (int i = 0; i < quad_size; i++) {
        quad_t* q = &quads[i];
        switch (q->op) {
        case Q_DECL:
            fprintf(out, "%d: (DECL, %s, %s)\n", i, q->a1, q->a2);
            break;
        case Q_ASSIGN:
            fprintf(out, "%d: (ASSIGN, %s, %s)\n", i, q->a1, q->a2);
            break;
        case Q_PRINT:
            fprintf(out, "%d: (PRINT, %s)\n", i, q->a1);
            break;
        case Q_BINOP:
            fprintf(out, "%d: (BINOP, %s, %s, %s, %s)\n", i, q->a1, q->a2, q->a3, q->a4);
            break;
        case Q_UNOP:
            fprintf(out, "%d: (UNOP, %s, %s, %s)\n", i, q->a1, q->a2, q->a3);
            break;

        case Q_LABEL:
            fprintf(out, "%d: (LABEL, %s)\n", i, q->a1);
            break;
        case Q_PARAM:
            fprintf(out, "%d: (PARAM, %s)\n", i, q->a1);
            break;
        case Q_CALLN:
            fprintf(out, "%d: (CALLN, %s, argc=%s, res=%s)\n",
                    i, q->a1, q->a2 ? q->a2 : "0", q->a3 ? q->a3 : "_");
            break;
        case Q_JMP:
            fprintf(out, "%d: (JMP, %s)\n", i, q->a1);
            break;
        case Q_JZ:
            fprintf(out, "%d: (JZ, %s, %s)\n", i, q->a1, q->a2);
            break;

        default:
            fprintf(out, "%d: (UNKNOWN)\n", i);
            break;
        }
    }
}
