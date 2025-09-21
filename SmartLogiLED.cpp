// SmartLogiLED - Advanced Logitech RGB Keyboard Controller
//
// This application controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock)
// and allows the user to customize colors via a GUI and tray icon.
//
// Copyright (c) 2025 Günter DANKL
// SPDX-License-Identifier: MIT
//
// SmartLogiLED.cpp : Defines the entry point for the application.

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_Config.h"
#include "SmartLogiLED_IniFiles.h"
#include "SmartLogiLED_LockKeys.h"
#include "SmartLogiLED_AppProfiles.h"
#include "SmartLogiLED_ProcessMonitor.h"
#include "SmartLogiLED_KeyMapping.h"
#include "SmartLogiLED_Version.h"
#include "SmartLogiLED_Dialogs.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <shellapi.h>
#include <commdlg.h>
#include <algorithm>
#include <vector>

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
COLORREF defaultColor = RGB(0, 89, 89); // default color used for all other keys and lock keys when off

// Version information functions
std::wstring GetApplicationVersion() {
    return SMARTLOGILED_VERSION_STRING;
}

std::wstring GetApplicationFullVersion() {
    return SMARTLOGILED_VERSION_FULL;
}

std::wstring GetApplicationName() {
    return SMARTLOGILED_PRODUCT_NAME;
}

DWORD GetVersionNumber() {
    return SMARTLOGILED_VERSION_NUMBER;
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                CreateTrayIcon(HWND hWnd);
void                RemoveTrayIcon();
void                ShowTrayContextMenu(HWND hWnd);
void                PopulateAppProfileCombo(HWND hCombo);
void                RefreshAppProfileCombo(HWND hWnd);
void                RemoveSelectedProfile(HWND hWnd);
void                UpdateActiveProfileSelection(HWND hWnd);
void                UpdateCurrentProfileLabel(HWND hWnd);
void                UpdateRemoveButtonState(HWND hWnd);
void                UpdateAppProfileColorBoxes(HWND hWnd);
void                UpdateLockKeysCheckbox(HWND hWnd);
void                UpdateKeysButtonState(HWND hWnd);
void                UpdateActionKeysButtonState(HWND hWnd);

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

    return static_cast<int>(msg.wParam);
}

// Registers the window class
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMARTLOGILED));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1); // Changed from COLOR_WINDOW to COLOR_BTNFACE
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
    
    // Create tooltip with version information
    std::wstring tooltip = SMARTLOGILED_PRODUCT_NAME;
    tooltip += L" v";
    tooltip += SMARTLOGILED_VERSION_STRING;
    wcscpy_s(nid.szTip, tooltip.c_str());
    
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
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING | (startMinimized ? MF_CHECKED : MF_UNCHECKED), 
                ID_TRAY_START_MINIMIZED, L"Start minimized");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_CLOSE, L"Close");
    SetForegroundWindow(hWnd); // Required for menu to disappear correctly
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

