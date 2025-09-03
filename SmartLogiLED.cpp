// SmartLogiLED.cpp : Defines the entry point for the application.
//
// This application controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock)
// and allows the user to customize colors via a GUI and tray icon.

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_Logic.h"
#include "SmartLogiLED_Config.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <shellapi.h>
#include <commdlg.h>
#include <algorithm>
#include <vector>

#define MAX_LOADSTRING 100
#define WM_UPDATE_PROFILE_COMBO (WM_USER + 100)

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
COLORREF defaultColor = RGB(0, 89, 89); // default color used for all other keys and lock keys when off

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Help(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    KeysDialog(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AddProfileDialog(HWND, UINT, WPARAM, LPARAM);
void                CreateTrayIcon(HWND hWnd);
void                RemoveTrayIcon();
void                ShowTrayContextMenu(HWND hWnd);
void                PopulateAppProfileCombo(HWND hCombo);
void                RefreshAppProfileCombo(HWND hWnd);
void                ShowAddProfileDialog(HWND hWnd);
void                RemoveSelectedProfile(HWND hWnd);
void                UpdateActiveProfileSelection(HWND hWnd);
void                UpdateCurrentProfileLabel(HWND hWnd);
void                UpdateRemoveButtonState(HWND hWnd);
void                UpdateAppProfileColorBoxes(HWND hWnd);
void                ShowAppColorPicker(HWND hWnd, bool isHighlightColor);
void                UpdateLockKeysCheckbox(HWND hWnd);
void                ShowKeysDialog(HWND hWnd);
void                UpdateKeysButtonState(HWND hWnd);

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
    LoadLockKeyColorsFromRegistry();

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
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1); // Changed from COLOR_WINDOW to COLOR_BTNFACE
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
    static HBRUSH hBrushNum = NULL, hBrushCaps = NULL, hBrushScroll = NULL, hBrushDefault = NULL;
    static HBRUSH hBrushAppColor = NULL;
    static HBRUSH hBrushAppHighlightColor = NULL;
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
                        // Set color according to lock state and feature enabled state
                        if (IsLockKeysFeatureEnabled()) {
                            if ((GetKeyState(VK_NUMLOCK) & 0x0001) == 0x0001)
                                SetKeyColor(LogiLed::KeyName::NUM_LOCK, numLockColor);
                            else
                                SetKeyColor(LogiLed::KeyName::NUM_LOCK, defaultColor);
                        } else {
                            SetKeyColor(LogiLed::KeyName::NUM_LOCK, defaultColor);
                        }
                        SaveLockKeyColorsToRegistry();
                        break;
                    case IDC_BOX_CAPSLOCK:
                        // Show color picker for CapsLock
                        ShowColorPicker(hWnd, capsLockColor, LogiLed::KeyName::CAPS_LOCK);
                        if (hBrushCaps) DeleteObject(hBrushCaps);
                        hBrushCaps = CreateSolidBrush(capsLockColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_CAPSLOCK), NULL, TRUE);
                        // Set color according to lock state and feature enabled state
                        if (IsLockKeysFeatureEnabled()) {
                            if ((GetKeyState(VK_CAPITAL) & 0x0001) == 0x0001)
                                SetKeyColor(LogiLed::KeyName::CAPS_LOCK, capsLockColor);
                            else
                                SetKeyColor(LogiLed::KeyName::CAPS_LOCK, defaultColor);
                        } else {
                            SetKeyColor(LogiLed::KeyName::CAPS_LOCK, defaultColor);
                        }
                        SaveLockKeyColorsToRegistry();
                        break;
                    case IDC_BOX_SCROLLLOCK:
                        // Show color picker for ScrollLock
                        ShowColorPicker(hWnd, scrollLockColor, LogiLed::KeyName::SCROLL_LOCK);
                        if (hBrushScroll) DeleteObject(hBrushScroll);
                        hBrushScroll = CreateSolidBrush(scrollLockColor);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_SCROLLLOCK), NULL, TRUE);
                        // Set color according to lock state and feature enabled state
                        if (IsLockKeysFeatureEnabled()) {
                            if ((GetKeyState(VK_SCROLL) & 0x0001) == 0x0001)
                                SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, scrollLockColor);
                            else
                                SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, defaultColor);
                        } else {
                            SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, defaultColor);
                        }
                        SaveLockKeyColorsToRegistry();
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
                        SaveLockKeyColorsToRegistry();
                        break;
                    case IDM_ABOUT:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                        break;
                    case IDM_HELP:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPBOX), hWnd, Help);
                        break;
                    case IDM_START_MINIMIZED:
                        startMinimized = !startMinimized;
                        SaveStartMinimizedSetting(startMinimized);
                        break;
                    case IDM_IMPORT_PROFILE:
                        ImportProfileFromIniFile(hWnd);
                        break;
                    case IDM_EXPORT_PROFILES:
                        ExportAllProfilesToIniFiles();
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
                    case IDC_BUTTON_ADD_PROFILE:
                        ShowAddProfileDialog(hWnd);
                        break;
                    case IDC_BUTTON_REMOVE_PROFILE:
                        RemoveSelectedProfile(hWnd);
                        break;
                    case IDC_BUTTON_KEYS:
                        ShowKeysDialog(hWnd);
                        break;
                    case IDC_COMBO_APPPROFILE:
                        if (HIWORD(wParam) == CBN_SELCHANGE) {
                            UpdateRemoveButtonState(hWnd);
                            UpdateAppProfileColorBoxes(hWnd);
                            UpdateLockKeysCheckbox(hWnd);
                            UpdateKeysButtonState(hWnd);
                        }
                        break;
                    case IDC_BOX_APPCOLOR:
                        // Show color picker for App Color
                        ShowAppColorPicker(hWnd, false);
                        break;
                    case IDC_BOX_APPHIGHLIGHTCOLOR:
                        // Show color picker for App Highlight Color
                        ShowAppColorPicker(hWnd, true);
                        break;
                    case IDC_CHECK_LOCK_KEYS_VISUALISATION:
                        // Handle lock keys visualisation checkbox
                        {
                            HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                            HWND hCheckbox = GetDlgItem(hWnd, IDC_CHECK_LOCK_KEYS_VISUALISATION);
                            
                            if (hCombo && hCheckbox) {
                                int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                                if (selectedIndex > 0) { // Not "NONE"
                                    WCHAR appName[256];
                                    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                                    
                                    // Get checkbox state
                                    bool isChecked = (SendMessage(hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
                                    
                                    // Update the profile
                                    UpdateAppProfileLockKeysEnabled(appName, isChecked);
                                    UpdateAppProfileLockKeysEnabledInRegistry(appName, isChecked);
                                }
                            }
                        }
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
               
                // Existing color box handling...
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
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_DEFAULTCOLOR)) {
                    if (hBrushDefault) DeleteObject(hBrushDefault);
                    hBrushDefault = CreateSolidBrush(defaultColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)hBrushDefault;
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_APPCOLOR)) {
                    if (hBrushAppColor) DeleteObject(hBrushAppColor);
                    
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                    COLORREF appColor = RGB(128, 128, 128); // Default gray
                    
                    if (selectedIndex > 0) { // Not "NONE"
                        WCHAR appName[256];
                        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                        AppColorProfile* profile = GetAppProfileByName(appName);
                        if (profile) {
                            appColor = profile->appColor;
                        }
                    }
                    
                    hBrushAppColor = CreateSolidBrush(appColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    
                    // If NONE is selected, we'll draw the diagonal line in WM_PAINT
                    // This just provides the background color
                    return (LRESULT)hBrushAppColor;
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_APPHIGHLIGHTCOLOR)) {
                    if (hBrushAppHighlightColor) DeleteObject(hBrushAppHighlightColor);
                    
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                    COLORREF appHighlightColor = RGB(128, 128, 128); // Default gray
                    
                    if (selectedIndex > 0) { // Not "NONE"
                        WCHAR appName[256];
                        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                        AppColorProfile* profile = GetAppProfileByName(appName);
                        if (profile) {
                            appHighlightColor = profile->appHighlightColor;
                        }
                    }
                    
                    hBrushAppHighlightColor = CreateSolidBrush(appHighlightColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    
                    // If NONE is selected, we'll draw the diagonal line in WM_PAINT
                    // This just provides the background color
                    return (LRESULT)hBrushAppHighlightColor;
                }
            }
            break;
        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
                if (pDIS->CtlType == ODT_STATIC) {
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = hCombo ? (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0) : CB_ERR;
                    
                    if (pDIS->CtlID == IDC_BOX_APPCOLOR) {
                        COLORREF appColor = RGB(128, 128, 128); // Default gray
                        
                        if (selectedIndex > 0) { // Not "NONE"
                            WCHAR appName[256];
                            SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                            AppColorProfile* profile = GetAppProfileByName(appName);
                            if (profile) {
                                appColor = profile->appColor;
                            }
                        }
                        
                        // Fill the rectangle with the color
                        HBRUSH brush = CreateSolidBrush(appColor);
                        FillRect(pDIS->hDC, &pDIS->rcItem, brush);
                        DeleteObject(brush);
                        
                        // Draw red diagonal line if "NONE" is selected (deactivated state)
                        if (selectedIndex == 0 || selectedIndex == CB_ERR) {
                            HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0)); // Bold red pen
                            HPEN oldPen = (HPEN)SelectObject(pDIS->hDC, redPen);
                            
                            // Draw diagonal line from top-left to bottom-right
                            MoveToEx(pDIS->hDC, pDIS->rcItem.left + 2, pDIS->rcItem.top + 2, NULL);
                            LineTo(pDIS->hDC, pDIS->rcItem.right - 2, pDIS->rcItem.bottom - 2);
                            
                            SelectObject(pDIS->hDC, oldPen);
                            DeleteObject(redPen);
                        }
                        
                        return TRUE;
                    }
                    else if (pDIS->CtlID == IDC_BOX_APPHIGHLIGHTCOLOR) {
                        COLORREF appHighlightColor = RGB(128, 128, 128); // Default gray
                        
                        if (selectedIndex > 0) { // Not "NONE"
                            WCHAR appName[256];
                            SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                            AppColorProfile* profile = GetAppProfileByName(appName);
                            if (profile) {
                                appHighlightColor = profile->appHighlightColor;
                            }
                        }
                        
                        // Fill the rectangle with the color
                        HBRUSH brush = CreateSolidBrush(appHighlightColor);
                        FillRect(pDIS->hDC, &pDIS->rcItem, brush);
                        DeleteObject(brush);
                        
                        // Draw red diagonal line if "NONE" is selected (deactivated state)
                        if (selectedIndex == 0 || selectedIndex == CB_ERR) {
                            HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0)); // Bold red pen
                            HPEN oldPen = (HPEN)SelectObject(pDIS->hDC, redPen);
                            
                            // Draw diagonal line from top-left to bottom-right
                            MoveToEx(pDIS->hDC, pDIS->rcItem.left + 2, pDIS->rcItem.top + 2, NULL);
                            LineTo(pDIS->hDC, pDIS->rcItem.right - 2, pDIS->rcItem.bottom - 2);
                            
                            SelectObject(pDIS->hDC, oldPen);
                            DeleteObject(redPen);
                        }
                        
                        return TRUE;
                    }
                }
            }
            break;
        case WM_DESTROY:
            // Cleanup brushes and tray icon
            if (hBrushNum) DeleteObject(hBrushNum);
            if (hBrushCaps) DeleteObject(hBrushCaps);
            if (hBrushScroll) DeleteObject(hBrushScroll);
            if (hBrushDefault) DeleteObject(hBrushDefault);
            if (hBrushAppColor) DeleteObject(hBrushAppColor);
            if (hBrushAppHighlightColor) DeleteObject(hBrushAppHighlightColor);
            RemoveTrayIcon();
            CleanupAppMonitoring(); // Cleanup app monitoring before other cleanup
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
        case WM_UPDATE_PROFILE_COMBO: // Custom message to update profile combo box
            UpdateActiveProfileSelection(hWnd);
            UpdateCurrentProfileLabel(hWnd);
            UpdateRemoveButtonState(hWnd);
            UpdateAppProfileColorBoxes(hWnd);
            UpdateLockKeysCheckbox(hWnd);
            break;
        case WM_INITMENUPOPUP:
            // Update menu checkmarks when menu is about to be displayed
            {
                HMENU hMenu = (HMENU)wParam;
                if (hMenu) {
                    // Check/uncheck the "Start minimized" menu item
                    CheckMenuItem(hMenu, IDM_START_MINIMIZED, 
                        MF_BYCOMMAND | (startMinimized ? MF_CHECKED : MF_UNCHECKED));
                }
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
   // Fixed window size - increased height to accommodate new App Profile section
   HWND hWnd = CreateWindowW(szWindowClass, szTitle,
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, 0, 440, 460, nullptr, nullptr, hInstance, nullptr);
   if (!hWnd) return FALSE;

   // Group Box for lock keys
   HWND hGroup = CreateWindowW(L"BUTTON", L"Lock Keys Color", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
      20, 10, 300, 140, hWnd, (HMENU)IDC_GROUP_LOCKS, hInstance, nullptr);

   // Color Boxes and Labels for lock keys inside the Group Box
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 40, 30, 60, 60, hWnd, (HMENU)IDC_BOX_NUMLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 140, 30, 60, 60, hWnd, (HMENU)IDC_BOX_CAPSLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 240, 30, 60, 60, hWnd, (HMENU)IDC_BOX_SCROLLLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"NUM LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_NUMLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"CAPS LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_CAPSLOCK, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"SCROLL LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 240, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_SCROLLLOCK, hInstance, nullptr);

   // Default Color Box and Label for other keys (and lock keys when off)
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 340, 30, 60, 60, hWnd, (HMENU)IDC_BOX_DEFAULTCOLOR, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Default Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 340, 95, 60, 40, hWnd, (HMENU)IDC_LABEL_DEFAULTCOLOR, hInstance, nullptr);

   // Group Box for App Profiles
   HWND hAppGroup = CreateWindowW(L"BUTTON", L"App Profile", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 20, 160, 380, 220, hWnd, (HMENU)IDC_GROUP_APPPROFILE, hInstance, nullptr);
   // Current Profile Label
   CreateWindowW(L"STATIC", L"Profile in use: NONE", WS_VISIBLE | WS_CHILD, 40, 190, 300, 15, hWnd, (HMENU)IDC_LABEL_CURRENT_PROFILE, hInstance, nullptr);
   // Combo Box for App Profiles
   HWND hCombo = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 40, 220, 200, 200, hWnd, (HMENU)IDC_COMBO_APPPROFILE, hInstance, nullptr);
   // Add Profile Button
   CreateWindowW(L"BUTTON", L"+", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 250, 220, 30, 25, hWnd, (HMENU)IDC_BUTTON_ADD_PROFILE, hInstance, nullptr);
   // Remove Profile Button  
   CreateWindowW(L"BUTTON", L"-", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 290, 220, 30, 25, hWnd, (HMENU)IDC_BUTTON_REMOVE_PROFILE, hInstance, nullptr);
   // Keys Button
   CreateWindowW(L"BUTTON", L"Keys", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 330, 220, 50, 25, hWnd, (HMENU)IDC_BUTTON_KEYS, hInstance, nullptr);

   // App Color Box and Label
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY | SS_OWNERDRAW, 40, 260, 60, 60, hWnd, (HMENU)IDC_BOX_APPCOLOR, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"App Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 325, 60, 30, hWnd, (HMENU)IDC_LABEL_APPCOLOR, hInstance, nullptr);

   // App Highlight Color Box and Label
   CreateWindowW(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY | SS_OWNERDRAW, 140, 260, 60, 60, hWnd, (HMENU)IDC_BOX_APPHIGHLIGHTCOLOR, hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Highlight Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, 325, 60, 30, hWnd, (HMENU)IDC_LABEL_APPHIGHLIGHTCOLOR, hInstance, nullptr);

   // Lock Keys Visualisation Checkbox
   CreateWindowW(L"BUTTON", L"Lock Keys", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 240, 280, 100, 20, hWnd, (HMENU)IDC_CHECK_LOCK_KEYS_VISUALISATION, hInstance, nullptr);

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

   // Set main window handle for UI updates
   SetMainWindowHandle(hWnd);
   
   // Load app profiles from registry (includes highlight color and keys)
   LoadAppProfilesFromRegistry();

   /*
   // If no profiles exist, add some sample app color profiles with lock keys feature control
   if (GetAppProfilesCount() == 0) {
       AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0), true);      // Yellow for Notepad, lock keys enabled
       AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);      // Cyan for Chrome, lock keys disabled
       AddAppColorProfile(L"firefox.exe", RGB(255, 165, 0), false);     // Orange for Firefox, lock keys disabled  
       AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);           // Green for VS Code, lock keys enabled
       AddAppColorProfile(L"devenv.exe", RGB(128, 0, 128), true);       // Purple for Visual Studio, lock keys enabled
       SaveAppProfilesToRegistry();
   }
   */

   // Populate the combo box with app profiles
   PopulateAppProfileCombo(hCombo);
   
   // Check for already running apps and update colors
   CheckRunningAppsAndUpdateColors();
   
   // Update the combo box to reflect any displayed profiles after checking running apps
   UpdateActiveProfileSelection(hWnd);
   
   // Update the current profile label
   UpdateCurrentProfileLabel(hWnd);

   // Update the remove button state
   UpdateRemoveButtonState(hWnd);
   
   // Update app profile color boxes
   UpdateAppProfileColorBoxes(hWnd);
   
   // Update lock keys checkbox
   UpdateLockKeysCheckbox(hWnd);

   // Initialize app monitoring
   InitializeAppMonitoring();

   // Set up keyboard hook
   keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
   
   return TRUE;
}

