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
    
    // Determine the color to use for "off" state - app color if profile is active, otherwise default color
    COLORREF offStateColor = defaultColor;
    {
        for (const auto& profile : appColorProfiles) {
            if (profile.isProfileCurrInUse) {
                offStateColor = profile.appColor;
                break;
            }
        }
    }

    // NumLock
    keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    pressedKey = LogiLed::KeyName::NUM_LOCK;
    colorToSet = (keyState == 0x0001) ? numLockColor : offStateColor;
    SetKeyColor(pressedKey, colorToSet);

    // ScrollLock
    keyState = GetKeyState(VK_SCROLL) & 0x0001;
    pressedKey = LogiLed::KeyName::SCROLL_LOCK;
    colorToSet = (keyState == 0x0001) ? scrollLockColor : offStateColor;
    SetKeyColor(pressedKey, colorToSet);

    // CapsLock
    keyState = GetKeyState(VK_CAPITAL) & 0x0001;
    pressedKey = LogiLed::KeyName::CAPS_LOCK;
    colorToSet = (keyState == 0x0001) ? capsLockColor : offStateColor;
    SetKeyColor(pressedKey, colorToSet);
}

// Set highlight color for keys from the currently active profile
void SetHighlightKeysColor() {
    // Find the currently active profile
    for (const auto& profile : appColorProfiles) {
        if (profile.isProfileCurrInUse) {
            // Apply highlight color to all keys in the highlight list
            for (const auto& key : profile.highlightKeys) {
                // Check if this is a lock key and if lock keys are enabled
                bool isLockKey = (key == LogiLed::KeyName::NUM_LOCK || 
                                key == LogiLed::KeyName::CAPS_LOCK || 
                                key == LogiLed::KeyName::SCROLL_LOCK);
                
                if (isLockKey && profile.lockKeysEnabled) {
                    // For lock keys, check their state and apply appropriate color
                    SHORT keyState = 0;
                    COLORREF lockColor = defaultColor;
                    
                    if (key == LogiLed::KeyName::NUM_LOCK) {
                        keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
                        lockColor = (keyState == 0x0001) ? numLockColor : defaultColor;
                    } else if (key == LogiLed::KeyName::CAPS_LOCK) {
                        keyState = GetKeyState(VK_CAPITAL) & 0x0001;
                        lockColor = (keyState == 0x0001) ? capsLockColor : defaultColor;
                    } else if (key == LogiLed::KeyName::SCROLL_LOCK) {
                        keyState = GetKeyState(VK_SCROLL) & 0x0001;
                        lockColor = (keyState == 0x0001) ? scrollLockColor : defaultColor;
                    }
                    
                    // Lock key color takes precedence over highlight color when lock is active
                    if (keyState == 0x0001) {
                        SetKeyColor(key, lockColor);
                    } else {
                        SetKeyColor(key, profile.appHighlightColor);
                    }
                } else {
                    // For non-lock keys or when lock keys are disabled, apply highlight color
                    SetKeyColor(key, profile.appHighlightColor);
                }
            }
            break;
        }
    }
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

        // Determine the color to use for "off" state - app color if profile is active, otherwise default color
        COLORREF offStateColor = defaultColor;
        {
            std::lock_guard<std::mutex> lock(appProfilesMutex);
            for (const auto& profile : appColorProfiles) {
                if (profile.isProfileCurrInUse) {
                    offStateColor = profile.appColor;
                    break;
                }
            }
        }

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
                SetKeyColor(pressedKey, offStateColor);
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
                colorToSet = (keyState == 0x0000) ? numLockColor : offStateColor;
                break;
            case VK_SCROLL:
                keyState = GetKeyState(VK_SCROLL) & 0x0001;
                pressedKey = LogiLed::KeyName::SCROLL_LOCK;
                colorToSet = (keyState == 0x0000) ? scrollLockColor : offStateColor;
                break;
            case VK_CAPITAL:
                keyState = GetKeyState(VK_CAPITAL) & 0x0001;
                pressedKey = LogiLed::KeyName::CAPS_LOCK;
                colorToSet = (keyState == 0x0000) ? capsLockColor : offStateColor;
                break;
            }

            SetKeyColor(pressedKey, colorToSet);
            
            // Also reapply highlight keys in case the pressed key is in the highlight list
            SetHighlightKeysColor();
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
                            if (otherProfile.isProfileCurrInUse && otherProfile.appName != profile.appName) {
                                shouldDisplay = true; // Always take control as the most recently activated
                                break;
                            }
                        }
                        
                        if (shouldDisplay) {
                            // Clear all other displayed flags
                            for (auto& p : appColorProfiles) {
                                if (p.isProfileCurrInUse && p.appName != profile.appName) {
                                    p.isProfileCurrInUse = false;
                                    // Debug message for isProfileCurrInUse change
                                    std::wstringstream debugMsg2;
                                    debugMsg2 << L"[DEBUG] Profile " << p.appName << L" - isProfileCurrInUse changed to FALSE (handoff to most recent)\n";
                                    OutputDebugStringW(debugMsg2.str().c_str());
                                }
                            }
                            // Set this profile as displayed
                            profile.isProfileCurrInUse = true;
                            
                            // Debug message for isProfilecurrentlyInUse change
                            std::wstringstream debugMsg3;
                            debugMsg3 << L"[DEBUG] Profile " << profile.appName << L" - isProfileCurrInUse changed to TRUE (most recently activated)\n";
                            OutputDebugStringW(debugMsg3.str().c_str());
                            
                            SetDefaultColor(profile.appColor);
                            if (profile.lockKeysEnabled) {
                                SetLockKeysColor(); // Update lock key colors based on profile's lock keys setting
                            }
                            
                            // Apply highlight keys color
                            SetHighlightKeysColor();
                            
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
                        if (profile.isProfileCurrInUse) {
                            profile.isProfileCurrInUse = false;
                            
                            // Debug message for isProfileCurrInUse change
                            std::wstringstream debugMsg2;
                            debugMsg2 << L"[DEBUG] Profile " << profile.appName << L" - isProfileCurrInUse changed to FALSE (app stopped)\n";
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
                                activeProfile->isProfileCurrInUse = true;
                                
                                // Debug message for isProfileCurrInUse change
                                std::wstringstream debugMsg3;
                                debugMsg3 << L"[DEBUG] Profile " << activeProfile->appName << L" - isProfileCurrInUse changed to TRUE (handoff from " << profile.appName << L")\n";
                                OutputDebugStringW(debugMsg3.str().c_str());
                                
                                SetDefaultColor(activeProfile->appColor);
                                if (activeProfile->lockKeysEnabled) {
                                    SetLockKeysColor();
                                }
                                
                                // Apply highlight keys color
                                SetHighlightKeysColor();
                                
                                // Notify UI to update combo box
                                if (mainWindowHandle) {
                                    PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
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
            // Don't change isProfileCurrInUse flag when updating existing profile
            return;
        }
    }
    
    // Add new profile
    AppColorProfile newProfile;
    newProfile.appName = appName;
    newProfile.appColor = color;
    newProfile.lockKeysEnabled = lockKeysEnabled;
    newProfile.isAppRunning = IsAppRunning(appName);
    newProfile.isProfileCurrInUse = false; // Initialize as not displayed
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
        if (profile.isProfileCurrInUse) {
            return profile.lockKeysEnabled; // Return the setting from the displayed profile
        }
    }
    
    // If no profile is displayed, lock keys feature is always enabled (default behavior)
    return true;
}

