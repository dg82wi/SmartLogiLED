// SmartLogiLED.cpp : Defines the entry point for the application.
//
// This application controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock)
// and allows the user to customize colors via a GUI and tray icon.

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_Logic.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <shellapi.h>
#include <commdlg.h>

#define MAX_LOADSTRING 100

// Global variables:
HINSTANCE hInst;                                // Current instance
WCHAR szTitle[MAX_LOADSTRING];                  // Title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // Main window class name
HHOOK keyboardHook;                            // Keyboard hook handle
NOTIFYICONDATA nid;                            // Tray icon data

// Start minimized setting
bool startMinimized = false;

// Color settings for lock keys and default
COLORREF capsLockColor = RGB(0, 179, 0); // Caps Lock color
COLORREF scrollLockColor = RGB(0, 179, 0); // Scroll Lock color
COLORREF numLockColor = RGB(0, 179, 0); // Num Lock color
COLORREF offColor = RGB(0, 89, 89); // Lock Key off color
COLORREF defaultColor = RGB(0, 89, 89); // default color used for all other keys

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Help(HWND, UINT, WPARAM, LPARAM);
void                CreateTrayIcon(HWND hWnd);
void                RemoveTrayIcon();
void                ShowTrayContextMenu(HWND hWnd);

// Entry point for the application
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Load start minimized setting from registry
    startMinimized = LoadStartMinimizedSetting();

    // Load colors from registry
    LoadColorsFromRegistry();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SMARTLOGILED, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Application initialization
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SMARTLOGILED));

    MSG msg;

    // Main message loop
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

// Registers the window class
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMARTLOGILED));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SMARTLOGILED);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// Create the icon in the taskbar (system tray)
void CreateTrayIcon(HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMARTLOGILED));
    nid.uCallbackMessage = WM_APP + 1; // Custom message
    wcscpy_s(nid.szTip, L"SmartLogiLED");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Remove the icon from the taskbar
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// Show tray context menu (right-click)
void ShowTrayContextMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L"Open");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING | (startMinimized ? MF_CHECKED : MF_UNCHECKED), 
                ID_TRAY_START_MINIMIZED, L"Start minimized");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_CLOSE, L"Close");
    SetForegroundWindow(hWnd); // Required for menu to disappear correctly
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