// Populate app profile combo box with current profiles
void PopulateAppProfileCombo(HWND hCombo) {
    // Clear existing items
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    
    // Add "NONE" as the first item
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"NONE");
    
    // Get current profiles
    std::vector<AppColorProfile> profiles = GetAppColorProfilesCopy();
    
    int displayedProfileIndex = 0; // Default to "NONE" (index 0)
    
    // Add each profile to the combo box
    for (size_t i = 0; i < profiles.size(); i++) {
        const auto& profile = profiles[i];
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)profile.appName.c_str());
        
        // Check if this profile is currently displayed (controlling colors)
        // Use the FIRST displayed profile found (should only be one now)
        if (profile.isProfileCurrInUse && displayedProfileIndex == 0) {
            displayedProfileIndex = (int)i + 1; // +1 because of "NONE" at index 0
        }
    }
    
    // Select the displayed profile if found, otherwise select "NONE"
    SendMessage(hCombo, CB_SETCURSEL, displayedProfileIndex, 0);
}

// Refresh the app profile combo box
void RefreshAppProfileCombo(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (hCombo) {
        PopulateAppProfileCombo(hCombo);
        UpdateCurrentProfileLabel(hWnd);
        UpdateRemoveButtonState(hWnd);
        UpdateAppProfileColorBoxes(hWnd);
        UpdateLockKeysCheckbox(hWnd);
    }
}

