// SmartLogiLED_Logic.cpp : Contains the logic functions for SmartLogiLED application.
//
// This file contains color management, keyboard hook logic, and app monitoring.

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_Logic.h"
#include "SmartLogiLED_Config.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <commdlg.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

// External variables from main file
extern COLORREF capsLockColor;
extern COLORREF scrollLockColor; 
extern COLORREF numLockColor;
extern COLORREF defaultColor;
extern HHOOK keyboardHook;

// App monitoring variables
std::vector<AppColorProfile> appColorProfiles;
std::mutex appProfilesMutex;
static std::thread appMonitorThread;
static bool appMonitoringRunning = false;
static HWND mainWindowHandle = nullptr;

// Set color for a key using Logitech LED SDK
void SetKeyColor(LogiLed::KeyName key, COLORREF color) {
    int r = GetRValue(color) * 100 / 255;
    int g = GetGValue(color) * 100 / 255;
    int b = GetBValue(color) * 100 / 255;
    LogiLedSetLightingForKeyWithKeyName(key, r, g, b);
}

// Set color for all keys
void SetDefaultColor(COLORREF color) {
    int r = GetRValue(color) * 100 / 255;
    int g = GetGValue(color) * 100 / 255;
    int b = GetBValue(color) * 100 / 255;
    LogiLedSetLighting(r, g, b);
}

// Set color for lock keys depending on their state (only if lock keys feature is enabled)
void SetLockKeysColor(void) {
    SHORT keyState;
    LogiLed::KeyName pressedKey;
    COLORREF colorToSet;

    // NumLock
    keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    pressedKey = LogiLed::KeyName::NUM_LOCK;
    colorToSet = (keyState == 0x0001) ? numLockColor : defaultColor;
    SetKeyColor(pressedKey, colorToSet);

    // ScrollLock
    keyState = GetKeyState(VK_SCROLL) & 0x0001;
    pressedKey = LogiLed::KeyName::SCROLL_LOCK;
    colorToSet = (keyState == 0x0001) ? scrollLockColor : defaultColor;
    SetKeyColor(pressedKey, colorToSet);

    // CapsLock
    keyState = GetKeyState(VK_CAPITAL) & 0x0001;
    pressedKey = LogiLed::KeyName::CAPS_LOCK;
    colorToSet = (keyState == 0x0001) ? capsLockColor : defaultColor;
    SetKeyColor(pressedKey, colorToSet);
}

// Show color picker dialog and update color
void ShowColorPicker(HWND hWnd, COLORREF& color, LogiLed::KeyName key) {
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

// Keyboard hook procedure to update lock key colors on key press
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;

        // Check if lock key was pressed and if lock keys feature is enabled
        if ((pKeyStruct->vkCode == VK_NUMLOCK) || (pKeyStruct->vkCode == VK_SCROLL) || (pKeyStruct->vkCode == VK_CAPITAL)) {
            // If lock keys feature is disabled, just set the key to default color
            if (!IsLockKeysFeatureEnabled()) {
                LogiLed::KeyName pressedKey;
                switch (pKeyStruct->vkCode) {
                case VK_NUMLOCK:
                    pressedKey = LogiLed::KeyName::NUM_LOCK;
                    break;
                case VK_SCROLL:
                    pressedKey = LogiLed::KeyName::SCROLL_LOCK;
                    break;
                case VK_CAPITAL:
                    pressedKey = LogiLed::KeyName::CAPS_LOCK;
                    break;
                }
                SetKeyColor(pressedKey, defaultColor);
                return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
            }

            SHORT keyState;
            LogiLed::KeyName pressedKey;
            COLORREF colorToSet;

            // The lock key state is updated only after this callback, so current state off means the key will be turned on
            switch (pKeyStruct->vkCode) {
            case VK_NUMLOCK:
                keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
                pressedKey = LogiLed::KeyName::NUM_LOCK;
                colorToSet = (keyState == 0x0000) ? numLockColor : defaultColor;
                break;
            case VK_SCROLL:
                keyState = GetKeyState(VK_SCROLL) & 0x0001;
                pressedKey = LogiLed::KeyName::SCROLL_LOCK;
                colorToSet = (keyState == 0x0000) ? scrollLockColor : defaultColor;
                break;
            case VK_CAPITAL:
                keyState = GetKeyState(VK_CAPITAL) & 0x0001;
                pressedKey = LogiLed::KeyName::CAPS_LOCK;
                colorToSet = (keyState == 0x0000) ? capsLockColor : defaultColor;
                break;
            }

            SetKeyColor(pressedKey, colorToSet);
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// App monitoring functions

// Check if a process has visible windows
bool IsProcessVisible(DWORD processId) {
    struct EnumData {
        DWORD processId;
        bool hasVisibleWindow;
    };
    
    EnumData enumData = { processId, false };
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* data = reinterpret_cast<EnumData*>(lParam);
        
        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);
        
        if (windowProcessId == data->processId) {
            // Check if window is visible and not minimized
            if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
                // Check if it's a main window (has no owner)
                if (GetWindow(hwnd, GW_OWNER) == NULL) {
                    // Additional check: window should have a title or be a main application window
                    WCHAR windowTitle[256];
                    if (GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR)) > 0) {
                        data->hasVisibleWindow = true;
                        return FALSE; // Stop enumeration
                    }
                }
            }
        }
        return TRUE; // Continue enumeration
    }, reinterpret_cast<LPARAM>(&enumData));
    
    return enumData.hasVisibleWindow;
}