// Main window procedure (handles messages)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBrushNum = NULL, hBrushCaps = NULL, hBrushScroll = NULL, hBrushOff = NULL, hBrushDefault = NULL;
    switch (message) {
        case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);
                switch (wmId) {
                    case IDC_BOX_NUMLOCK:
                        // Show color picker for NumLock
                        ShowColorPicker(hWnd, numLockColor, LogiLed::KeyName::NUM_LOCK);
                        if (hBrushNum) DeleteObject(hBrushNum);
                        hBrushNum = CreateSolidBrush(numLockColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_NUMLOCK), NULL, TRUE);
                        // Set color according to lock state
                        if ((GetKeyState(VK_NUMLOCK) & 0x0001) == 0x0001)
                            SetKeyColor(LogiLed::KeyName::NUM_LOCK, numLockColor);
                        else
                            SetKeyColor(LogiLed::KeyName::NUM_LOCK, offColor);
                        SaveColorsToRegistry();
                        break;
                    case IDC_BOX_CAPSLOCK:
                        // Show color picker for CapsLock
                        ShowColorPicker(hWnd, capsLockColor, LogiLed::KeyName::CAPS_LOCK);
                        if (hBrushCaps) DeleteObject(hBrushCaps);
                        hBrushCaps = CreateSolidBrush(capsLockColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_CAPSLOCK), NULL, TRUE);
                        if ((GetKeyState(VK_CAPITAL) & 0x0001) == 0x0001)
                            SetKeyColor(LogiLed::KeyName::CAPS_LOCK, capsLockColor);
                        else
                            SetKeyColor(LogiLed::KeyName::CAPS_LOCK, offColor);
                        SaveColorsToRegistry();
                        break;
                    case IDC_BOX_SCROLLLOCK:
                        // Show color picker for ScrollLock
                        ShowColorPicker(hWnd, scrollLockColor, LogiLed::KeyName::SCROLL_LOCK);
                        if (hBrushScroll) DeleteObject(hBrushScroll);
                        hBrushScroll = CreateSolidBrush(scrollLockColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_SCROLLLOCK), NULL, TRUE);
                        if ((GetKeyState(VK_SCROLL) & 0x0001) == 0x0001)
                            SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, scrollLockColor);
                        else
                            SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, offColor);
                        SaveColorsToRegistry();
                        break;
                    case IDC_BOX_DEFAULTCOLOR:
                        // Show color picker for default color
                        ShowColorPicker(hWnd, defaultColor);
                        if (hBrushDefault) DeleteObject(hBrushDefault);
                        hBrushDefault = CreateSolidBrush(defaultColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_DEFAULTCOLOR), NULL, TRUE);
                        // set new color for all keys
                        SetDefaultColor(defaultColor);
                        // set lock keys color
                        SetLockKeysColor();
                        SaveColorsToRegistry();
                        break;
                    case IDC_BOX_OFFCOLOR:
                        // Show color picker for off color
                        ShowColorPicker(hWnd, offColor);
                        if (hBrushOff) DeleteObject(hBrushOff);
                        hBrushOff = CreateSolidBrush(offColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_OFFCOLOR), NULL, TRUE);
                        // Update all lock keys to off color if not active
                        SetLockKeysColor();
                        SaveColorsToRegistry();
                        break;
                    case IDM_ABOUT:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                        break;
                    case IDM_HELP:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPBOX), hWnd, Help);
                        break;
                    case IDM_EXIT:
                        DestroyWindow(hWnd);
                        break;
                    case ID_TRAY_OPEN:
                        ShowWindow(hWnd, SW_RESTORE);
                        RemoveTrayIcon();
                        break;
                    case ID_TRAY_START_MINIMIZED:
                        startMinimized = !startMinimized;
                        SaveStartMinimizedSetting(startMinimized);
                        break;
                    case ID_TRAY_CLOSE:
                        DestroyWindow(hWnd);
                        break;
                    default:
                        return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
            break;
        case WM_CTLCOLORSTATIC:
            {
                // Set brush color for static controls (color boxes)
                HDC hdcStatic = (HDC)wParam;
                HWND hCtrl = (HWND)lParam;
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_NUMLOCK)) {
                    if (hBrushNum) DeleteObject(hBrushNum);
                    hBrushNum = CreateSolidBrush(numLockColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)hBrushNum;
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_CAPSLOCK)) {
                    if (hBrushCaps) DeleteObject(hBrushCaps);
                    hBrushCaps = CreateSolidBrush(capsLockColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)hBrushCaps;
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_SCROLLLOCK)) {
                    if (hBrushScroll) DeleteObject(hBrushScroll);
                    hBrushScroll = CreateSolidBrush(scrollLockColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)hBrushScroll;
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_OFFCOLOR)) {
                    if (hBrushOff) DeleteObject(hBrushOff);
                    hBrushOff = CreateSolidBrush(offColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)hBrushOff;
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_DEFAULTCOLOR)) {
                    if (hBrushDefault) DeleteObject(hBrushDefault);
                    hBrushDefault = CreateSolidBrush(defaultColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)hBrushDefault;
                }
            }
            break;
        case WM_PAINT:
            {
                // Paint color boxes
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                RECT r;
                HWND hBox;
                hBox = GetDlgItem(hWnd, IDC_BOX_NUMLOCK);
                if (hBox) {
                    GetWindowRect(hBox, &r);
                    MapWindowPoints(NULL, hWnd, (LPPOINT)&r, 2);
                    HBRUSH brush = CreateSolidBrush(numLockColor);
                    FillRect(hdc, &r, brush);
                    DeleteObject(brush);
                }
                hBox = GetDlgItem(hWnd, IDC_BOX_CAPSLOCK);
                if (hBox) {
                    GetWindowRect(hBox, &r);
                    MapWindowPoints(NULL, hWnd, (LPPOINT)&r, 2);
                    HBRUSH brush = CreateSolidBrush(capsLockColor);
                    FillRect(hdc, &r, brush);
                    DeleteObject(brush);
                }
                hBox = GetDlgItem(hWnd, IDC_BOX_SCROLLLOCK);
                if (hBox) {
                    GetWindowRect(hBox, &r);
                    MapWindowPoints(NULL, hWnd, (LPPOINT)&r, 2);
                    HBRUSH brush = CreateSolidBrush(scrollLockColor);
                    FillRect(hdc, &r, brush);
                    DeleteObject(brush);
                }
                hBox = GetDlgItem(hWnd, IDC_BOX_OFFCOLOR);
                if (hBox) {
                    GetWindowRect(hBox, &r);
                    MapWindowPoints(NULL, hWnd, (LPPOINT)&r, 2);
                    HBRUSH brush = CreateSolidBrush(offColor);
                    FillRect(hdc, &r, brush);
                    DeleteObject(brush);
                }
                hBox = GetDlgItem(hWnd, IDC_BOX_DEFAULTCOLOR);
                if (hBox) {
                    GetWindowRect(hBox, &r);
                    MapWindowPoints(NULL, hWnd, (LPPOINT)&r, 2);
                    HBRUSH brush = CreateSolidBrush(defaultColor);
                    FillRect(hdc, &r, brush);
                    DeleteObject(brush);
                }
                EndPaint(hWnd, &ps);
            }
            break;
        case WM_DESTROY:
            // Cleanup brushes and tray icon
            if (hBrushNum) DeleteObject(hBrushNum);
            if (hBrushCaps) DeleteObject(hBrushCaps);
            if (hBrushScroll) DeleteObject(hBrushScroll);
            if (hBrushOff) DeleteObject(hBrushOff);
            if (hBrushDefault) DeleteObject(hBrushDefault);
            RemoveTrayIcon();
            UnhookWindowsHookEx(keyboardHook);
            LogiLedRestoreLighting();
            LogiLedShutdown();
            PostQuitMessage(0);
            break;
        case WM_CLOSE:
            // Minimize to tray instead of closing
            ShowWindow(hWnd, SW_HIDE);
            CreateTrayIcon(hWnd);
            // Do not destroy window!
            break;
        case WM_SYSCOMMAND:
            // Minimize to tray on minimize
            if ((wParam & 0xFFF0) == SC_MINIMIZE) {
                ShowWindow(hWnd, SW_HIDE);
                CreateTrayIcon(hWnd);
            }
			else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
            break;
        case WM_APP + 1: // Custom message for tray icon
            if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hWnd, SW_RESTORE);
                RemoveTrayIcon();
            } else if (lParam == WM_RBUTTONUP) {
                ShowTrayContextMenu(hWnd);
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Stores the instance handle and creates the main window
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;
   // Fixed window size
   HWND hWnd = CreateWindowW(szWindowClass, szTitle,
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, 0, 450, 340, nullptr, nullptr, hInstance, nullptr);
   if (!hWnd) return FALSE;

   // Group Box for lock keys
   HWND hGroup = CreateWindowW(L"BUTTON", L"Lock Keys Color", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
      20, 10, 400, 140, hWnd, (HMENU)IDC_GROUP_LOCKS, hInstance, nullptr);

   // Color Boxes and Labels for lock keys inside the Group Box
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 40, 30, 60, 60, hWnd, (HMENU)IDC_BOX_NUMLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 140, 30, 60, 60, hWnd, (HMENU)IDC_BOX_CAPSLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 240, 30, 60, 60, hWnd, (HMENU)IDC_BOX_SCROLLLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"NUM LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_NUMLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"CAPS LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_CAPSLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"SCROLL LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 240, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_SCROLLLOCK, hInstance, nullptr);

   // Off Color Box and Label for lock keys
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 340, 30, 60, 60, hWnd, (HMENU)IDC_BOX_OFFCOLOR, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Off", WS_VISIBLE | WS_CHILD | SS_CENTER, 340, 95, 60, 20, hWnd, (HMENU)IDC_LABEL_OFFCOLOR, hInstance, nullptr);

   // Default Color Box and Label for other keys
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 40, 160, 60, 60, hWnd, (HMENU)IDC_BOX_DEFAULTCOLOR, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Other Keys", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 225, 60, 40, hWnd, (HMENU)IDC_LABEL_DEFAULTCOLOR, hInstance, nullptr);

   // Show window according to start minimized setting
   if (startMinimized) {
       ShowWindow(hWnd, SW_HIDE);
       CreateTrayIcon(hWnd);
   } else {
       ShowWindow(hWnd, nCmdShow);
   }
   UpdateWindow(hWnd);

   // Initialize Logitech LED SDK
   bool LedInitialized = LogiLedInit();
   if (!LedInitialized) {
       MessageBox(hWnd, L"Couldn't initialize LogiTech LED SDK", L"ERROR", 0);
       return 0;
   } else if (!LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB)) {
       MessageBox(hWnd, L"Couldn't set LOGI_DEVICETYPE_PERKEY_RGB mode", L"ERROR", 0);
       return 0;
   }
   
   // Save current lighting state
   LogiLedSaveCurrentLighting();

   // Set all keys to default color
   SetDefaultColor(defaultColor);

   // Set colors for lock keys based on current state
   SetLockKeysColor();

   // Set up keyboard hook
   keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
   
   return TRUE;
}

// Callback function for the About dialog box
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Load and set the 64x64 small icon for About dialog
        HICON hIcon = (HICON)LoadImage(
            hInst,
            MAKEINTRESOURCE(IDI_SMALL),
            IMAGE_ICON,
            64, 64,
            LR_DEFAULTCOLOR
        );
        if (hIcon)
        {
            SendDlgItemMessage(hDlg, IDC_STATIC_ICON_ABOUT, STM_SETICON, (WPARAM)hIcon, 0);
        }
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Callback function for the Help dialog box
INT_PTR CALLBACK Help(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Load and set the 256x256 main icon for Help dialog
        HICON hIcon = (HICON)LoadImage(
            hInst,
            MAKEINTRESOURCE(IDI_SMARTLOGILED),
            IMAGE_ICON,
            256, 256,
            LR_DEFAULTCOLOR
        );
        if (hIcon)
        {
            SendDlgItemMessage(hDlg, IDC_STATIC_ICON_HELP, STM_SETICON, (WPARAM)hIcon, 0);
        }
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