// Show dialog to add a new app profile
void ShowAddProfileDialog(HWND hWnd) {
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDPROFILEBOX), hWnd, AddProfileDialog);
}

// Remove the selected app profile
void RemoveSelectedProfile(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR || selectedIndex == 0) { // Can't remove "NONE" 
        MessageBoxW(hWnd, L"No valid profile selected", L"Remove Profile", MB_OK);
        return;
    }
    
    // Get the selected app name
    WCHAR appName[256];
    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
    
    // Confirm removal
    std::wstring message = L"Remove profile for: " + std::wstring(appName) + L"?";
    if (MessageBoxW(hWnd, message.c_str(), L"Confirm Removal", MB_YESNO) == IDYES) {
        // Remove the profile from memory and registry
        RemoveAppColorProfile(appName);
        RemoveAppProfileFromRegistry(appName);
        
        // Refresh the combo box
        RefreshAppProfileCombo(hWnd);
    }
}

// Update combo box selection to reflect current displayed profile without repopulating
void UpdateActiveProfileSelection(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    std::vector<AppColorProfile> profiles = GetAppColorProfilesCopy();
    
    // Find the currently displayed profile (the one controlling colors)
    for (size_t i = 0; i < profiles.size(); i++) {
        if (profiles[i].isProfileCurrInUse) {
            // Select the displayed profile (index + 1 because of "NONE" at index 0)
            SendMessage(hCombo, CB_SETCURSEL, i + 1, 0);
            return;
        }
    }
    
    // If no profile is displayed, select "NONE"
    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
}