// Get list of currently running processes
std::vector<std::wstring> GetRunningProcesses() {
    std::vector<std::wstring> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                processes.push_back(std::wstring(pe32.szExeFile));
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    
    return processes;
}

// Get list of currently running visible processes
std::vector<std::wstring> GetVisibleRunningProcesses() {
    std::vector<std::wstring> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                // Check if process has visible windows
                if (IsProcessVisible(pe32.th32ProcessID)) {
                    processes.push_back(std::wstring(pe32.szExeFile));
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    
    return processes;
}

// Check if a specific app is running
bool IsAppRunning(const std::wstring& appName) {
    std::vector<std::wstring> processes = GetVisibleRunningProcesses();
    
    // Convert to lowercase for case-insensitive comparison
    std::wstring lowerAppName = appName;
    std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::towlower);
    
    for (const auto& process : processes) {
        std::wstring lowerProcess = process;
        std::transform(lowerProcess.begin(), lowerProcess.end(), lowerProcess.begin(), ::towlower);
        
        if (lowerProcess == lowerAppName) {
            return true;
        }
    }
    
    return false;
}

// App monitoring thread function
void AppMonitorThreadProc() {
    std::vector<std::wstring> lastRunningApps;
    
    while (appMonitoringRunning) {
        std::vector<std::wstring> currentRunningApps = GetVisibleRunningProcesses();
        
        // Check for newly started apps
        for (const auto& app : currentRunningApps) {
            // If app wasn't running before but is running now
            if (std::find(lastRunningApps.begin(), lastRunningApps.end(), app) == lastRunningApps.end()) {
                // Check if this app has a color profile
                std::lock_guard<std::mutex> lock(appProfilesMutex);
                for (auto& profile : appColorProfiles) {
                    std::wstring lowerAppName = profile.appName;
                    std::wstring lowerCurrentApp = app;
                    std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::towlower);
                    std::transform(lowerCurrentApp.begin(), lowerCurrentApp.end(), lowerCurrentApp.begin(), ::towlower);
                    
                    if (lowerAppName == lowerCurrentApp && !profile.isActive) {
                        // App started - activate its color profile
                        profile.isActive = true;
                        SetDefaultColor(profile.appColor);
                        if (profile.lockKeysEnabled) {
                            SetLockKeysColor(); // Update lock key colors based on profile's lock keys setting
                        }
                        break;
                    }
                }
            }
        }
        
        // Check for stopped apps
        for (const auto& app : lastRunningApps) {
            // If app was running before but is not running now
            if (std::find(currentRunningApps.begin(), currentRunningApps.end(), app) == currentRunningApps.end()) {
                std::lock_guard<std::mutex> lock(appProfilesMutex);
                for (auto& profile : appColorProfiles) {
                    std::wstring lowerAppName = profile.appName;
                    std::wstring lowerStoppedApp = app;
                    std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::towlower);
                    std::transform(lowerStoppedApp.begin(), lowerStoppedApp.end(), lowerStoppedApp.begin(), ::towlower);
                    
                    if (lowerAppName == lowerStoppedApp && profile.isActive) {
                        // App stopped - deactivate its color profile
                        profile.isActive = false;
                        
                        // Check if any other monitored app is still running
                        bool foundActiveApp = false;
                        AppColorProfile const* activeProfile = nullptr;
                        for (const auto& otherProfile : appColorProfiles) {
                            if (otherProfile.isActive && otherProfile.appName != profile.appName) {
                                // first active profile takes precedence
                                foundActiveApp = true;
                                activeProfile = &otherProfile;
                                break;
                            }
                        }
                        
                        if (foundActiveApp && activeProfile) {
                            // Hand off lighting to remaining active profile
                            SetDefaultColor(activeProfile->appColor);
                            if (activeProfile->lockKeysEnabled) {
                                SetLockKeysColor();
                            }
                        } else {
                            // If no monitored apps are running, restore default color and enable lock keys
                            SetDefaultColor(defaultColor);
                            SetLockKeysColor(); // Lock keys will be enabled again (default behavior)
                        }
                        break;
                    }
                }
            }
        }
        
        lastRunningApps = currentRunningApps;
        
        // Sleep for 2 seconds before next check
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