// Check running apps and update colors immediately
void CheckRunningAppsAndUpdateColors() {
    OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - Starting scan\n");
    
    // First, clear all displayed flags
    for (auto& profile : appColorProfiles) {
        if (profile.isProfileCurrInUse) {
            profile.isProfileCurrInUse = false;
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile " << profile.appName << L" - isProfileCurrInUse changed to FALSE (scan reset)\n";
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
        selectedProfile->isProfileCurrInUse = true;
        
        // Debug message for isProfileCurrInUse change
        std::wstringstream debugMsg;
        debugMsg << L"[DEBUG] Profile " << selectedProfile->appName << L" - isProfileCurrInUse changed to TRUE (scan - priority: " 
                 << (selectedProfile->appName == lastActivatedProfile ? L"most recent" : L"first available") << L")\n";
        OutputDebugStringW(debugMsg.str().c_str());
        
        activeColor = selectedProfile->appColor;
        // Set the appropriate color
        SetDefaultColor(activeColor);
        // Set lock keys color only if lock keys feature is enabled for this profile
        if (selectedProfile->lockKeysEnabled) {
            SetLockKeysColor();
        }
        
        // Apply highlight keys color
        SetHighlightKeysColor();
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
        if (profile.isProfileCurrInUse) {
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

// Update app profile color
void UpdateAppProfileColor(const std::wstring& appName, COLORREF newAppColor) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        std::wstring lowerExisting = profile.appName;
        std::wstring lowerTarget = appName;
        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
        
        if (lowerExisting == lowerTarget) {
            profile.appColor = newAppColor;
            
            // If this profile is currently displayed, update the colors immediately
            if (profile.isProfileCurrInUse) {
                SetDefaultColor(newAppColor);
                if (profile.lockKeysEnabled) {
                    SetLockKeysColor();
                }
                // Apply highlight keys color
                SetHighlightKeysColor();
            }
            break;
        }
    }
}

// Update app profile highlight color
void UpdateAppProfileHighlightColor(const std::wstring& appName, COLORREF newHighlightColor) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        std::wstring lowerExisting = profile.appName;
        std::wstring lowerTarget = appName;
        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
        
        if (lowerExisting == lowerTarget) {
            profile.appHighlightColor = newHighlightColor;
            
            // If this profile is currently displayed, update highlight keys immediately
            if (profile.isProfileCurrInUse) {
                SetHighlightKeysColor();
            }
            break;
        }
    }
}

