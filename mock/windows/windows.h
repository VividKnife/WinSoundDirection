#pragma once
// Mock Windows.h for cross-platform development
// This file provides basic Windows type definitions for compilation on non-Windows platforms
// The actual functionality will not work - this is for syntax checking only

#ifdef MOCK_WINDOWS_APIS

#include <cstddef>

// Basic Windows types
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HICON;
typedef void* HCURSOR;
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
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef long LRESULT;
typedef unsigned long UINT_PTR;
typedef unsigned long DWORD_PTR;
typedef unsigned int UINT32;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef void* LPVOID;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef DWORD* LPDWORD;
typedef UINT* LPUINT;
typedef LONG* LPLONG;

// Windows constants
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

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
#define WM_APP 0x8000
#define WM_NCCREATE 0x0081

#define PM_REMOVE 0x0001

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

// MessageBox flags
#define MB_OK 0x00000000L
#define MB_ICONINFORMATION 0x00000040L
#define MB_ICONWARNING 0x00000030L
#define MB_ICONERROR 0x00000010L

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

typedef struct tagWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    DWORD wParam;
    DWORD lParam;
    DWORD time;
    POINT pt;
} MSG;

typedef struct tagWNDCLASSEXW {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
    HICON hIconSm;
} WNDCLASSEXW;

typedef struct tagNOTIFYICONDATAW {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    wchar_t szTip[128];
} NOTIFYICONDATA;

// Mock function declarations (will not work)
inline BOOL PeekMessage(MSG*, HWND, UINT, UINT, UINT) { return FALSE; }
inline BOOL TranslateMessage(const MSG*) { return FALSE; }
inline LONG DispatchMessage(const MSG*) { return 0; }
inline void PostQuitMessage(int) {}
inline DWORD GetTickCount() { return 0; }
inline void Sleep(DWORD) {}
inline BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return FALSE; }
inline BOOL ShowWindow(HWND, int) { return FALSE; }
inline HWND GetForegroundWindow() { return NULL; }
inline BOOL GetWindowRect(HWND, RECT*) { return FALSE; }
inline BOOL GetClientRect(HWND, RECT*) { return FALSE; }
inline BOOL SetForegroundWindow(HWND) { return TRUE; }
inline HINSTANCE GetModuleHandle(LPCSTR) { return NULL; }
inline DWORD GetLastError() { return 0; }
inline int MessageBoxA(HWND, LPCSTR, LPCSTR, UINT) { return 0; }
inline BOOL RegisterClassExW(const WNDCLASSEXW*) { return TRUE; }
inline BOOL UnregisterClassW(LPCWSTR, HINSTANCE) { return TRUE; }
inline HWND CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID) { return NULL; }
inline BOOL DestroyWindow(HWND) { return TRUE; }
inline BOOL UpdateWindow(HWND) { return TRUE; }
inline BOOL SetTimer(HWND, UINT_PTR, UINT, void*) { return TRUE; }
inline BOOL KillTimer(HWND, UINT_PTR) { return TRUE; }
inline BOOL RegisterHotKey(HWND, int, UINT, UINT) { return TRUE; }
inline BOOL UnregisterHotKey(HWND, int) { return TRUE; }
inline BOOL Shell_NotifyIcon(DWORD, NOTIFYICONDATA*) { return TRUE; }
inline HMENU CreatePopupMenu() { return nullptr; }
inline BOOL DestroyMenu(HMENU) { return TRUE; }
inline BOOL AppendMenuW(HMENU, UINT, UINT_PTR, LPCWSTR) { return TRUE; }
inline BOOL InsertMenuW(HMENU, UINT, UINT, UINT_PTR, LPCWSTR) { return TRUE; }
inline UINT TrackPopupMenu(HMENU, UINT, int, int, int, HWND, const RECT*) { return 0; }

// COM
inline long CoInitializeEx(void*, DWORD) { return S_OK; }
inline void CoUninitialize() {}

#endif // MOCK_WINDOWS_APIS