// Initialize app monitoring
void InitializeAppMonitoring() {
    if (!appMonitoringRunning) {
        appMonitoringRunning = true;
        appMonitorThread = std::thread(AppMonitorThreadProc);
    }
}

// Cleanup app monitoring
void CleanupAppMonitoring() {
    appMonitoringRunning = false;
    if (appMonitorThread.joinable()) {
        appMonitorThread.join();
    }
}

// Add an app color profile with lock keys feature control
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    // Check if profile already exists
    for (auto& profile : appColorProfiles) {
        std::wstring lowerExisting = profile.appName;
        std::wstring lowerNew = appName;
        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
        std::transform(lowerNew.begin(), lowerNew.end(), lowerNew.begin(), ::towlower);
        
        if (lowerExisting == lowerNew) {
            // Update existing profile
            profile.appColor = color;
            profile.lockKeysEnabled = lockKeysEnabled;
            profile.isActive = IsAppRunning(appName);
            return;
        }
    }
    
    // Add new profile
    AppColorProfile newProfile;
    newProfile.appName = appName;
    newProfile.appColor = color;
    newProfile.lockKeysEnabled = lockKeysEnabled;
    newProfile.isActive = IsAppRunning(appName);
    appColorProfiles.push_back(newProfile);
}

// Remove an app color profile
void RemoveAppColorProfile(const std::wstring& appName) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    appColorProfiles.erase(
        std::remove_if(appColorProfiles.begin(), appColorProfiles.end(),
            [&appName](const AppColorProfile& profile) {
                std::wstring lowerExisting = profile.appName;
                std::wstring lowerToRemove = appName;
                std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
                std::transform(lowerToRemove.begin(), lowerToRemove.end(), lowerToRemove.begin(), ::towlower);
                return lowerExisting == lowerToRemove;
            }),
        appColorProfiles.end()
    );
}

// Check if lock keys feature should be enabled based on current active profile
bool IsLockKeysFeatureEnabled() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    // Check if any profile is currently active
    for (const auto& profile : appColorProfiles) {
        if (profile.isActive) {
            return profile.lockKeysEnabled; // Return the setting from the active profile
        }
    }
    
    // If no profile is active, lock keys feature is always enabled (default behavior)
    return true;
}

// Check running apps and update colors immediately
void CheckRunningAppsAndUpdateColors() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    bool foundActiveApp = false;
    COLORREF activeColor = defaultColor;
    
    // Check all profiles
    for (auto& profile : appColorProfiles) {
        bool isRunning = IsAppRunning(profile.appName);
        profile.isActive = isRunning;
        
        // If multiple apps are running, the first one in the list takes precedence
        if (isRunning && !foundActiveApp) {
            foundActiveApp = true;
            activeColor = profile.appColor;
            // Set the appropriate color
            SetDefaultColor(activeColor);
            // Set lock keys color only if lock keys feature is enabled for this profile
            if (profile.lockKeysEnabled) {
                SetLockKeysColor();
            }
        }
    }
}