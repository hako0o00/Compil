#ifndef QUAD_H
#define QUAD_H

#include <stdio.h>

typedef enum {
    Q_DECL,
    Q_ASSIGN,
    Q_PRINT,
    Q_BINOP,
    Q_UNOP
} quad_op_t;

typedef struct {
    quad_op_t op;
    char* a1;
    char* a2;
    char* a3;
    char* a4;
} quad_t;

void quad_reset(void);

void quad_emit_decl(const char* name, const char* type);
void quad_emit_assign(const char* dst, const char* src);
void quad_emit_print(const char* operand);
void quad_emit_binop(const char* op, const char* res, const char* a, const char* b);
void quad_emit_unop(const char* op, const char* res, const char* a);

char* quad_new_temp(void);

int quad_count(void);
quad_t* quad_at(int i);
void quad_dump(FILE* out);

#endif
