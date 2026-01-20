#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"
#include "vm.h"
#include "parser.tab.h"

extern FILE* yyin;
const char* g_filename;

int main(int argc, char* argv[]) {
    //MessageBoxA(NULL, "Program is running", "cpix", MB_OK);
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

static void attach_console(void)
{
    if (!GetConsoleWindow()) AllocConsole();
    freopen("CONIN$",  "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR cmd, int show)
{
    (void)h; (void)p; (void)cmd; (void)show;

    attach_console();
    // Use MinGW globals for argv/argc in a Windows-subsystem app
    int rc = main(__argc, __argv);

    MessageBoxA(NULL, "Entered WinMain", "cpix", MB_OK);

    // Optional: keep window open so you can read output
    system("pause");
    return rc;
}
