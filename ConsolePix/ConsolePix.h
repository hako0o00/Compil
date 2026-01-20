#ifndef CONSOLEPIX_H
#define CONSOLEPIX_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

typedef struct sKeyState {
    bool bPressed;
    bool bReleased;
    bool bHeld;
} sKeyState;

// Drawing functions
void Draw(int x, int y, short col);
void DrawLine(int x1, int y1, int x2, int y2,  short col);
void Fill(int x1, int y1, int x2, int y2, short col);
void FillCircle(int xc, int yc, int r,  short col);
void DrawCircle(int xc, int yc, int r,  short col);
void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short col);
void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short col);
void DrawLineThick(int x1, int y1, int x2, int y2, int r, short col);
void FillRoundedRect(int x, int y, int a, int b, short col);
void PrintSpriteCentered(int x1, int y1, int width, int height, short sprite[]);
void PrintSprite(int x1, int y1, int width, int height, short sprite[]);
void PrintSpriteNoBackground(int x1, int y1, int width, int height, short sprite[]);
void DrawCharScaled(char car, int x, int y, int size, short col);
void WriteStringScaled(int x, int y, short col, int size, char *chaine);

// Console management
int ConstructConsole(int width, int height, int fontw, int fonth, int title);
void updateConsolePart(int minx, int miny, int maxx, int maxy);



// Input handling
void update_In(void);
void update_Keys(void);

// Frame management
void BeginFrame(void);
void EndFrame(int shouldUpdate);
void UpdateTitle(const char* title);

// Cleanup
void CleanupConsole(void);

// Accessors
int GetMouseX(void);
int GetMouseY(void);
sKeyState* GetMouse(int button);
sKeyState* GetKey(int key);

// FPS timing
double GetFPS(void);

// Internal accessors for attributes-only updates
HANDLE GetConsoleHandle(void);
int GetScreenWidth(void);
int GetScreenHeight(void);


#endif // CONSOLEPIX_H
