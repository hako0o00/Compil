#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"
#include "vm.h"
#include "parser.tab.h"
#include "ConsolePix.h"

#define WIDTH  400
#define HEIGHT 400
#define PIXEL  2
#define CIRCLE_RADIUS 20

extern FILE* yyin;
const char* g_filename;

int main(int argc, char* argv[]) {

    if (!ConsolePix_EnsureConhost()) return 0;

    // === Child instance (now hosted by conhost) ===
    SetConsoleTitleA("My Console");

    if (!ConstructConsole(WIDTH, HEIGHT, PIXEL, PIXEL, 23)) {
        printf("ConstructConsole failed.\n");
        system("pause");
        return 1;
    }

    char titleBuffer[128];
    for (;;) {
        BeginFrame();

        Fill(0, 0, WIDTH, HEIGHT, 0x0000);
        FillCircle(GetMouseX(), GetMouseY(), CIRCLE_RADIUS, 0x0004);

        WriteStringScaled(10,10,0x00FF,1,"Himaron");
        WriteStringScaled(10,40,0x00FF,2,"Himaron");
        WriteStringScaled(10,80,0x00FF,3,"Himaron");
        WriteStringScaled(10,150,0x00FF,4,"Himaron");

        if (GetKey(VK_ESCAPE)->bPressed) break;

        EndFrame(1);

        double fps = GetFPS();
        snprintf(titleBuffer, sizeof(titleBuffer), "FPS: %.2f", fps);
        UpdateTitle(titleBuffer);
    }

    CleanupConsole();
    system("pause");
    return 0;
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

