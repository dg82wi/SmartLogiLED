// SmartLogiLED_Logic.cpp : Contains the logic functions for SmartLogiLED application.
//
// This file contains color management, keyboard hook logic, and app monitoring.

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_Logic.h"
#include "SmartLogiLED_Config.h"
#include "SmartLogiLED_KeyMapping.h"
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
#include <deque>    // For activation history

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
static std::deque<std::wstring> activationHistory; // Enhanced: Track activation history for better fallback
static const size_t MAX_ACTIVATION_HISTORY = 10; // Maximum number of profiles to remember in history

// Enhanced helper function to manage activation history
void UpdateActivationHistory(const std::wstring& profileName) {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    // Remove the profile from history if it already exists
    auto it = std::find(activationHistory.begin(), activationHistory.end(), profileName);
    if (it != activationHistory.end()) {
        activationHistory.erase(it);
    }
    
    // Add to front of history (most recent)
    activationHistory.push_front(profileName);
    
    // Limit history size
    if (activationHistory.size() > MAX_ACTIVATION_HISTORY) {
        activationHistory.pop_back();
    }
    
    // Update the current last activated profile
    lastActivatedProfile = profileName;
    
    // Debug logging
    std::wstringstream debugMsg;
    debugMsg << L"[DEBUG] Activation history updated. Current order: ";
    for (const auto& name : activationHistory) {
        debugMsg << name << L" -> ";
    }
    debugMsg << L"END\n";
    OutputDebugStringW(debugMsg.str().c_str());
}

// Enhanced function to find the best fallback profile
AppColorProfile* FindBestFallbackProfile(const std::wstring& excludeProfile = L"") {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    // Go through activation history from most recent to oldest
    for (const auto& historicalProfile : activationHistory) {
        // Skip the excluded profile (usually the one that just stopped)
        if (!excludeProfile.empty() && historicalProfile == excludeProfile) {
            continue;
        }
        
        // Find this profile in our current profiles
        for (auto& profile : appColorProfiles) {
            std::wstring lowerHistorical = historicalProfile;
            std::wstring lowerProfile = profile.appName;
            std::transform(lowerHistorical.begin(), lowerHistorical.end(), lowerHistorical.begin(), ::towlower);
            std::transform(lowerProfile.begin(), lowerProfile.end(), lowerProfile.begin(), ::towlower);
            
            if (lowerHistorical == lowerProfile && profile.isAppRunning) {
                // Found a running profile from our history - this is our best fallback
                std::wstringstream debugMsg;
                debugMsg << L"[DEBUG] Best fallback profile found: " << profile.appName 
                         << L" (from activation history)\n";
                OutputDebugStringW(debugMsg.str().c_str());
                return &profile;
            }
        }
    }
    
    // If no profile from history is available, find any running profile
    for (auto& profile : appColorProfiles) {
        if (profile.isAppRunning && 
            (excludeProfile.empty() || profile.appName != excludeProfile)) {
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Fallback profile found (not in history): " << profile.appName << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
            return &profile;
        }
    }
    
    OutputDebugStringW(L"[DEBUG] No fallback profile available\n");
    return nullptr;
}