// Main window procedure (handles messages)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HBRUSH hBrushNum = nullptr, hBrushCaps = nullptr, hBrushScroll = nullptr, hBrushDefault = nullptr;
    static HBRUSH hBrushAppColor = nullptr;
    static HBRUSH hBrushAppHighlightColor = nullptr;
    static HBRUSH hBrushAppActionColor = nullptr;
    
    switch (message) {
        case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);
                switch (wmId) {
                    case IDC_BOX_NUMLOCK:
                        // Show color picker for NumLock
                        ShowColorPicker(hWnd, numLockColor, LogiLed::KeyName::NUM_LOCK);
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_NUMLOCK), nullptr, TRUE);
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
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_CAPSLOCK), nullptr, TRUE);
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
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_SCROLLLOCK), nullptr, TRUE);
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
                        InvalidateRect(GetDlgItem(hWnd, IDC_BOX_DEFAULTCOLOR), nullptr, TRUE);
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
                    case IDM_EXPORT_SELECTED_PROFILE:
                        ExportSelectedProfileToIniFile(hWnd);
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
                    case IDC_BUTTON_AKEYS:
                        ShowActionKeysDialog(hWnd);
                        break;
                    case IDC_COMBO_APPPROFILE:
                        if (HIWORD(wParam) == CBN_SELCHANGE) {
                            UpdateRemoveButtonState(hWnd);
                            UpdateAppProfileColorBoxes(hWnd);
                            UpdateLockKeysCheckbox(hWnd);
                            UpdateKeysButtonState(hWnd);
                            UpdateActionKeysButtonState(hWnd);
                        }
                        break;
                    case IDC_BOX_APPCOLOR:
                        // Show color picker for App Color
                        ShowAppColorPicker(hWnd, 0); // 0 for app color
                        break;
                    case IDC_BOX_APPHIGHLIGHTCOLOR:
                        // Show color picker for App Highlight Color
                        ShowAppColorPicker(hWnd, 1); // 1 for highlight color
                        break;
                    case IDC_BOX_APPACTIONCOLOR:
                        // Show color picker for App Action Color
                        ShowAppColorPicker(hWnd, 2); // 2 for action color
                        break;
                    case IDC_CHECK_LOCK_KEYS_VISUALISATION:
                        // Handle lock keys visualisation checkbox
                        {
                            HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                            HWND hCheckbox = GetDlgItem(hWnd, IDC_CHECK_LOCK_KEYS_VISUALISATION);
                            
                            if (hCombo && hCheckbox) {
                                int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
                                if (selectedIndex > 0) { // Not "NONE"
                                    WCHAR appName[256]{};
                                    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
                                    
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
                HDC hdcStatic = reinterpret_cast<HDC>(wParam);
                HWND hCtrl = reinterpret_cast<HWND>(lParam);
               
                // Existing color box handling...
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_NUMLOCK)) {
                    if (hBrushNum) DeleteObject(hBrushNum);
                    hBrushNum = CreateSolidBrush(numLockColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return reinterpret_cast<LRESULT>(hBrushNum);
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_CAPSLOCK)) {
                    if (hBrushCaps) DeleteObject(hBrushCaps);
                    hBrushCaps = CreateSolidBrush(capsLockColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return reinterpret_cast<LRESULT>(hBrushCaps);
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_SCROLLLOCK)) {
                    if (hBrushScroll) DeleteObject(hBrushScroll);
                    hBrushScroll = CreateSolidBrush(scrollLockColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return reinterpret_cast<LRESULT>(hBrushScroll);
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_DEFAULTCOLOR)) {
                    if (hBrushDefault) DeleteObject(hBrushDefault);
                    hBrushDefault = CreateSolidBrush(defaultColor);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return reinterpret_cast<LRESULT>(hBrushDefault);
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_APPCOLOR)) {
                    HBRUSH hOldBrush = hBrushAppColor;
                    
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
                    COLORREF appColor = RGB(128, 128, 128); // Default gray
                    
                    if (selectedIndex > 0) { // Not "NONE"
                        WCHAR appName[256]{};
                        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
                        AppColorProfile* profile = GetAppProfileByName(appName);
                        if (profile) {
                            appColor = profile->appColor;
                        }
                    }
                    
                    hBrushAppColor = CreateSolidBrush(appColor);
                    if (hOldBrush) DeleteObject(hOldBrush);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    
                    // If NONE is selected, we'll draw the diagonal line in WM_PAINT
                    // This just provides the background color
                    return reinterpret_cast<LRESULT>(hBrushAppColor);
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_APPHIGHLIGHTCOLOR)) {
                    HBRUSH hOldBrush = hBrushAppHighlightColor;
                    
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
                    COLORREF appHighlightColor = RGB(128, 128, 128); // Default gray
                    
                    if (selectedIndex > 0) { // Not "NONE"
                        WCHAR appName[256]{};
                        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
                        AppColorProfile* profile = GetAppProfileByName(appName);
                        if (profile) {
                            appHighlightColor = profile->appHighlightColor;
                        }
                    }
                    
                    hBrushAppHighlightColor = CreateSolidBrush(appHighlightColor);
                    if (hOldBrush) DeleteObject(hOldBrush);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    
                    // If NONE is selected, we'll draw the diagonal line in WM_PAINT
                    // This just provides the background color
                    return reinterpret_cast<LRESULT>(hBrushAppHighlightColor);
                }
                if (hCtrl == GetDlgItem(hWnd, IDC_BOX_APPACTIONCOLOR)) {
                    HBRUSH hOldBrush = hBrushAppActionColor;
                    
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
                    COLORREF appActionColor = RGB(128, 128, 128); // Default gray
                    
                    if (selectedIndex > 0) { // Not "NONE"
                        WCHAR appName[256]{};
                        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
                        AppColorProfile* profile = GetAppProfileByName(appName);
                        if (profile) {
                            appActionColor = profile->appActionColor;
                        }
                    }
                    
                    hBrushAppActionColor = CreateSolidBrush(appActionColor);
                    if (hOldBrush) DeleteObject(hOldBrush);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    
                    // If NONE is selected, we'll draw the diagonal line in WM_PAINT
                    // This just provides the background color
                    return reinterpret_cast<LRESULT>(hBrushAppActionColor);
                }
            }
            break;
        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT* pDIS = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (pDIS->CtlType == ODT_STATIC) {
                    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
                    int selectedIndex = hCombo ? static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0)) : CB_ERR;
                    
                    if (pDIS->CtlID == IDC_BOX_APPCOLOR) {
                        COLORREF appColor = RGB(128, 128, 128); // Default gray
                        
                        if (selectedIndex > 0) { // Not "NONE"
                            WCHAR appName[256]{};
                            SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
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
                            HPEN oldPen = static_cast<HPEN>(SelectObject(pDIS->hDC, redPen));
                            
                            // Draw diagonal line from top-left to bottom-right
                            MoveToEx(pDIS->hDC, pDIS->rcItem.left + 2, pDIS->rcItem.top + 2, nullptr);
                            LineTo(pDIS->hDC, pDIS->rcItem.right - 2, pDIS->rcItem.bottom - 2);
                            
                            SelectObject(pDIS->hDC, oldPen);
                            DeleteObject(redPen);
                        }
                        
                        return TRUE;
                    }
                    else if (pDIS->CtlID == IDC_BOX_APPHIGHLIGHTCOLOR) {
                        COLORREF appHighlightColor = RGB(128, 128, 128); // Default gray
                        
                        if (selectedIndex > 0) { // Not "NONE"
                            WCHAR appName[256]{};
                            SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
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
                            HPEN oldPen = static_cast<HPEN>(SelectObject(pDIS->hDC, redPen));
                            
                            // Draw diagonal line from top-left to bottom-right
                            MoveToEx(pDIS->hDC, pDIS->rcItem.left + 2, pDIS->rcItem.top + 2, nullptr);
                            LineTo(pDIS->hDC, pDIS->rcItem.right - 2, pDIS->rcItem.bottom - 2);
                            
                            SelectObject(pDIS->hDC, oldPen);
                            DeleteObject(redPen);
                        }
                        
                        return TRUE;
                    }
                    else if (pDIS->CtlID == IDC_BOX_APPACTIONCOLOR) {
                        COLORREF appActionColor = RGB(128, 128, 128); // Default gray
                        
                        if (selectedIndex > 0) { // Not "NONE"
                            WCHAR appName[256]{};
                            SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
                            AppColorProfile* profile = GetAppProfileByName(appName);
                            if (profile) {
                                appActionColor = profile->appActionColor;
                            }
                        }
                        
                        // Fill the rectangle with the color
                        HBRUSH brush = CreateSolidBrush(appActionColor);
                        FillRect(pDIS->hDC, &pDIS->rcItem, brush);
                        DeleteObject(brush);
                        
                        // Draw red diagonal line if "NONE" is selected (deactivated state)
                        if (selectedIndex == 0 || selectedIndex == CB_ERR) {
                            HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0)); // Bold red pen
                            HPEN oldPen = static_cast<HPEN>(SelectObject(pDIS->hDC, redPen));
                            
                            // Draw diagonal line from top-left to bottom-right
                            MoveToEx(pDIS->hDC, pDIS->rcItem.left + 2, pDIS->rcItem.top + 2, nullptr);
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
            if (hBrushAppActionColor) DeleteObject(hBrushAppActionColor);
            RemoveTrayIcon();
            CleanupAppMonitoring(); // Cleanup app monitoring before other cleanup
            DisableKeyboardHook(); // Use managed hook cleanup
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
        case WM_LOCK_KEY_PRESSED: // Custom message for lock key pressed
            {
                DWORD vkCode = static_cast<DWORD>(wParam);
                DWORD vkState = static_cast<DWORD>(lParam); // Get the new key state from lParam
                HandleLockKeyPressed(vkCode, vkState);
            }
            break;
        case WM_UPDATE_PROFILE_COMBO: // Custom message to update profile combo box
            UpdateActiveProfileSelection(hWnd);
            UpdateCurrentProfileLabel(hWnd);
            UpdateRemoveButtonState(hWnd);
            UpdateAppProfileColorBoxes(hWnd);
            UpdateLockKeysCheckbox(hWnd);
            UpdateKeysButtonState(hWnd);
            UpdateActionKeysButtonState(hWnd);
            break;
        case WM_APP_STARTED: // Custom message for app started
            {
                // Get app name from message parameter (allocated on heap by thread)
                std::wstring* appName = reinterpret_cast<std::wstring*>(lParam);
                if (appName) {
                    HandleAppStarted(*appName);
                    delete appName; // Clean up heap allocated string
                }
            }
            break;
        case WM_APP_STOPPED: // Custom message for app stopped
            {
                // Get app name from message parameter (allocated on heap by thread)
                std::wstring* appName = reinterpret_cast<std::wstring*>(lParam);
                if (appName) {
                    HandleAppStopped(*appName);
                    delete appName; // Clean up heap allocated string
                }
            }
            break;
        case WM_INITMENUPOPUP:
            // Update menu checkmarks when menu is about to be displayed
            {
                HMENU hMenu = reinterpret_cast<HMENU>(wParam);
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
   CreateWindowW(L"BUTTON", L"Lock Keys Color", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
      20, 10, 300, 140, hWnd, reinterpret_cast<HMENU>(IDC_GROUP_LOCKS), hInstance, nullptr);

   // Color Boxes and Labels for lock keys inside the Group Box
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 40, 30, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_NUMLOCK), hInstance, nullptr);
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 140, 30, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_CAPSLOCK), hInstance, nullptr);
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 240, 30, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_SCROLLLOCK), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"NUM LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 95, 60, 40, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_NUMLOCK), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"CAPS LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, 95, 60, 40, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_CAPSLOCK), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"SCROLL LOCK", WS_VISIBLE | WS_CHILD | SS_CENTER, 240, 95, 60, 40, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_SCROLLLOCK), hInstance, nullptr);

   // Default Color Box and Label for other keys (and lock keys when off)
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY, 340, 30, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_DEFAULTCOLOR), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Default Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 340, 95, 60, 40, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_DEFAULTCOLOR), hInstance, nullptr);

   // Group Box for App Profiles
   CreateWindowW(L"BUTTON", L"App Profile", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 20, 160, 380, 220, hWnd, reinterpret_cast<HMENU>(IDC_GROUP_APPPROFILE), hInstance, nullptr);
   // Current Profile Label
   CreateWindowW(L"STATIC", L"Profile in use: NONE", WS_VISIBLE | WS_CHILD, 40, 190, 210, 15, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_CURRENT_PROFILE), hInstance, nullptr);
   // Combo Box for App Profiles
   HWND hCombo = CreateWindowW(L"COMBOBOX", nullptr, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 40, 220, 200, 200, hWnd, reinterpret_cast<HMENU>(IDC_COMBO_APPPROFILE), hInstance, nullptr);
   // Add Profile Button
   CreateWindowW(L"BUTTON", L"+", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 250, 220, 30, 25, hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_ADD_PROFILE), hInstance, nullptr);
   // Remove Profile Button  
   CreateWindowW(L"BUTTON", L"-", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 290, 220, 30, 25, hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_REMOVE_PROFILE), hInstance, nullptr);
   // Keys Button
   CreateWindowW(L"BUTTON", L"H-Keys", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 325, 260, 55, 25, hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_KEYS), hInstance, nullptr);
   // A-Keys Button
   CreateWindowW(L"BUTTON", L"A-Keys", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 325, 300, 55, 25, hWnd, reinterpret_cast<HMENU>(IDC_BUTTON_AKEYS), hInstance, nullptr);

   // App Color Box and Label
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY | SS_OWNERDRAW, 40, 260, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_APPCOLOR), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"App Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 40, 325, 60, 30, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_APPCOLOR), hInstance, nullptr);

   // App Highlight Color Box and Label
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY | SS_OWNERDRAW, 140, 260, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_APPHIGHLIGHTCOLOR), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Highlight Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, 325, 60, 30, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_APPHIGHLIGHTCOLOR), hInstance, nullptr);

   // App Action Color Box and Label
   CreateWindowW(L"STATIC", nullptr, WS_VISIBLE | WS_CHILD | SS_NOTIFY | SS_OWNERDRAW, 240, 260, 60, 60, hWnd, reinterpret_cast<HMENU>(IDC_BOX_APPACTIONCOLOR), hInstance, nullptr);
   CreateWindowW(L"STATIC", L"Action Color", WS_VISIBLE | WS_CHILD | SS_CENTER, 240, 325, 60, 30, hWnd, reinterpret_cast<HMENU>(IDC_LABEL_APPACTIONCOLOR), hInstance, nullptr);

   // Lock Keys Visualisation Checkbox
   CreateWindowW(L"BUTTON", L"Lock Keys", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 260, 190, 100, 20, hWnd, reinterpret_cast<HMENU>(IDC_CHECK_LOCK_KEYS_VISUALISATION), hInstance, nullptr);

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
       return FALSE; // Changed from return 0 to return FALSE for consistency
   } else if (!LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB)) {
       MessageBox(hWnd, L"Couldn't set LOGI_DEVICETYPE_PERKEY_RGB mode", L"ERROR", 0);
       return FALSE; // Changed from return 0 to return FALSE for consistency
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
   InitializeAppMonitoring(hWnd);

   // Initialize keyboard hook based on lock keys feature status
   UpdateKeyboardHookStateUnsafe();
   
   return TRUE;
}