// Update the current profile label to show which profile is currently active
void UpdateCurrentProfileLabel(HWND hWnd) {
    HWND hLabel = GetDlgItem(hWnd, IDC_LABEL_CURRENT_PROFILE);
    if (!hLabel) return;
    
    std::vector<AppColorProfile> profiles = GetAppColorProfilesCopy();
    
    // Find the currently displayed profile (the one controlling colors)
    for (const auto& profile : profiles) {
        if (profile.isProfileCurrInUse) {
            std::wstring labelText = L"Profile in use: " + profile.appName;
            SetWindowTextW(hLabel, labelText.c_str());
            return;
        }
    }
    
    // If no profile is displayed, show "NONE"
    SetWindowTextW(hLabel, L"Profile in use: NONE");
}

// Update the remove button state based on current combo box selection
void UpdateRemoveButtonState(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    HWND hRemoveButton = GetDlgItem(hWnd, IDC_BUTTON_REMOVE_PROFILE);
    
    if (!hCombo || !hRemoveButton) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    
    // Disable the remove button if "NONE" is selected (index 0) or no selection
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        EnableWindow(hRemoveButton, FALSE);
    } else {
        EnableWindow(hRemoveButton, TRUE);
    }
    
    // Also update the Keys button state
    UpdateKeysButtonState(hWnd);
}

