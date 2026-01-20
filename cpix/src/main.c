#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"
#include "vm.h"
#include "parser.tab.h"

extern FILE* yyin;
const char* g_filename;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: cpix <file.Cpix>\n");
        return 1;
    }

    g_filename = argv[1];
    FILE* f = fopen(g_filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", g_filename);
        return 1;
    }

    yyin = f;
    quad_reset();

    if (yyparse() != 0) {
        fclose(f);
        return 1;
    }

    fclose(f);

    char out_filename[1024];
    strcpy(out_filename, g_filename);
    strcat(out_filename, ".quads.txt");
    FILE* out = fopen(out_filename, "w");
    if (out) {
        quad_dump(out);
        fclose(out);
    }

    int result = vm_run();
    return result;
}
