// SmartLogiLED.cpp : Defines the entry point for the application.
//
// This application controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock)
// and allows the user to customize colors via a GUI and tray icon.

#include "framework.h"
#include "SmartLogiLED.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <shellapi.h>
#include <commdlg.h>
#include <fstream>
#include <sstream>

#define MAX_LOADSTRING 100

// Global variables:
HINSTANCE hInst;                                // Current instance
WCHAR szTitle[MAX_LOADSTRING];                  // Title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // Main window class name
HHOOK keyboardHook;                            // Keyboard hook handle
NOTIFYICONDATA nid;                            // Tray icon data

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

// Set color for a key using Logitech LED SDK
void SetKeyColor(LogiLed::KeyName key, COLORREF color) {
    int r = GetRValue(color) * 100 / 255;
    int g = GetGValue(color) * 100 / 255;
    int b = GetBValue(color) * 100 / 255;
    LogiLedSetLightingForKeyWithKeyName(key, r, g, b);
}

// Set color for all keys
void SetDefaultColor(COLORREF color) {
    defaultColor = color;
    int r = GetRValue(defaultColor) * 100 / 255;
    int g = GetGValue(defaultColor) * 100 / 255;
    int b = GetBValue(defaultColor) * 100 / 255;
    LogiLedSetLighting(r, g, b);
}

// Set color for lock keys depending on their state
void SetLockKeysColor(void) {
    SHORT keyState;
    LogiLed::KeyName pressedKey;
    COLORREF colorToSet;

    // NumLock
    keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    pressedKey = LogiLed::KeyName::NUM_LOCK;
    colorToSet = (keyState == 0x0001) ? numLockColor : offColor;
    SetKeyColor(pressedKey, colorToSet);

    // ScrollLock
    keyState = GetKeyState(VK_SCROLL) & 0x0001;
    pressedKey = LogiLed::KeyName::SCROLL_LOCK;
    colorToSet = (keyState == 0x0001) ? scrollLockColor : offColor;
    SetKeyColor(pressedKey, colorToSet);

    // CapsLock
    keyState = GetKeyState(VK_CAPITAL) & 0x0001;
    pressedKey = LogiLed::KeyName::CAPS_LOCK;
    colorToSet = (keyState == 0x0001) ? capsLockColor : offColor;
    SetKeyColor(pressedKey, colorToSet);
}

// Show color picker dialog and update color
void ShowColorPicker(HWND hWnd, COLORREF& color, LogiLed::KeyName key = LogiLed::KeyName::ESC) {
    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.rgbResult = color;
    COLORREF custColors[16] = {};
    cc.lpCustColors = custColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc)) {
        color = cc.rgbResult;
        if (key != LogiLed::KeyName::ESC) {
            SetKeyColor(key, color);
        }
        InvalidateRect(hWnd, NULL, TRUE);
    }
}

// Save color settings to INI file
void SaveColorsToIni() {
    std::ofstream ini("settings.ini");
    ini << "[Colors]\n";
    ini << "NumLock=" << (int)GetRValue(numLockColor) << "," << (int)GetGValue(numLockColor) << "," << (int)GetBValue(numLockColor) << "\n";
    ini << "CapsLock=" << (int)GetRValue(capsLockColor) << "," << (int)GetGValue(capsLockColor) << "," << (int)GetBValue(capsLockColor) << "\n";
    ini << "ScrollLock=" << (int)GetRValue(scrollLockColor) << "," << (int)GetGValue(scrollLockColor) << "," << (int)GetBValue(scrollLockColor) << "\n";
    ini << "LockKeyOff=" << (int)GetRValue(offColor) << "," << (int)GetGValue(offColor) << "," << (int)GetBValue(offColor) << "\n";
    ini << "Default=" << (int)GetRValue(defaultColor) << "," << (int)GetGValue(defaultColor) << "," << (int)GetBValue(defaultColor) << "\n";
}

// Load color settings from INI file
void LoadColorsFromIni() {
    std::ifstream ini("settings.ini");
    std::string line;
    while (std::getline(ini, line)) {
        if (line.find("NumLock=") == 0) {
            int r, g, b;
            sscanf_s(line.c_str(), "NumLock=%d,%d,%d", &r, &g, &b);
            numLockColor = RGB(r, g, b);
        } else if (line.find("CapsLock=") == 0) {
            int r, g, b;
            sscanf_s(line.c_str(), "CapsLock=%d,%d,%d", &r, &g, &b);
            capsLockColor = RGB(r, g, b);
        } else if (line.find("ScrollLock=") == 0) {
            int r, g, b;
            sscanf_s(line.c_str(), "ScrollLock=%d,%d,%d", &r, &g, &b);
            scrollLockColor = RGB(r, g, b);
        } else if (line.find("LockKeyOff=") == 0) {
            int r, g, b;
            sscanf_s(line.c_str(), "LockKeyOff=%d,%d,%d", &r, &g, &b);
            offColor = RGB(r, g, b);
        } else if (line.find("Default=") == 0) {
            int r, g, b;
            sscanf_s(line.c_str(), "Default=%d,%d,%d", &r, &g, &b);
            defaultColor = RGB(r, g, b);
        }
    }
}

// Entry point for the application
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Load colors from settings.ini
    LoadColorsFromIni();

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

// Keyboard hook procedure to update lock key colors on key press
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;

        // Check if lock key was pressed
        if ((pKeyStruct->vkCode == VK_NUMLOCK) || (pKeyStruct->vkCode == VK_SCROLL) || (pKeyStruct->vkCode == VK_CAPITAL)) {
            SHORT keyState;
            LogiLed::KeyName pressedKey;
            COLORREF colorToSet;

            // The lock key state is updated only after this callback, so current state off means the key will be turned on
            switch (pKeyStruct->vkCode) {
            case VK_NUMLOCK:
                keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
                pressedKey = LogiLed::KeyName::NUM_LOCK;
                colorToSet = (keyState == 0x0000) ? numLockColor : offColor;
                break;
            case VK_SCROLL:
                keyState = GetKeyState(VK_SCROLL) & 0x0001;
                pressedKey = LogiLed::KeyName::SCROLL_LOCK;
                colorToSet = (keyState == 0x0000) ? scrollLockColor : offColor;
                break;
            case VK_CAPITAL:
                keyState = GetKeyState(VK_CAPITAL) & 0x0001;
                pressedKey = LogiLed::KeyName::CAPS_LOCK;
                colorToSet = (keyState == 0x0000) ? capsLockColor : offColor;
                break;
            }

            SetKeyColor(pressedKey, colorToSet);
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
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
                        SaveColorsToIni();
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
                        SaveColorsToIni();
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
                        SaveColorsToIni();
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
                        SaveColorsToIni();
                        break;
                    case IDC_BOX_OFFCOLOR:
                        // Show color picker for off color
                        ShowColorPicker(hWnd, offColor);
                        if (hBrushOff) DeleteObject(hBrushOff);
                        hBrushOff = CreateSolidBrush(offColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_OFFCOLOR), NULL, TRUE);
                        // Update all lock keys to off color if not active
                        SetLockKeysColor();
                        SaveColorsToIni();
                        break;
                    case IDM_ABOUT:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                        break;
                    case IDM_EXIT:
                        DestroyWindow(hWnd);
                        break;
                    case ID_TRAY_OPEN:
                        ShowWindow(hWnd, SW_RESTORE);
                        RemoveTrayIcon();
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

   ShowWindow(hWnd, SW_HIDE);
   UpdateWindow(hWnd);
   CreateTrayIcon(hWnd);

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
        return (INT_PTR)TRUE;

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