// Populate app profile combo box with current profiles
void PopulateAppProfileCombo(HWND hCombo) {
    // Clear existing items
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    
    // Add "NONE" as the first item
    SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"NONE"));
    
    // Get current profiles
    std::vector<AppColorProfile> profiles = GetAppColorProfilesCopy();
    
    int displayedProfileIndex = 0; // Default to "NONE" (index 0)
    
    // Add each profile to the combo box
    for (size_t i = 0; i < profiles.size(); i++) {
        const auto& profile = profiles[i];
        SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(profile.appName.c_str()));
        
        // Check if this profile is currently displayed (controlling colors)
        // Use the FIRST displayed profile found (should only be one now)
        if (profile.isProfileCurrInUse && displayedProfileIndex == 0) {
            displayedProfileIndex = static_cast<int>(i) + 1; // +1 because of "NONE" at index 0
        }
    }
    
    // Select the displayed profile if found, otherwise select "NONE"
    SendMessage(hCombo, CB_SETCURSEL, displayedProfileIndex, 0);
}

// Remove the selected app profile
void RemoveSelectedProfile(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
    if (selectedIndex == CB_ERR || selectedIndex == 0) { // Can't remove "NONE" 
        MessageBoxW(hWnd, L"No valid profile selected", L"Remove Profile", MB_OK);
        return;
    }
    
    // Get the selected app name
    WCHAR appName[256]{};
    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
    
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
            SendMessage(hCombo, CB_SETCURSEL, static_cast<int>(i) + 1, 0);
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
    
    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
    
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
    
    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
    
    // Disable the keys button if "NONE" is selected (index 0) or no selection
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        EnableWindow(hKeysButton, FALSE);
    } else {
        EnableWindow(hKeysButton, TRUE);
    }
}