// Enhanced function to clean up activation history
void CleanupActivationHistory() {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    // Remove profiles from history that are no longer configured
    auto it = activationHistory.begin();
    while (it != activationHistory.end()) {
        bool profileExists = false;
        for (const auto& profile : appColorProfiles) {
            std::wstring lowerHistorical = *it;
            std::wstring lowerProfile = profile.appName;
            std::transform(lowerHistorical.begin(), lowerHistorical.end(), lowerHistorical.begin(), ::towlower);
            std::transform(lowerProfile.begin(), lowerProfile.end(), lowerProfile.begin(), ::towlower);
            
            if (lowerHistorical == lowerProfile) {
                profileExists = true;
                break;
            }
        }
        
        if (!profileExists) {
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Removing deleted profile from activation history: " << *it << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
            it = activationHistory.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update lastActivatedProfile to match the front of history
    if (!activationHistory.empty()) {
        lastActivatedProfile = activationHistory.front();
    } else {
        lastActivatedProfile.clear();
    }
}

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
                // Send message to main window about app start
                if (mainWindowHandle) {
                    // Copy string to heap for message passing
                    std::wstring* appName = new std::wstring(app);
                    PostMessage(mainWindowHandle, WM_APP_STARTED, 0, reinterpret_cast<LPARAM>(appName));
                }
            }
        }
        
        // Check for stopped apps
        for (const auto& app : lastRunningApps) {
            // If app was running before but is not running now
            if (std::find(currentRunningApps.begin(), currentRunningApps.end(), app) == currentRunningApps.end()) {
                // Send message to main window about app stop
                if (mainWindowHandle) {
                    // Copy string to heap for message passing
                    std::wstring* appName = new std::wstring(app);
                    PostMessage(mainWindowHandle, WM_APP_STOPPED, 0, reinterpret_cast<LPARAM>(appName));
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
    
    // Clean up activation history after removing profile
    CleanupActivationHistory();
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
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
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
    
    // Use enhanced fallback logic to find the best profile
    selectedProfile = FindBestFallbackProfile();
    
    if (selectedProfile) {
        foundActiveApp = true;
        // Update activation history with this profile
        UpdateActivationHistory(selectedProfile->appName);
        
        // Debug message for last activated profile initialization
        std::wstringstream debugMsgLast;
        debugMsgLast << L"[DEBUG] Most recently activated profile initialized to: " << selectedProfile->appName << L" (scan)\n";
        OutputDebugStringW(debugMsgLast.str().c_str());
    }
    
    // Activate the selected profile
    if (selectedProfile) {
        selectedProfile->isProfileCurrInUse = true;
        
        // Debug message for isProfileCurrInUse change
        std::wstringstream debugMsg;
        debugMsg << L"[DEBUG] Profile " << selectedProfile->appName << L" - isProfileCurrInUse changed to TRUE (app start initialization)\n";
        OutputDebugStringW(debugMsg.str().c_str());
        
        // Set the appropriate color
        SetDefaultColor(selectedProfile->appColor);
        // Set lock keys color only if lock keys feature is enabled for this profile
        if (selectedProfile->lockKeysEnabled) {
            SetLockKeysColor();
        }
        
        // Apply highlight keys color
        SetHighlightKeysColor();
    }
    
    if (!foundActiveApp) {
        OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - No active profiles found, using default colors\n");
        lastActivatedProfile.clear();
        activationHistory.clear(); // Enhanced: clear activation history when no profiles are active
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

// Enhanced: Get the activation history for debugging/UI purposes
std::vector<std::wstring> GetActivationHistory() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return std::vector<std::wstring>(activationHistory.begin(), activationHistory.end());
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

// Message handlers for app monitoring
void HandleAppStarted(const std::wstring& appName) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    // Check if this app has a color profile
    for (auto& profile : appColorProfiles) {
        std::wstring lowerAppName = profile.appName;
        std::wstring lowerCurrentApp = appName;
        std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::towlower);
        std::transform(lowerCurrentApp.begin(), lowerCurrentApp.end(), lowerCurrentApp.begin(), ::towlower);
        
        if (lowerAppName == lowerCurrentApp && !profile.isAppRunning) {
            // App started - activate its color profile
            profile.isAppRunning = true;
            
            // Debug message for isAppRunning change
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] App started: " << profile.appName << L" - isAppRunning changed to TRUE\n";
            OutputDebugStringW(debugMsg.str().c_str());
            
            // Enhanced: Update activation history and make this the most recent
            UpdateActivationHistory(profile.appName);
            
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
                
                // Debug message for isProfileCurrInUse change
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

void HandleAppStopped(const std::wstring& appName) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    for (auto& profile : appColorProfiles) {
        std::wstring lowerAppName = profile.appName;
        std::wstring lowerStoppedApp = appName;
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
                
                // Enhanced: Use the enhanced fallback logic
                AppColorProfile* activeProfile = FindBestFallbackProfile(profile.appName);
                
                if (activeProfile) {
                    // Hand off lighting to the selected active profile
                    activeProfile->isProfileCurrInUse = true;
                    
                    // Update activation history to make this the current one
                    UpdateActivationHistory(activeProfile->appName);
                    
                    // Debug message for isProfileCurrInUse change
                    std::wstringstream debugMsg3;
                    debugMsg3 << L"[DEBUG] Profile " << activeProfile->appName << L" - isProfileCurrInUse changed to TRUE (enhanced handoff from " << profile.appName << L")\n";
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
                    lastActivatedProfile.clear();
                    activationHistory.clear(); // Enhanced: clear activation history
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