// Update app profile lock keys enabled setting
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        std::wstring lowerExisting = profile.appName;
        std::wstring lowerTarget = appName;
        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
        
        if (lowerExisting == lowerTarget) {
            profile.lockKeysEnabled = lockKeysEnabled;
            
            // If this profile is currently displayed, update lock keys behavior immediately
            if (profile.isProfileCurrInUse) {
                if (lockKeysEnabled) {
                    SetLockKeysColor(); // Enable lock key color visualization
                } else {
                    // Disable lock keys - set them to the app's color (not defaultColor)
                    SetKeyColor(LogiLed::KeyName::NUM_LOCK, profile.appColor);
                    SetKeyColor(LogiLed::KeyName::CAPS_LOCK, profile.appColor);
                    SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, profile.appColor);
                }
                
                // Reapply highlight keys (this will properly handle lock keys based on new setting)
                SetHighlightKeysColor();
            }
            break;
        }
    }
}

// Get app profile by name
AppColorProfile* GetAppProfileByName(const std::wstring& appName) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        std::wstring lowerExisting = profile.appName;
        std::wstring lowerTarget = appName;
        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
        
        if (lowerExisting == lowerTarget) {
            return &profile;
        }
    }
    
    return nullptr;
}

// Update app profile highlight keys
void UpdateAppProfileHighlightKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        std::wstring lowerExisting = profile.appName;
        std::wstring lowerTarget = appName;
        std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
        
        if (lowerExisting == lowerTarget) {
            profile.highlightKeys = highlightKeys;
            
            // If this profile is currently displayed, update highlight keys immediately
            if (profile.isProfileCurrInUse) {
                // First set default color to all keys to clear old highlights
                SetDefaultColor(profile.appColor);
                // Then reapply lock keys if enabled
                if (profile.lockKeysEnabled) {
                    SetLockKeysColor();
                }
                // Finally apply new highlight keys
                SetHighlightKeysColor();
            }
            break;
        }
    }
}

