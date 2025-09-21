// SmartLogiLED_Dialogs.cpp : Contains all dialog functionality for SmartLogiLED application.
//
// This file contains:
// - About and Help dialog procedures
// - Keys configuration dialog for highlight keys
// - Action Keys configuration dialog
// - Add Profile dialog for creating new app profiles
// - Dialog utility functions and keyboard hooks

#include "framework.h"
#include "SmartLogiLED_Dialogs.h"
#include "SmartLogiLED_AppProfiles.h"
#include "SmartLogiLED_KeyMapping.h"
#include "SmartLogiLED_ProcessMonitor.h"
#include "SmartLogiLED_Config.h"
#include "SmartLogiLED_Constants.h"
#include "Resource.h"
#include <commdlg.h>
#include <algorithm>
#include <vector>

// External variables
extern HINSTANCE hInst;

// Global variables for Keys dialog
static std::vector<LogiLed::KeyName> currentHighlightKeys;
static std::wstring currentAppNameForKeys;
static HHOOK keysDialogHook = nullptr;

// Global variables for Action Keys dialog
static std::vector<LogiLed::KeyName> currentActionKeys;
static std::wstring currentAppNameForActionKeys;
static HHOOK actionKeysDialogHook = nullptr;

// Forward declarations for main window UI functions (implemented in main file)
extern void PopulateAppProfileCombo(HWND hCombo);
extern void UpdateCurrentProfileLabel(HWND hWnd);
extern void UpdateRemoveButtonState(HWND hWnd);
extern void UpdateAppProfileColorBoxes(HWND hWnd);
extern void UpdateLockKeysCheckbox(HWND hWnd);

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
            MAKEINTRESOURCE(IDI_SMARTLOGILED),
            IMAGE_ICON,
            128, 128,
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
                WCHAR appName[256]{};
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

// Low-level keyboard hook for the Action Keys dialog
LRESULT CALLBACK ActionKeysDialogKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        // Convert virtual key to LogiLed key
        LogiLed::KeyName logiKey = VirtualKeyToLogiLedKey(pKeyStruct->vkCode);
        
        // Check if key is already in the list
        auto it = std::find(currentActionKeys.begin(), currentActionKeys.end(), logiKey);
        
        if (it != currentActionKeys.end()) {
            // Key is in list, remove it
            currentActionKeys.erase(it);
        } else {
            // Key is not in list, add it
            currentActionKeys.push_back(logiKey);
        }
        
        // Update the text field
        HWND hEditKeys = FindWindow(nullptr, L"Configure Action Keys");
        if (hEditKeys) {
            hEditKeys = GetDlgItem(hEditKeys, IDC_EDIT_KEYS);
            if (hEditKeys) {
                std::wstring keysText = FormatHighlightKeysForDisplay(currentActionKeys);
                SetWindowTextW(hEditKeys, keysText.c_str());
            }
        }
        
        // Prevent the key from being processed by other applications
        return 1;
    }
    
    return CallNextHookEx(actionKeysDialogHook, nCode, wParam, lParam);
}

