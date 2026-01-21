#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
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
#define PIXEL  1

extern FILE* yyin;
const char* g_filename;

/* Parent spawns child in conhost, child continues. */
static int ensure_child_conhost(int argc, char** argv, const char** script_out)
{
    /* Child mode: argv[1] == --child, argv[2] == script */
    if (argc >= 3 && strcmp(argv[1], "--child") == 0) {
        *script_out = argv[2];
        return 1; /* continue in child */
    }

    /* Parent mode: expects a script path */
    if (argc < 2) {
        fprintf(stderr, "Usage: cpix <file.Cpix>\n");
        return 0;
    }

    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, MAX_PATH);

    char cmdline[4096];

    /* Canonical Windows quoting pattern for cmd:
       cmd.exe /K ""<exe>" --child "<script>""  */
    snprintf(cmdline, sizeof(cmdline),
             "C:\\Windows\\System32\\conhost.exe C:\\Windows\\System32\\cmd.exe /K \"\"%s\" --child \"%s\"\"",
             exe, argv[1]);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "CreateProcess failed: %lu", (unsigned long)GetLastError());
        MessageBoxA(NULL, msg, "cpix", MB_OK | MB_ICONERROR);
        return 0;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* Parent exits; child runs in conhost */
    return 0;
}

int main(int argc, char* argv[])
{
    const char* script = NULL;

    /* Ensure we are running in conhost child */
    if (!ensure_child_conhost(argc, argv, &script)) return 0;

    /* ---------- Parse + generate quadruplets ---------- */
    g_filename = script;
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

    /* Optional: dump quads (remove this block if you want it smaller) */
    {
        char out_filename[1024];
        snprintf(out_filename, sizeof(out_filename), "%s.quads.txt", g_filename);
        FILE* out = fopen(out_filename, "w");
        if (out) { quad_dump(out); fclose(out); }
    }

    /* ---------- ConsolePix init ---------- */
    SetConsoleTitleA("cpix");

    if (!ConstructConsole(WIDTH, HEIGHT, PIXEL, PIXEL, 23)) {
        fprintf(stderr, "ConstructConsole failed.\n");
        return 1;
    }

    /* ---------- Run VM blocks ---------- */
    vm_init();
    vm_run_block("START");
    char titleBuffer[128];
    for (;;) {
        BeginFrame();


        int rc = vm_run_block("UPDATE");
        if (rc != 0) {
            fprintf(stderr, "UPDATE failed\n");
            getchar(); /* keep console open */
            break;
        }
        if (GetKey(VK_ESCAPE)->bPressed) break;

        EndFrame(1);
        double fps = GetFPS();
        snprintf(titleBuffer, sizeof(titleBuffer), "FPS: %.2f", fps);
        UpdateTitle(titleBuffer);
    }

    CleanupConsole();
    vm_shutdown();
    return 0;
}
