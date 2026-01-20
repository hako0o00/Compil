#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ConsolePix.h"

#define WIDTH  400
#define HEIGHT 400
#define PIXEL  2
#define CIRCLE_RADIUS 20

int SetupWin(void)
{
    if (!AllocConsole()) {
        MessageBoxA(NULL, "AllocConsole failed", "Error", MB_OK);
        return 0;
    }

    freopen("CONIN$",  "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    return 1;
}

int main(void)
{
    SetupWin();

    SetConsoleTitleA("My Console");
    if (!ConstructConsole(WIDTH, HEIGHT, PIXEL, PIXEL, 23)) {
        printf("ConstructConsole failed.\n");
        system("pause");
        return 1;
    }

    char titleBuffer[128];
    while (1) {
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
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    return main();
}
