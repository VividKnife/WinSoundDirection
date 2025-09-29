#pragma once
// Mock Windows.h for cross-platform development
// This file provides basic Windows type definitions for compilation on non-Windows platforms
// The actual functionality will not work - this is for syntax checking only

#ifdef MOCK_WINDOWS_APIS

// Basic Windows types
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HICON;
typedef void* HMENU;
typedef void* HDC;
typedef void* HBITMAP;
typedef void* HBRUSH;

typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef long LONG;
typedef int BOOL;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef void* LPVOID;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;

typedef DWORD* LPDWORD;
typedef UINT* LPUINT;
typedef LONG* LPLONG;

// Windows constants
#define TRUE 1
#define FALSE 0
#define NULL 0

#define MAX_PATH 260

// Virtual key codes
#define VK_HOME 0x24
#define VK_END 0x23
#define VK_ESCAPE 0x1B
#define VK_RETURN 0x0D
#define VK_SPACE 0x20
#define VK_F1 0x70
#define VK_F12 0x7B

// Modifier keys
#define MOD_ALT 0x0001
#define MOD_CONTROL 0x0002
#define MOD_SHIFT 0x0004
#define MOD_WIN 0x0008

// Window messages
#define WM_USER 0x0400
#define WM_QUIT 0x0012
#define WM_CLOSE 0x0010
#define WM_DESTROY 0x0002
#define WM_PAINT 0x000F
#define WM_SIZE 0x0005
#define WM_MOVE 0x0003
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_RBUTTONUP 0x0205
#define WM_MOUSEMOVE 0x0200
#define WM_HOTKEY 0x0312
#define WM_TIMER 0x0113
#define WM_COMMAND 0x0111
#define WM_NCCREATE 0x0081

// Window styles
#define WS_POPUP 0x80000000L
#define WS_EX_LAYERED 0x00080000
#define WS_EX_TRANSPARENT 0x00000020
#define WS_EX_TOPMOST 0x00000008
#define WS_EX_NOACTIVATE 0x08000000

// SetWindowPos flags
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0001
#define SWP_NOZORDER 0x0004
#define SWP_NOACTIVATE 0x0010

#define HWND_TOPMOST ((HWND)-1)
#define HWND_NOTOPMOST ((HWND)-2)

// ShowWindow commands
#define SW_HIDE 0
#define SW_SHOWNOACTIVATE 4

// Error codes
#define S_OK 0
#define E_FAIL 0x80004005L

// Structures
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    DWORD wParam;
    DWORD lParam;
    DWORD time;
    POINT pt;
} MSG;

// Mock function declarations (will not work)
inline BOOL PeekMessage(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) { return FALSE; }
inline BOOL TranslateMessage(const MSG* lpMsg) { return FALSE; }
inline LONG DispatchMessage(const MSG* lpMsg) { return 0; }
inline void PostQuitMessage(int nExitCode) {}
inline DWORD GetTickCount() { return 0; }
inline void Sleep(DWORD dwMilliseconds) {}
inline BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) { return FALSE; }
inline BOOL ShowWindow(HWND hWnd, int nCmdShow) { return FALSE; }
inline HWND GetForegroundWindow() { return NULL; }
inline BOOL GetWindowRect(HWND hWnd, RECT* lpRect) { return FALSE; }
inline BOOL GetClientRect(HWND hWnd, RECT* lpRect) { return FALSE; }
inline HINSTANCE GetModuleHandle(LPCSTR lpModuleName) { return NULL; }
inline DWORD GetLastError() { return 0; }

// COM
inline long CoInitializeEx(void* pvReserved, DWORD dwCoInit) { return S_OK; }
inline void CoUninitialize() {}

#endif // MOCK_WINDOWS_APIS