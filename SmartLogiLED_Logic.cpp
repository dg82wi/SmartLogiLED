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
#include <sstream>  // For wstringstream

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
static std::wstring lastActivatedProfile; // Track the most recently activated profile

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
                    
                    if (lowerAppName == lowerCurrentApp && !profile.isAppRunning) {
                        // App started - activate its color profile
                        profile.isAppRunning = true;
                        
                        // Debug message for isAppRunning change
                        std::wstringstream debugMsg;
                        debugMsg << L"[DEBUG] App started: " << profile.appName << L" - isAppRunning changed to TRUE\n";
                        OutputDebugStringW(debugMsg.str().c_str());
                        
                        // Update the most recently activated profile
                        lastActivatedProfile = profile.appName;
                        
                        // Debug message for last activated profile change
                        std::wstringstream debugMsgLast;
                        debugMsgLast << L"[DEBUG] Most recently activated profile updated to: " << profile.appName << L"\n";
                        OutputDebugStringW(debugMsgLast.str().c_str());
                        
                        // Check if this should be the displayed profile (most recently activated takes precedence)
                        bool shouldDisplay = true;
                        for (const auto& otherProfile : appColorProfiles) {
                            if (otherProfile.isProfileCurrentlyInUse && otherProfile.appName != profile.appName) {
                                shouldDisplay = true; // Always take control as the most recently activated
                                break;
                            }
                        }
                        
                        if (shouldDisplay) {
                            // Clear all other displayed flags
                            for (auto& p : appColorProfiles) {
                                if (p.isProfileCurrentlyInUse && p.appName != profile.appName) {
                                    p.isProfileCurrentlyInUse = false;
                                    // Debug message for isProfileCurrentlyInUse change
                                    std::wstringstream debugMsg2;
                                    debugMsg2 << L"[DEBUG] Profile " << p.appName << L" - isProfileCurrentlyInUse changed to FALSE (handoff to most recent)\n";
                                    OutputDebugStringW(debugMsg2.str().c_str());
                                }
                            }
                            // Set this profile as displayed
                            profile.isProfileCurrentlyInUse = true;
                            
                            // Debug message for isProfileCurrentlyInUse change
                            std::wstringstream debugMsg3;
                            debugMsg3 << L"[DEBUG] Profile " << profile.appName << L" - isProfileCurrentlyInUse changed to TRUE (most recently activated)\n";
                            OutputDebugStringW(debugMsg3.str().c_str());
                            
                            SetDefaultColor(profile.appColor);
                            if (profile.lockKeysEnabled) {
                                SetLockKeysColor(); // Update lock key colors based on profile's lock keys setting
                            }
                            
                            // Notify UI to update combo box
                            if (mainWindowHandle) {
                                PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
                            }
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
                    
                    if (lowerAppName == lowerStoppedApp && profile.isAppRunning) {
                        // App stopped - deactivate its color profile
                        profile.isAppRunning = false;
                        
                        // Debug message for isAppRunning change
                        std::wstringstream debugMsg;
                        debugMsg << L"[DEBUG] App stopped: " << profile.appName << L" - isAppRunning changed to FALSE\n";
                        OutputDebugStringW(debugMsg.str().c_str());
                        
                        // If this was the displayed profile, find a replacement or clear display
                        if (profile.isProfileCurrentlyInUse) {
                            profile.isProfileCurrentlyInUse = false;
                            
                            // Debug message for isProfilecurrentlyInUse change
                            std::wstringstream debugMsg2;
                            debugMsg2 << L"[DEBUG] Profile " << profile.appName << L" - isProfilecurrentlyInUse changed to FALSE (app stopped)\n";
                            OutputDebugStringW(debugMsg2.str().c_str());
                            
                            // Find the most recently activated profile that is still running
                            AppColorProfile* activeProfile = nullptr;
                            
                            // First, try to find the most recently activated profile if it's still running
                            if (!lastActivatedProfile.empty()) {
                                for (auto& otherProfile : appColorProfiles) {
                                    if (otherProfile.isAppRunning && 
                                        otherProfile.appName == lastActivatedProfile && 
                                        otherProfile.appName != profile.appName) {
                                        activeProfile = &otherProfile;
                                        break;
                                    }
                                }
                            }
                            
                            // If the most recently activated profile is not running, find any other running profile
                            if (!activeProfile) {
                                for (auto& otherProfile : appColorProfiles) {
                                    if (otherProfile.isAppRunning && otherProfile.appName != profile.appName) {
                                        activeProfile = &otherProfile;
                                        // Update last activated to this profile since we're giving it control
                                        lastActivatedProfile = otherProfile.appName;
                                        
                                        // Debug message for last activated profile change
                                        std::wstringstream debugMsgLast;
                                        debugMsgLast << L"[DEBUG] Most recently activated profile updated to: " << otherProfile.appName << L" (handoff fallback)\n";
                                        OutputDebugStringW(debugMsgLast.str().c_str());
                                        break;
                                    }
                                }
                            }
                            
                            if (activeProfile) {
                                // Hand off lighting to the selected active profile
                                activeProfile->isProfileCurrentlyInUse = true;
                                
                                // Debug message for isProfilecurrentlyInUse change
                                std::wstringstream debugMsg3;
                                debugMsg3 << L"[DEBUG] Profile " << activeProfile->appName << L" - isProfilecurrentlyInUse changed to TRUE (handoff from " << profile.appName << L")\n";
                                OutputDebugStringW(debugMsg3.str().c_str());
                                
                                SetDefaultColor(activeProfile->appColor);
                                if (activeProfile->lockKeysEnabled) {
                                    SetLockKeysColor();
                                }
                            } else {
                                // If no monitored apps are running, restore default color and enable lock keys
                                OutputDebugStringW(L"[DEBUG] No more active profiles - restoring default colors\n");
                                lastActivatedProfile.clear(); // Clear the last activated profile
                                SetDefaultColor(defaultColor);
                                SetLockKeysColor(); // Lock keys will be enabled again (default behavior)
                            }
                            
                            // Notify UI to update combo box
                            if (mainWindowHandle) {
                                PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
                            }
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
            profile.isAppRunning = IsAppRunning(appName);
            // Don't change isProfileCurrentlyInUse flag when updating existing profile
            return;
        }
    }
    
    // Add new profile
    AppColorProfile newProfile;
    newProfile.appName = appName;
    newProfile.appColor = color;
    newProfile.lockKeysEnabled = lockKeysEnabled;
    newProfile.isAppRunning = IsAppRunning(appName);
    newProfile.isProfileCurrentlyInUse = false; // Initialize as not displayed
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

// Check if lock keys feature should be enabled based on current displayed profile
bool IsLockKeysFeatureEnabled() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    // Check if any profile is currently displayed (controlling colors)
    for (const auto& profile : appColorProfiles) {
        if (profile.isProfileCurrentlyInUse) {
            return profile.lockKeysEnabled; // Return the setting from the displayed profile
        }
    }
    
    // If no profile is displayed, lock keys feature is always enabled (default behavior)
    return true;
}

// Check running apps and update colors immediately
void CheckRunningAppsAndUpdateColors() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - Starting scan\n");
    
    // First, clear all displayed flags
    for (auto& profile : appColorProfiles) {
        if (profile.isProfileCurrentlyInUse) {
            profile.isProfileCurrentlyInUse = false;
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile " << profile.appName << L" - isProfileCurrentlyInUse changed to FALSE (scan reset)\n";
            OutputDebugStringW(debugMsg.str().c_str());
        }
    }
    
    bool foundActiveApp = false;
    COLORREF activeColor = defaultColor;
    AppColorProfile* selectedProfile = nullptr;
    
    // Update all profiles' running status first
    for (auto& profile : appColorProfiles) {
        bool isRunning = IsAppRunning(profile.appName);
        
        // Debug message for isAppRunning changes
        if (profile.isAppRunning != isRunning) {
            profile.isAppRunning = isRunning;
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile " << profile.appName << L" - isAppRunning changed to " << (isRunning ? L"TRUE" : L"FALSE") << L" (scan)\n";
            OutputDebugStringW(debugMsg.str().c_str());
        } else {
            profile.isAppRunning = isRunning;
        }
    }
    
    // Find the profile to activate (prioritize most recently activated)
    if (!lastActivatedProfile.empty()) {
        // First, try to find the most recently activated profile if it's running
        for (auto& profile : appColorProfiles) {
            if (profile.isAppRunning && profile.appName == lastActivatedProfile) {
                selectedProfile = &profile;
                foundActiveApp = true;
                break;
            }
        }
    }
    
    // If the most recently activated profile is not running, find the first running one
    if (!foundActiveApp) {
        for (auto& profile : appColorProfiles) {
            if (profile.isAppRunning) {
                selectedProfile = &profile;
                foundActiveApp = true;
                // Update last activated to this profile since we're giving it control
                lastActivatedProfile = profile.appName;
                
                // Debug message for last activated profile change
                std::wstringstream debugMsgLast;
                debugMsgLast << L"[DEBUG] Most recently activated profile updated to: " << profile.appName << L" (scan fallback)\n";
                OutputDebugStringW(debugMsgLast.str().c_str());
                break;
            }
        }
    }
    
    // Activate the selected profile
    if (selectedProfile) {
        selectedProfile->isProfileCurrentlyInUse = true;
        
        // Debug message for isProfileCurrentlyInUse change
        std::wstringstream debugMsg;
        debugMsg << L"[DEBUG] Profile " << selectedProfile->appName << L" - isProfileCurrentlyInUse changed to TRUE (scan - priority: " 
                 << (selectedProfile->appName == lastActivatedProfile ? L"most recent" : L"first available") << L")\n";
        OutputDebugStringW(debugMsg.str().c_str());
        
        activeColor = selectedProfile->appColor;
        // Set the appropriate color
        SetDefaultColor(activeColor);
        // Set lock keys color only if lock keys feature is enabled for this profile
        if (selectedProfile->lockKeysEnabled) {
            SetLockKeysColor();
        }
    }
    
    if (!foundActiveApp) {
        OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - No active profiles found, using default colors\n");
        lastActivatedProfile.clear(); // Clear the last activated profile
    }
    
    OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - Scan complete\n");
}

// Get thread-safe copy of app color profiles
std::vector<AppColorProfile> GetAppColorProfilesCopy() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return appColorProfiles;
}

// Get the currently displayed profile (the one controlling colors)
AppColorProfile* GetDisplayedProfile() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        if (profile.isProfileCurrentlyInUse) {
            return &profile;
        }
    }
    
    return nullptr; // No profile is currently displayed
}

// Set main window handle for UI updates
void SetMainWindowHandle(HWND hWnd) {
    mainWindowHandle = hWnd;
}

// Get the name of the most recently activated profile
std::wstring GetLastActivatedProfileName() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return lastActivatedProfile;
}