// Update the keys button state based on current combo box selection
void UpdateKeysButtonState(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    HWND hKeysButton = GetDlgItem(hWnd, IDC_BUTTON_KEYS);
    
    if (!hCombo || !hKeysButton) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    
    // Disable the keys button if "NONE" is selected (index 0) or no selection
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        EnableWindow(hKeysButton, FALSE);
    } else {
        EnableWindow(hKeysButton, TRUE);
    }
}

// Update app profile color boxes to show colors from selected profile
void UpdateAppProfileColorBoxes(HWND hWnd) {
    HWND hAppColorBox = GetDlgItem(hWnd, IDC_BOX_APPCOLOR);
    HWND hAppHighlightColorBox = GetDlgItem(hWnd, IDC_BOX_APPHIGHLIGHTCOLOR);
    
    if (hAppColorBox) {
        InvalidateRect(hAppColorBox, NULL, TRUE);
        UpdateWindow(hAppColorBox); // Force immediate redraw
    }
    if (hAppHighlightColorBox) {
        InvalidateRect(hAppHighlightColorBox, NULL, TRUE);
        UpdateWindow(hAppHighlightColorBox); // Force immediate redraw
    }
}

// Show color picker for app profile colors
void ShowAppColorPicker(HWND hWnd, bool isHighlightColor) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        // log to debug console
		OutputDebugStringW(L"No valid profile selected for color change.\n");
        return;
    }
    
    // Get the selected app name
    WCHAR appName[256];
    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
    
    // Get the current profile
    AppColorProfile* profile = GetAppProfileByName(appName);
    if (!profile) {
        MessageBoxW(hWnd, L"Profile not found", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Get current color
    COLORREF currentColor = isHighlightColor ? profile->appHighlightColor : profile->appColor;
    
    // Show color picker
    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.rgbResult = currentColor;
    COLORREF custColors[16] = {};
    cc.lpCustColors = custColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    
    if (ChooseColor(&cc)) {
        // Update the profile color
        if (isHighlightColor) {
            UpdateAppProfileHighlightColor(appName, cc.rgbResult);
            UpdateAppProfileHighlightColorInRegistry(appName, cc.rgbResult);
        } else {
            UpdateAppProfileColor(appName, cc.rgbResult);
            UpdateAppProfileColorInRegistry(appName, cc.rgbResult);
        }
        
        // Update the color boxes display
        UpdateAppProfileColorBoxes(hWnd);
    }
}

// Update the lock keys checkbox state based on current profile selection
void UpdateLockKeysCheckbox(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    HWND hCheckbox = GetDlgItem(hWnd, IDC_CHECK_LOCK_KEYS_VISUALISATION);
    
    if (!hCombo || !hCheckbox) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        // "NONE" selected - disable checkbox and uncheck it
        EnableWindow(hCheckbox, FALSE);
        SendMessage(hCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
    } else {
        // Profile selected - enable checkbox and set state based on profile
        EnableWindow(hCheckbox, TRUE);
        
        WCHAR appName[256];
        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
        AppColorProfile* profile = GetAppProfileByName(appName);
        
        if (profile) {
            SendMessage(hCheckbox, BM_SETCHECK, profile->lockKeysEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        } else {
            SendMessage(hCheckbox, BM_SETCHECK, BST_CHECKED, 0); // Default to checked
        }
    }
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

// Global variables for Keys dialog
static std::vector<LogiLed::KeyName> currentHighlightKeys;
static std::wstring currentAppNameForKeys;
static HHOOK keysDialogHook = nullptr;

// Low-level keyboard hook for the Keys dialog
LRESULT CALLBACK KeysDialogKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        // Convert virtual key to LogiLed key
        LogiLed::KeyName logiKey = VirtualKeyToLogiLedKey(pKeyStruct->vkCode);
        
        // Check if key is already in the list
        auto it = std::find(currentHighlightKeys.begin(), currentHighlightKeys.end(), logiKey);
        
        if (it != currentHighlightKeys.end()) {
            // Key is in list, remove it
            currentHighlightKeys.erase(it);
        } else {
            // Key is not in list, add it
            currentHighlightKeys.push_back(logiKey);
        }
        
        // Update the text field
        HWND hEditKeys = FindWindow(nullptr, L"Configure Highlight Keys");
        if (hEditKeys) {
            hEditKeys = GetDlgItem(hEditKeys, IDC_EDIT_KEYS);
            if (hEditKeys) {
                std::wstring keysText = FormatHighlightKeysForDisplay(currentHighlightKeys);
                SetWindowTextW(hEditKeys, keysText.c_str());
            }
        }
        
        // Prevent the key from being processed by other applications
        return 1;
    }
    
    return CallNextHookEx(keysDialogHook, nCode, wParam, lParam);
}

// Callback function for the Keys dialog box
INT_PTR CALLBACK KeysDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Get the app name and current keys from the selected profile
        HWND hMainWnd = GetParent(hDlg);
        HWND hCombo = GetDlgItem(hMainWnd, IDC_COMBO_APPPROFILE);
        
        if (hCombo) {
            int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            if (selectedIndex > 0) { // Not "NONE"
                WCHAR appName[256];
                SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                currentAppNameForKeys = appName;
                
                // Get current highlight keys for this profile
                AppColorProfile* profile = GetAppProfileByName(currentAppNameForKeys);
                if (profile) {
                    currentHighlightKeys = profile->highlightKeys;
                } else {
                    currentHighlightKeys.clear();
                }
                
                // Display current keys in the text field
                std::wstring keysText = FormatHighlightKeysForDisplay(currentHighlightKeys);
                SetDlgItemTextW(hDlg, IDC_EDIT_KEYS, keysText.c_str());
                
                // Set up keyboard hook to capture key presses
                keysDialogHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeysDialogKeyboardProc, GetModuleHandle(nullptr), 0);
            }
        }
        
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_RESET_KEYS:
            // Clear all highlight keys
            currentHighlightKeys.clear();
            SetDlgItemTextW(hDlg, IDC_EDIT_KEYS, L"");
            return (INT_PTR)TRUE;
            
        case IDC_BUTTON_DONE_KEYS:
        case IDOK:
            // Save the highlight keys and close dialog
            if (!currentAppNameForKeys.empty()) {
                UpdateAppProfileHighlightKeys(currentAppNameForKeys, currentHighlightKeys);
                UpdateAppProfileHighlightKeysInRegistry(currentAppNameForKeys, currentHighlightKeys);
            }
            
            // Remove keyboard hook
            if (keysDialogHook) {
                UnhookWindowsHookEx(keysDialogHook);
                keysDialogHook = nullptr;
            }
            
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            
        case IDCANCEL:
            // Remove keyboard hook
            if (keysDialogHook) {
                UnhookWindowsHookEx(keysDialogHook);
                keysDialogHook = nullptr;
            }
            
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
        
    case WM_CLOSE:
        // Remove keyboard hook
        if (keysDialogHook) {
            UnhookWindowsHookEx(keysDialogHook);
            keysDialogHook = nullptr;
        }
        
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// Show the Keys dialog
void ShowKeysDialog(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        MessageBoxW(hWnd, L"Please select an app profile first.", L"Keys Configuration", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    DialogBox(hInst, MAKEINTRESOURCE(IDD_KEYSBOX), hWnd, KeysDialog);
}

// Callback function for the Add Profile dialog box
INT_PTR CALLBACK AddProfileDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Get the combo box and populate it with running processes
            HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_APP_SELECTOR);
            if (hCombo) {
                // Clear the combo box first
                SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
                
                // Get visible running processes
                std::vector<std::wstring> processes = GetVisibleRunningProcesses();
                
                // Get existing profiles to filter out apps that already have profiles
                std::vector<AppColorProfile> existingProfiles = GetAppColorProfilesCopy();
                
                // Add processes to combo box, but filter out those that already have profiles
                for (const auto& process : processes) {
                    bool hasProfile = false;
                    
                    // Check if this process already has a profile (case-insensitive)
                    for (const auto& profile : existingProfiles) {
                        std::wstring lowerProcess = process;
                        std::wstring lowerProfileApp = profile.appName;
                        std::transform(lowerProcess.begin(), lowerProcess.end(), lowerProcess.begin(), ::towlower);
                        std::transform(lowerProfileApp.begin(), lowerProfileApp.end(), lowerProfileApp.begin(), ::towlower);
                        
                        if (lowerProcess == lowerProfileApp) {
                            hasProfile = true;
                            break;
                        }
                    }
                    
                    // Only add to combo box if it doesn't already have a profile
                    if (!hasProfile) {
                        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)process.c_str());
                    }
                }
                
                // Set focus to the combo box
                SetFocus(hCombo);
            }
            return (INT_PTR)TRUE;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_DONE_ADD_PROFILE:
        case IDOK:
            {
                // Get the text from the combo box
                HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_APP_SELECTOR);
                if (hCombo) {
                    WCHAR appName[256];
                    GetWindowTextW(hCombo, appName, sizeof(appName) / sizeof(WCHAR));
                    
                    // Check if something was entered
                    if (wcslen(appName) == 0) {
                        MessageBoxW(hDlg, L"Please enter an application name.", L"Add Profile", MB_OK | MB_ICONWARNING);
                        return (INT_PTR)TRUE;
                    }
                    
                    // Check if profile already exists (double-check since user can type manually)
                    std::vector<AppColorProfile> profiles = GetAppColorProfilesCopy();
                    bool exists = false;
                    for (const auto& profile : profiles) {
                        std::wstring lowerExisting = profile.appName;
                        std::wstring lowerNew = appName;
                        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
                        std::transform(lowerNew.begin(), lowerNew.end(), lowerNew.begin(), ::towlower);
                        
                        if (lowerExisting == lowerNew) {
                            exists = true;
                            break;
                        }
                    }
                    
                    if (exists) {
                        MessageBoxW(hDlg, L"Profile already exists for this application!", L"Add Profile", MB_OK | MB_ICONWARNING);
                        return (INT_PTR)TRUE;
                    }
                    
                    // Create the new profile with specified defaults
                    std::wstring newAppName = appName;
                    COLORREF appColor = RGB(0, 255, 255);      // Cyan
                    COLORREF highlightColor = RGB(255, 0, 0);  // Red
                    bool lockKeysEnabled = false;              // Lock keys feature off
                    
                    // Add the profile
                    AddAppColorProfile(newAppName, appColor, lockKeysEnabled);
                    
                    // Get the added profile, set highlight color and save to registry
                    AppColorProfile* newProfile = GetAppProfileByName(newAppName);
                    if (newProfile) {
                        newProfile->appHighlightColor = highlightColor;
                        // highlightKeys is already empty by default
                        AddAppProfileToRegistry(*newProfile);
                    }
                    
                    // Close the dialog and refresh the main window
                    EndDialog(hDlg, IDOK);
                    
                    // Refresh the combo box in the main window
                    HWND hParent = GetParent(hDlg);
                    if (hParent) {
                        RefreshAppProfileCombo(hParent);
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
        
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}