// Convert Virtual Key code to LogiLed::KeyName
LogiLed::KeyName VirtualKeyToLogiLedKey(DWORD vkCode) {
    switch (vkCode) {
        case VK_ESCAPE: return LogiLed::KeyName::ESC;
        case VK_F1: return LogiLed::KeyName::F1;
        case VK_F2: return LogiLed::KeyName::F2;
        case VK_F3: return LogiLed::KeyName::F3;
        case VK_F4: return LogiLed::KeyName::F4;
        case VK_F5: return LogiLed::KeyName::F5;
        case VK_F6: return LogiLed::KeyName::F6;
        case VK_F7: return LogiLed::KeyName::F7;
        case VK_F8: return LogiLed::KeyName::F8;
        case VK_F9: return LogiLed::KeyName::F9;
        case VK_F10: return LogiLed::KeyName::F10;
        case VK_F11: return LogiLed::KeyName::F11;
        case VK_F12: return LogiLed::KeyName::F12;
        case VK_SNAPSHOT: return LogiLed::KeyName::PRINT_SCREEN;
        case VK_SCROLL: return LogiLed::KeyName::SCROLL_LOCK;
        case VK_PAUSE: return LogiLed::KeyName::PAUSE_BREAK;
        case VK_OEM_3: return LogiLed::KeyName::TILDE; // ~ key
        case '1': return LogiLed::KeyName::ONE;
        case '2': return LogiLed::KeyName::TWO;
        case '3': return LogiLed::KeyName::THREE;
        case '4': return LogiLed::KeyName::FOUR;
        case '5': return LogiLed::KeyName::FIVE;
        case '6': return LogiLed::KeyName::SIX;
        case '7': return LogiLed::KeyName::SEVEN;
        case '8': return LogiLed::KeyName::EIGHT;
        case '9': return LogiLed::KeyName::NINE;
        case '0': return LogiLed::KeyName::ZERO;
        case VK_OEM_MINUS: return LogiLed::KeyName::MINUS;
        case VK_OEM_PLUS: return LogiLed::KeyName::EQUALS;
        case VK_BACK: return LogiLed::KeyName::BACKSPACE;
        case VK_INSERT: return LogiLed::KeyName::INSERT;
        case VK_HOME: return LogiLed::KeyName::HOME;
        case VK_PRIOR: return LogiLed::KeyName::PAGE_UP;
        case VK_NUMLOCK: return LogiLed::KeyName::NUM_LOCK;
        case VK_DIVIDE: return LogiLed::KeyName::NUM_SLASH;
        case VK_MULTIPLY: return LogiLed::KeyName::NUM_ASTERISK;
        case VK_SUBTRACT: return LogiLed::KeyName::NUM_MINUS;
        case VK_TAB: return LogiLed::KeyName::TAB;
        case 'Q': return LogiLed::KeyName::Q;
        case 'W': return LogiLed::KeyName::W;
        case 'E': return LogiLed::KeyName::E;
        case 'R': return LogiLed::KeyName::R;
        case 'T': return LogiLed::KeyName::T;
        case 'Y': return LogiLed::KeyName::Y;
        case 'U': return LogiLed::KeyName::U;
        case 'I': return LogiLed::KeyName::I;
        case 'O': return LogiLed::KeyName::O;
        case 'P': return LogiLed::KeyName::P;
        case VK_OEM_4: return LogiLed::KeyName::OPEN_BRACKET; // [
        case VK_OEM_6: return LogiLed::KeyName::CLOSE_BRACKET; // ]
        case VK_OEM_5: return LogiLed::KeyName::BACKSLASH; // backslash
        case VK_DELETE: return LogiLed::KeyName::KEYBOARD_DELETE;
        case VK_END: return LogiLed::KeyName::END;
        case VK_NEXT: return LogiLed::KeyName::PAGE_DOWN;
        case VK_NUMPAD7: return LogiLed::KeyName::NUM_SEVEN;
        case VK_NUMPAD8: return LogiLed::KeyName::NUM_EIGHT;
        case VK_NUMPAD9: return LogiLed::KeyName::NUM_NINE;
        case VK_ADD: return LogiLed::KeyName::NUM_PLUS;
        case VK_CAPITAL: return LogiLed::KeyName::CAPS_LOCK;
        case 'A': return LogiLed::KeyName::A;
        case 'S': return LogiLed::KeyName::S;
        case 'D': return LogiLed::KeyName::D;
        case 'F': return LogiLed::KeyName::F;
        case 'G': return LogiLed::KeyName::G;
        case 'H': return LogiLed::KeyName::H;
        case 'J': return LogiLed::KeyName::J;
        case 'K': return LogiLed::KeyName::K;
        case 'L': return LogiLed::KeyName::L;
        case VK_OEM_1: return LogiLed::KeyName::SEMICOLON; // ;
        case VK_OEM_7: return LogiLed::KeyName::APOSTROPHE; // '
        case VK_RETURN: return LogiLed::KeyName::ENTER;
        case VK_NUMPAD4: return LogiLed::KeyName::NUM_FOUR;
        case VK_NUMPAD5: return LogiLed::KeyName::NUM_FIVE;
        case VK_NUMPAD6: return LogiLed::KeyName::NUM_SIX;
        case VK_LSHIFT: return LogiLed::KeyName::LEFT_SHIFT;
        case 'Z': return LogiLed::KeyName::Z;
        case 'X': return LogiLed::KeyName::X;
        case 'C': return LogiLed::KeyName::C;
        case 'V': return LogiLed::KeyName::V;
        case 'B': return LogiLed::KeyName::B;
        case 'N': return LogiLed::KeyName::N;
        case 'M': return LogiLed::KeyName::M;
        case VK_OEM_COMMA: return LogiLed::KeyName::COMMA;
        case VK_OEM_PERIOD: return LogiLed::KeyName::PERIOD;
        case VK_OEM_2: return LogiLed::KeyName::FORWARD_SLASH; // /
        case VK_RSHIFT: return LogiLed::KeyName::RIGHT_SHIFT;
        case VK_UP: return LogiLed::KeyName::ARROW_UP;
        case VK_NUMPAD1: return LogiLed::KeyName::NUM_ONE;
        case VK_NUMPAD2: return LogiLed::KeyName::NUM_TWO;
        case VK_NUMPAD3: return LogiLed::KeyName::NUM_THREE;
        case VK_LCONTROL: return LogiLed::KeyName::LEFT_CONTROL;
        case VK_LWIN: return LogiLed::KeyName::LEFT_WINDOWS;
        case VK_LMENU: return LogiLed::KeyName::LEFT_ALT;
        case VK_SPACE: return LogiLed::KeyName::SPACE;
        case VK_RMENU: return LogiLed::KeyName::RIGHT_ALT;
        case VK_RWIN: return LogiLed::KeyName::RIGHT_WINDOWS;
        case VK_APPS: return LogiLed::KeyName::APPLICATION_SELECT;
        case VK_RCONTROL: return LogiLed::KeyName::RIGHT_CONTROL;
        case VK_LEFT: return LogiLed::KeyName::ARROW_LEFT;
        case VK_DOWN: return LogiLed::KeyName::ARROW_DOWN;
        case VK_RIGHT: return LogiLed::KeyName::ARROW_RIGHT;
        case VK_NUMPAD0: return LogiLed::KeyName::NUM_ZERO;
        case VK_DECIMAL: return LogiLed::KeyName::NUM_PERIOD;
        default: return LogiLed::KeyName::ESC; // Return ESC for unknown keys
    }
}

