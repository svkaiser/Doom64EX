#include <stdbool.h>
#include <windows.h>

#define EDIT_ID     100
#define COPY_ID     102
#define QUIT_ID     103
#define READY_ID    104

static HWND        hwndMain;           //Main window handle
static HWND        hwndBuffer;
static HWND        hwndButtonCopy;
static HWND        hwndButtonReady;
static HWND        hwndButtonQuit;
static HFONT       hfBufferFont;
static HFONT       hfButtonFont;
static HBRUSH      hbrEditBackground;
static HDC         hdc = 0;
static HINSTANCE   hwndMainInst;       //Main window instance

static const char d64SysConsoleClass[] = "D64SysConsole";

static LRESULT CALLBACK SysConsoleProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case QUIT_ID:
            /* I_DestroySysConsole(); */
            /* I_ShutdownVideo(); */
            exit(0);
            break;

        case COPY_ID:
            SendMessage(hwndBuffer, EM_SETSEL, 0, -1);
            SendMessage(hwndBuffer, WM_COPY, 0, 0);
            break;

        case READY_ID:
            /* NET_CL_StartGame(); */
            DestroyWindow(hwndButtonReady);
            break;
        }
        break;

    case WM_SETFOCUS:
    case WM_SHOWWINDOW:
        UpdateWindow(hwndMain);
        break;


    case WM_CREATE:
        hbrEditBackground = CreateSolidBrush(RGB(64, 64, 64));
        break;

    case WM_CTLCOLORSTATIC:
        if((HWND) lParam == hwndBuffer) {
            SetBkColor((HDC) wParam, RGB(64, 64, 64));
            SetTextColor((HDC) wParam, RGB(0xff, 0xff, 0x00));

            return (long)hbrEditBackground;
        }
        break;

    default:
        return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

void winapi_init()
{
    WNDCLASSEX  wcx;
    RECT        rect;
    int         swidth;
    int         sheight;

    hwndMainInst = GetModuleHandle(NULL);
    memset(&wcx, 0, sizeof(WNDCLASSEX));

    wcx.cbSize          = sizeof(WNDCLASSEX);
    wcx.style           = CS_OWNDC;
    wcx.lpfnWndProc     = (WNDPROC)SysConsoleProc;
    wcx.cbClsExtra      = 0;
    wcx.cbWndExtra      = 0;
    wcx.hInstance       = hwndMainInst;
    wcx.hIcon           = NULL;
    wcx.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground   = (HBRUSH)COLOR_WINDOW;
    wcx.lpszClassName   = d64SysConsoleClass;
    wcx.hIconSm         = NULL;

    if(!RegisterClassEx(&wcx)) {
        return;
    }

    rect.left   = 0;
    rect.right  = 384;
    rect.top    = 0;
    rect.bottom = 480;

    AdjustWindowRect(&rect, WS_POPUPWINDOW, FALSE);

    hdc     = GetDC(GetDesktopWindow());
    swidth  = GetDeviceCaps(hdc, HORZRES);
    sheight = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(GetDesktopWindow(), hdc);

    // Create the window
    hwndMain = CreateWindowEx(
                   0,                          //Extended window style
                   d64SysConsoleClass,         // Window class name
                   NULL,                       // Window title
                   WS_POPUPWINDOW,             // Window style
                   (swidth - 400) / 2,
                   (sheight - 480) / 2 ,
                   rect.right - rect.left + 1,
                   rect.bottom - rect.top + 1, // Width and height of the window
                   NULL,                       // HWND of the parent window (can be null also)
                   NULL,                       // Handle to menu
                   hwndMainInst,               // Handle to application instance
                   NULL                        // Pointer to window creation data
               );

    if(!hwndMain) {
        return;
    }

    hwndBuffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
                              ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                              6, 16, 370, 400,
                              hwndMain,
                              (HMENU)EDIT_ID,
                              hwndMainInst, NULL);

    hwndButtonCopy = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                  5, 445, 64, 24,
                                  hwndMain,
                                  (HMENU)COPY_ID,
                                  hwndMainInst, NULL);
    SendMessage(hwndButtonCopy, WM_SETTEXT, 0, (LPARAM) "copy");

    /* if(M_CheckParm("-server") > 0) { */
    /*     hwndButtonReady = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, */
    /*                                    82, 445, 64, 24, */
    /*                                    hwndMain, */
    /*                                    (HMENU)READY_ID, */
    /*                                    hwndMainInst, NULL); */
    /*     SendMessage(hwndButtonReady, WM_SETTEXT, 0, (LPARAM) "ready"); */
    /* } */

    hwndButtonQuit = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                  312, 445, 64, 24,
                                  hwndMain,
                                  (HMENU)QUIT_ID,
                                  hwndMainInst, NULL);
    SendMessage(hwndButtonQuit, WM_SETTEXT, 0, (LPARAM) "quit");

    hdc = GetDC(hwndMain);
    hfBufferFont = CreateFont(-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                              0,
                              0,
                              0,
                              FW_LIGHT,
                              0,
                              0,
                              0,
                              DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY,
                              FF_MODERN | FIXED_PITCH,
                              "Courier New");

    ReleaseDC(hwndMain,hdc);

    SendMessage(hwndBuffer, WM_SETFONT, (WPARAM) hfBufferFont, 0);

    ShowWindow(hwndMain,SW_SHOW);
    UpdateWindow(hwndMain);
}

void __winapi_quit()
{
    if (hwndMain) {
        // I_ShowSysConsole(false);
        CloseWindow(hwndMain);
        DestroyWindow(hwndMain);
        hwndMain = NULL;
    }
}

void winapi_show(bool show)
{
    ShowWindow(hwndMain, show ? SW_SHOW : SW_HIDE);
}

void winapi_add_text(const char *text)
{
}

void winapi_show_and_quit()
{
    MSG msg;

    winapi_show(true);

    while(1) {
        while(GetMessage(&msg,NULL,0,0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(100);
    }
}