// Update the action keys button state based on current combo box selection
void UpdateActionKeysButtonState(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    HWND hAKeysButton = GetDlgItem(hWnd, IDC_BUTTON_AKEYS);
    
    if (!hCombo || !hAKeysButton) return;
    
    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
    
    // Disable the A-Keys button if "NONE" is selected (index 0) or no selection
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        EnableWindow(hAKeysButton, FALSE);
    } else {
        EnableWindow(hAKeysButton, TRUE);
    }
}

// Update app profile color boxes to show colors from selected profile
void UpdateAppProfileColorBoxes(HWND hWnd) {
    HWND hAppColorBox = GetDlgItem(hWnd, IDC_BOX_APPCOLOR);
    HWND hAppHighlightColorBox = GetDlgItem(hWnd, IDC_BOX_APPHIGHLIGHTCOLOR);
    HWND hAppActionColorBox = GetDlgItem(hWnd, IDC_BOX_APPACTIONCOLOR);
    
    if (hAppColorBox) {
        InvalidateRect(hAppColorBox, nullptr, TRUE);
        UpdateWindow(hAppColorBox); // Force immediate redraw
    }
    if (hAppHighlightColorBox) {
        InvalidateRect(hAppHighlightColorBox, nullptr, TRUE);
        UpdateWindow(hAppHighlightColorBox); // Force immediate redraw
    }
    if (hAppActionColorBox) {
        InvalidateRect(hAppActionColorBox, nullptr, TRUE);
        UpdateWindow(hAppActionColorBox); // Force immediate redraw
    }
}

// Update the lock keys checkbox state based on current profile selection
void UpdateLockKeysCheckbox(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    HWND hCheckbox = GetDlgItem(hWnd, IDC_CHECK_LOCK_KEYS_VISUALISATION);
    
    if (!hCombo || !hCheckbox) return;
    
    int selectedIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
    
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        // "NONE" selected - disable checkbox and uncheck it
        EnableWindow(hCheckbox, FALSE);
        SendMessage(hCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
    } else {
        // Profile selected - enable checkbox and set state based on profile
        EnableWindow(hCheckbox, TRUE);
        
        WCHAR appName[256]{};
        SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(appName));
        AppColorProfile* profile = GetAppProfileByName(appName);
        
        if (profile) {
            SendMessage(hCheckbox, BM_SETCHECK, profile->lockKeysEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        } else {
            SendMessage(hCheckbox, BM_SETCHECK, BST_CHECKED, 0); // Default to checked
        }
    }
}

        