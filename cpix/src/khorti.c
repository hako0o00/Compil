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

typedef struct sKeyState {
    bool bPressed;
    bool bReleased;
    bool bHeld;
} sKeyState;


static inline int clampi(int v, int lo, int hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void print_last_error(const char* where) {
    DWORD e = GetLastError();
    char *msg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&msg, 0, NULL);
    fprintf(stderr, "%s failed (err=%lu): %s\n", where, (unsigned long)e, msg ? msg : "(no msg)");
    if (msg) LocalFree(msg);
}

static LARGE_INTEGER fps_freq;
static int64_t fps_last_update_ticks = 0;
static uint32_t fps_frame_count = 0;
static double fps_last_fps = 0.0;
static int fps_inited = 0;

double GetFPS(void) {
    if (!fps_inited) {
        QueryPerformanceFrequency(&fps_freq);
        fps_inited = 1;
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    int64_t now_ticks = now.QuadPart;

    if (fps_last_update_ticks == 0) {
        fps_last_update_ticks = now_ticks;
        return fps_last_fps;
    }

    fps_frame_count++;

    int64_t dt = now_ticks - fps_last_update_ticks;
    int64_t interval_ticks = (fps_freq.QuadPart * (int64_t)1000) / 1000;

    if (dt >= interval_ticks && dt > 0) {
        fps_last_fps = (double)fps_frame_count * (double)fps_freq.QuadPart / (double)dt;
        fps_frame_count = 0;
        fps_last_update_ticks = now_ticks;
    }

    return fps_last_fps;
}

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    (void)ctrlType;
    if (m_hOriginalConsole) SetConsoleActiveScreenBuffer(m_hOriginalConsole);
    return FALSE;
}

int ConstructConsole(int width, int height, int fontw, int fonth, int title) {
    oneP = (fontw == 1 && fonth == 1);
    m_nScreenWidth = width;
    m_nScreenHeight = height;

    m_hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_hOriginalConsole == INVALID_HANDLE_VALUE) { print_last_error("GetStdHandle(STD_OUTPUT_HANDLE)"); return 0; }
    GetConsoleScreenBufferInfo(m_hOriginalConsole, &m_OriginalConsoleInfo);

    m_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    if (m_hConsoleIn == INVALID_HANDLE_VALUE) { print_last_error("GetStdHandle(STD_INPUT_HANDLE)"); return 0; }

    m_hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if (m_hConsole == INVALID_HANDLE_VALUE) { print_last_error("CreateConsoleScreenBuffer"); return 0; }

    m_rectWindow = one;
    SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow);

    COORD coord = { (SHORT)width, (SHORT)height };
    if (!SetConsoleScreenBufferSize(m_hConsole, coord)) { print_last_error("SetConsoleScreenBufferSize"); return 0; }
    if (!SetConsoleActiveScreenBuffer(m_hConsole)) { print_last_error("SetConsoleActiveScreenBuffer"); return 0; }

    CONSOLE_FONT_INFOEX cfi;
    memset(&cfi, 0, sizeof(cfi));
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = (SHORT)fontw;
    cfi.dwFontSize.Y = (SHORT)fonth;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(m_hConsole, FALSE, &cfi);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(m_hConsole, &csbi)) { print_last_error("GetConsoleScreenBufferInfo"); return 0; }
    if (height > csbi.dwMaximumWindowSize.Y || width > csbi.dwMaximumWindowSize.X) return 0;

    SMALL_RECT scr = { 0, 0, (SHORT)(width - 1), (SHORT)(height - 1) };
    m_rectWindow = scr;
    if (!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow)) { print_last_error("SetConsoleWindowInfo(final)"); return 0; }

    if (!title) {
        HWND consoleWindow = GetConsoleWindow();
        if (consoleWindow) {
            LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_THICKFRAME);
            SetWindowLong(consoleWindow, GWL_STYLE, style);
            SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    DWORD inMode = 0;
    if (GetConsoleMode(m_hConsoleIn, &inMode)) {
        inMode |= (ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
        inMode &= ~(ENABLE_QUICK_EDIT_MODE);
        SetConsoleMode(m_hConsoleIn, inMode);
    }

    free(m_bufScreen);
    free(m_partBuffer);
    m_bufScreen = (CHAR_INFO*)calloc((size_t)width * (size_t)height, sizeof(CHAR_INFO));
    m_partBuffer = NULL;
    m_partBufferSize = 0;
    if (!m_bufScreen) return 0;


    free(m_bufAttributes);
    m_bufAttributes = (WORD*)calloc((size_t)width * (size_t)height, sizeof(WORD));
    if (!m_bufAttributes) return 0;
    
    for (int i = 0; i < width * height -1; i++)
            m_bufScreen[i].Char.UnicodeChar = (WCHAR)'O';

    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    return 1;
}

static int drawn = 0;

static sKeyState m_mouse[5];
static sKeyState m_keys[256];

static int m_nScreenWidth  = 0;
static int m_nScreenHeight = 0;

static CHAR_INFO *m_bufScreen = NULL;
static CHAR_INFO *m_partBuffer = NULL;
static int m_partBufferSize = 0;
static WORD      *m_bufAttributes = NULL;

static HANDLE m_hOriginalConsole = NULL;
static CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;

static HANDLE m_hConsole   = INVALID_HANDLE_VALUE;
static HANDLE m_hConsoleIn = INVALID_HANDLE_VALUE;

static SMALL_RECT m_rectWindow;
static SMALL_RECT one = { 0, 0, 1, 1 };

static short m_keyOldState[256] = {0};
static short m_keyNewState[256] = {0};
static bool  m_mouseOldState[5] = {0};
static bool  m_mouseNewState[5] = {0};

static bool m_bConsoleInFocus = true;

static int m_mousePosX = 0;
static int m_mousePosY = 0;

static INPUT_RECORD inBuf[32];
static bool oneP = false;

extern FILE* yyin;
const char* g_filename;

int main(int argc, char* argv[]) {
    MessageBoxA(NULL, "Program is running", "cpix", MB_OK);
    ConstructConsole(WIDTH, HEIGHT, PIXEL, PIXEL, 23)
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