// Callback function for the Action Keys dialog box
INT_PTR CALLBACK ActionKeysDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Set the dialog title
        SetWindowTextW(hDlg, L"Configure Action Keys");
        
        // Get the app name and current keys from the selected profile
        HWND hMainWnd = GetParent(hDlg);
        HWND hCombo = GetDlgItem(hMainWnd, IDC_COMBO_APPPROFILE);
        
        if (hCombo) {
            int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            if (selectedIndex > 0) { // Not "NONE"
                WCHAR appName[256]{};
                SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
                currentAppNameForActionKeys = appName;
                
                // Get current action keys for this profile
                AppColorProfile* profile = GetAppProfileByName(currentAppNameForActionKeys);
                if (profile) {
                    currentActionKeys = profile->actionKeys;
                } else {
                    currentActionKeys.clear();
                }
                
                // Display current keys in the text field
                std::wstring keysText = FormatHighlightKeysForDisplay(currentActionKeys);
                SetDlgItemTextW(hDlg, IDC_EDIT_KEYS, keysText.c_str());
                
                // Set up keyboard hook to capture key presses
                actionKeysDialogHook = SetWindowsHookEx(WH_KEYBOARD_LL, ActionKeysDialogKeyboardProc, GetModuleHandle(nullptr), 0);
            }
        }
        
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_RESET_KEYS:
            // Clear all action keys
            currentActionKeys.clear();
            SetDlgItemTextW(hDlg, IDC_EDIT_KEYS, L"");
            return (INT_PTR)TRUE;
            
        case IDC_BUTTON_DONE_KEYS:
        case IDOK:
            // Save the action keys and close dialog
            if (!currentAppNameForActionKeys.empty()) {
                UpdateAppProfileActionKeys(currentAppNameForActionKeys, currentActionKeys);
                UpdateAppProfileActionKeysInRegistry(currentAppNameForActionKeys, currentActionKeys);
            }
            
            // Remove keyboard hook
            if (actionKeysDialogHook) {
                UnhookWindowsHookEx(actionKeysDialogHook);
                actionKeysDialogHook = nullptr;
            }
            
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            
        case IDCANCEL:
            // Remove keyboard hook
            if (actionKeysDialogHook) {
                UnhookWindowsHookEx(actionKeysDialogHook);
                actionKeysDialogHook = nullptr;
            }
            
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
        
    case WM_CLOSE:
        // Remove keyboard hook
        if (actionKeysDialogHook) {
            UnhookWindowsHookEx(actionKeysDialogHook);
            actionKeysDialogHook = nullptr;
        }
        
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
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
                    WCHAR appName[256]{};
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
                    COLORREF actionColor = RGB(255, 255, 0);   // Yellow
                    bool lockKeysEnabled = false;              // Lock keys feature off
                    
                    // Add the profile
                    AddAppColorProfile(newAppName, appColor, lockKeysEnabled);
                    
                    // Get the added profile, set highlight color, action color and save to registry
                    AppColorProfile* newProfile = GetAppProfileByName(newAppName);
                    if (newProfile) {
                        newProfile->appHighlightColor = highlightColor;
                        newProfile->appActionColor = actionColor;
                        // highlightKeys and actionKeys are already empty by default
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

// Show the Action Keys dialog
void ShowActionKeysDialog(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        MessageBoxW(hWnd, L"Please select an app profile first.", L"Action Keys Configuration", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    DialogBox(hInst, MAKEINTRESOURCE(IDD_KEYSBOX), hWnd, ActionKeysDialog);
}

// Show dialog to add a new app profile
void ShowAddProfileDialog(HWND hWnd) {
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDPROFILEBOX), hWnd, AddProfileDialog);
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

// Show color picker for app profile colors
void ShowAppColorPicker(HWND hWnd, int colorType) {
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) return;
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
#ifdef ENABLE_DEBUG_LOGGING
        // log to debug console
        OutputDebugStringW(L"No valid profile selected for color change.\n");
#endif
        return;
    }
    
    // Get the selected app name
    WCHAR appName[256]{};
    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
    
    // Get the current profile
    AppColorProfile* profile = GetAppProfileByName(appName);
    if (!profile) {
        MessageBoxW(hWnd, L"Profile not found", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Get current color based on type: 0 = app color, 1 = highlight color, 2 = action color
    COLORREF currentColor;
    switch (colorType) {
        case 1: // Highlight color
            currentColor = profile->appHighlightColor;
            break;
        case 2: // Action color
            currentColor = profile->appActionColor;
            break;
        default: // App color
            currentColor = profile->appColor;
            break;
    }
    
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
        // Update the profile color based on type
        switch (colorType) {
            case 1: // Highlight color
                UpdateAppProfileHighlightColor(appName, cc.rgbResult);
                UpdateAppProfileHighlightColorInRegistry(appName, cc.rgbResult);
                break;
            case 2: // Action color
                UpdateAppProfileActionColor(appName, cc.rgbResult);
                UpdateAppProfileActionColorInRegistry(appName, cc.rgbResult);
                break;
            default: // App color
                UpdateAppProfileColor(appName, cc.rgbResult);
                UpdateAppProfileColorInRegistry(appName, cc.rgbResult);
                break;
        }
        
        // Update the color boxes display
        UpdateAppProfileColorBoxes(hWnd);
    }
}