// Convert LogiLed::KeyName to display name
std::wstring LogiLedKeyToDisplayName(LogiLed::KeyName key) {
    switch (key) {
        case LogiLed::KeyName::ESC: return L"ESC";
        case LogiLed::KeyName::F1: return L"F1";
        case LogiLed::KeyName::F2: return L"F2";
        case LogiLed::KeyName::F3: return L"F3";
        case LogiLed::KeyName::F4: return L"F4";
        case LogiLed::KeyName::F5: return L"F5";
        case LogiLed::KeyName::F6: return L"F6";
        case LogiLed::KeyName::F7: return L"F7";
        case LogiLed::KeyName::F8: return L"F8";
        case LogiLed::KeyName::F9: return L"F9";
        case LogiLed::KeyName::F10: return L"F10";
        case LogiLed::KeyName::F11: return L"F11";
        case LogiLed::KeyName::F12: return L"F12";
        case LogiLed::KeyName::PRINT_SCREEN: return L"PRINT";
        case LogiLed::KeyName::SCROLL_LOCK: return L"SCROLL";
        case LogiLed::KeyName::PAUSE_BREAK: return L"PAUSE";
        case LogiLed::KeyName::TILDE: return L"~";
        case LogiLed::KeyName::ONE: return L"1";
        case LogiLed::KeyName::TWO: return L"2";
        case LogiLed::KeyName::THREE: return L"3";
        case LogiLed::KeyName::FOUR: return L"4";
        case LogiLed::KeyName::FIVE: return L"5";
        case LogiLed::KeyName::SIX: return L"6";
        case LogiLed::KeyName::SEVEN: return L"7";
        case LogiLed::KeyName::EIGHT: return L"8";
        case LogiLed::KeyName::NINE: return L"9";
        case LogiLed::KeyName::ZERO: return L"0";
        case LogiLed::KeyName::MINUS: return L"-";
        case LogiLed::KeyName::EQUALS: return L"=";
        case LogiLed::KeyName::BACKSPACE: return L"BACKSPACE";
        case LogiLed::KeyName::INSERT: return L"INSERT";
        case LogiLed::KeyName::HOME: return L"HOME";
        case LogiLed::KeyName::PAGE_UP: return L"PGUP";
        case LogiLed::KeyName::NUM_LOCK: return L"NUMLOCK";
        case LogiLed::KeyName::NUM_SLASH: return L"NUM/";
        case LogiLed::KeyName::NUM_ASTERISK: return L"NUM*";
        case LogiLed::KeyName::NUM_MINUS: return L"NUM-";
        case LogiLed::KeyName::TAB: return L"TAB";
        case LogiLed::KeyName::Q: return L"Q";
        case LogiLed::KeyName::W: return L"W";
        case LogiLed::KeyName::E: return L"E";
        case LogiLed::KeyName::R: return L"R";
        case LogiLed::KeyName::T: return L"T";
        case LogiLed::KeyName::Y: return L"Y";
        case LogiLed::KeyName::U: return L"U";
        case LogiLed::KeyName::I: return L"I";
        case LogiLed::KeyName::O: return L"O";
        case LogiLed::KeyName::P: return L"P";
        case LogiLed::KeyName::OPEN_BRACKET: return L"[";
        case LogiLed::KeyName::CLOSE_BRACKET: return L"]";
        case LogiLed::KeyName::BACKSLASH: return L"\\";
        case LogiLed::KeyName::KEYBOARD_DELETE: return L"DELETE";
        case LogiLed::KeyName::END: return L"END";
        case LogiLed::KeyName::PAGE_DOWN: return L"PGDN";
        case LogiLed::KeyName::NUM_SEVEN: return L"NUM7";
        case LogiLed::KeyName::NUM_EIGHT: return L"NUM8";
        case LogiLed::KeyName::NUM_NINE: return L"NUM9";
        case LogiLed::KeyName::NUM_PLUS: return L"NUM+";
        case LogiLed::KeyName::CAPS_LOCK: return L"CAPS";
        case LogiLed::KeyName::A: return L"A";
        case LogiLed::KeyName::S: return L"S";
        case LogiLed::KeyName::D: return L"D";
        case LogiLed::KeyName::F: return L"F";
        case LogiLed::KeyName::G: return L"G";
        case LogiLed::KeyName::H: return L"H";
        case LogiLed::KeyName::J: return L"J";
        case LogiLed::KeyName::K: return L"K";
        case LogiLed::KeyName::L: return L"L";
        case LogiLed::KeyName::SEMICOLON: return L";";
        case LogiLed::KeyName::APOSTROPHE: return L"'";
        case LogiLed::KeyName::ENTER: return L"ENTER";
        case LogiLed::KeyName::NUM_FOUR: return L"NUM4";
        case LogiLed::KeyName::NUM_FIVE: return L"NUM5";
        case LogiLed::KeyName::NUM_SIX: return L"NUM6";
        case LogiLed::KeyName::LEFT_SHIFT: return L"LSHIFT";
        case LogiLed::KeyName::Z: return L"Z";
        case LogiLed::KeyName::X: return L"X";
        case LogiLed::KeyName::C: return L"C";
        case LogiLed::KeyName::V: return L"V";
        case LogiLed::KeyName::B: return L"B";
        case LogiLed::KeyName::N: return L"N";
        case LogiLed::KeyName::M: return L"M";
        case LogiLed::KeyName::COMMA: return L",";
        case LogiLed::KeyName::PERIOD: return L".";
        case LogiLed::KeyName::FORWARD_SLASH: return L"/";
        case LogiLed::KeyName::RIGHT_SHIFT: return L"RSHIFT";
        case LogiLed::KeyName::ARROW_UP: return L"UP";
        case LogiLed::KeyName::NUM_ONE: return L"NUM1";
        case LogiLed::KeyName::NUM_TWO: return L"NUM2";
        case LogiLed::KeyName::NUM_THREE: return L"NUM3";
        case LogiLed::KeyName::NUM_ENTER: return L"NUMENTER";
        case LogiLed::KeyName::LEFT_CONTROL: return L"LCTRL";
        case LogiLed::KeyName::LEFT_WINDOWS: return L"LWIN";
        case LogiLed::KeyName::LEFT_ALT: return L"LALT";
        case LogiLed::KeyName::SPACE: return L"SPACE";
        case LogiLed::KeyName::RIGHT_ALT: return L"RALT";
        case LogiLed::KeyName::RIGHT_WINDOWS: return L"RWIN";
        case LogiLed::KeyName::APPLICATION_SELECT: return L"MENU";
        case LogiLed::KeyName::RIGHT_CONTROL: return L"RCTRL";
        case LogiLed::KeyName::ARROW_LEFT: return L"LEFT";
        case LogiLed::KeyName::ARROW_DOWN: return L"DOWN";
        case LogiLed::KeyName::ARROW_RIGHT: return L"RIGHT";
        case LogiLed::KeyName::NUM_ZERO: return L"NUM0";
        case LogiLed::KeyName::NUM_PERIOD: return L"NUM.";
        case LogiLed::KeyName::G_1: return L"G1";
        case LogiLed::KeyName::G_2: return L"G2";
        case LogiLed::KeyName::G_3: return L"G3";
        case LogiLed::KeyName::G_4: return L"G4";
        case LogiLed::KeyName::G_5: return L"G5";
        case LogiLed::KeyName::G_6: return L"G6";
        case LogiLed::KeyName::G_7: return L"G7";
        case LogiLed::KeyName::G_8: return L"G8";
        case LogiLed::KeyName::G_9: return L"G9";
        case LogiLed::KeyName::G_LOGO: return L"G_LOGO";
        case LogiLed::KeyName::G_BADGE: return L"G_BADGE";
        default: return L"UNKNOWN";
    }
}

// Format highlight keys for display in text field
std::wstring FormatHighlightKeysForDisplay(const std::vector<LogiLed::KeyName>& keys) {
    if (keys.empty()) {
        return L"";
    }
    
    std::wstring result;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (i > 0) {
            result += L" - ";
        }
        result += LogiLedKeyToDisplayName(keys[i]);
    }
    return result;
}