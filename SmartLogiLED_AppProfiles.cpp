// SmartLogiLED_AppProfiles.cpp : Contains app profile and monitoring functionality.
//

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_AppProfiles.h"
#include "SmartLogiLED_LockKeys.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>
#include <deque>

// External variables from main file
extern COLORREF defaultColor;

// Custom Windows messages
#define WM_APP_STARTED (WM_USER + 102)
#define WM_APP_STOPPED (WM_USER + 103)
#define WM_UPDATE_PROFILE_COMBO (WM_USER + 100)

// App monitoring variables
std::vector<AppColorProfile> appColorProfiles;
std::mutex appProfilesMutex;
static std::thread appMonitorThread;
static bool appMonitoringRunning = false;
static HWND mainWindowHandle = nullptr;
static std::deque<std::wstring> activationHistory; // Enhanced: Track activation history for better fallback
static const size_t MAX_ACTIVATION_HISTORY = 10; // Maximum number of profiles to remember in history

// Helper function to get default color
COLORREF GetDefaultColor() {
    return defaultColor;
}

// Set main window handle for app profiles
void SetAppProfileMainWindowHandle(HWND hWnd) {
    mainWindowHandle = hWnd;
}

// ======================================================================
// INTERNAL HELPER FUNCTIONS (ASSUME MUTEX IS ALREADY LOCKED)
// ======================================================================

// Enhanced helper function to manage activation history (INTERNAL - NO LOCK)
void UpdateActivationHistoryInternal(const std::wstring& profileName) {
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
    
    // Debug logging
    std::wstringstream debugMsg;
    debugMsg << L"[DEBUG] Activation history updated. Current order: ";
    for (const auto& name : activationHistory) {
        debugMsg << name << L" -> ";
    }
    debugMsg << L"END\n";
    OutputDebugStringW(debugMsg.str().c_str());
}

// Enhanced function to find the best fallback profile (INTERNAL - NO LOCK)
AppColorProfile* FindBestFallbackProfileInternal(const std::wstring& excludeProfile) {
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

// Enhanced function to clean up activation history (INTERNAL - NO LOCK)
void CleanupActivationHistoryInternal() {
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
}

// Get the currently displayed profile (INTERNAL - NO LOCK)
AppColorProfile* GetDisplayedProfileInternal() {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    for (auto& profile : appColorProfiles) {
        if (profile.isProfileCurrInUse) {
            return &profile;
        }
    }
    
    return nullptr; // No profile is currently displayed
}

// Apply colors for a profile without holding mutex (INTERNAL - NO LOCK)
void ApplyProfileColorsInternal(AppColorProfile* profile) {
    // This function assumes mutex is NOT held and can be called safely
    // It will make necessary calls to LockKeys module
    
    if (!profile) {
        // No profile - use default colors and enable lock keys
        SetDefaultColor(defaultColor);
        SetLockKeysColor(); // This will use safe version
        UpdateKeyboardHookState(); // This will use safe version
        return;
    }
    
    // Apply profile colors
    SetDefaultColor(profile->appColor);
    
    if (profile->lockKeysEnabled) {
        SetLockKeysColorWithProfile(profile);
    } else {
        // Disable lock keys - set them to the app's color
        SetKeyColor(LogiLed::KeyName::NUM_LOCK, profile->appColor);
        SetKeyColor(LogiLed::KeyName::CAPS_LOCK, profile->appColor);
        SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, profile->appColor);
    }
    
    // Apply highlight keys
    SetHighlightKeysColorWithProfile(profile);
    
    // Update hook state
    UpdateKeyboardHookState();
}

// ======================================================================
// PUBLIC API FUNCTIONS (SAFE MUTEX USAGE)
// ======================================================================

// PUBLIC wrapper functions (for external API compatibility)
void UpdateActivationHistory(const std::wstring& profileName) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    UpdateActivationHistoryInternal(profileName);
}

AppColorProfile* FindBestFallbackProfile(const std::wstring& excludeProfile) {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return FindBestFallbackProfileInternal(excludeProfile);
}

void CleanupActivationHistory() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    CleanupActivationHistoryInternal();
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
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
    CleanupActivationHistoryInternal();
}

// Check running apps and update colors immediately
void CheckRunningAppsAndUpdateColors() {
    AppColorProfile* selectedProfile = nullptr;
    
    // Phase 1: Update profiles under lock, determine which profile to activate
    {
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
        selectedProfile = FindBestFallbackProfileInternal(L"");
        
        if (selectedProfile) {
            // Update activation history with this profile
            UpdateActivationHistoryInternal(selectedProfile->appName);
            
            // Debug message for last activated profile initialization
            std::wstringstream debugMsgLast;
            debugMsgLast << L"[DEBUG] Most recently activated profile initialized to: " << selectedProfile->appName << L" (scan)\n";
            OutputDebugStringW(debugMsgLast.str().c_str());
            
            // Mark this profile as active
            selectedProfile->isProfileCurrInUse = true;
            
            // Debug message for isProfileCurrInUse change
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile " << selectedProfile->appName << L" - isProfileCurrInUse changed to TRUE (app start initialization)\n";
            OutputDebugStringW(debugMsg.str().c_str());
        } else {
            OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - No active profiles found, using default colors\n");
        }
        
        OutputDebugStringW(L"[DEBUG] CheckRunningAppsAndUpdateColors() - Scan complete\n");
    } // Mutex released here
    
    // Phase 2: Apply colors without holding the mutex
    ApplyProfileColorsInternal(selectedProfile);
}

// Get thread-safe copy of app color profiles
std::vector<AppColorProfile> GetAppColorProfilesCopy() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return appColorProfiles;
}

// Get the currently displayed profile (the one controlling colors)
AppColorProfile* GetDisplayedProfile() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return GetDisplayedProfileInternal();
}

// Internal version that assumes mutex is already locked (DEPRECATED - use GetDisplayedProfileInternal)
AppColorProfile* GetDisplayedProfileUnsafe() {
    return GetDisplayedProfileInternal();
}

// Enhanced: Get the activation history for debugging/UI purposes
std::vector<std::wstring> GetActivationHistory() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return std::vector<std::wstring>(activationHistory.begin(), activationHistory.end());
}

// Update app profile color
void UpdateAppProfileColor(const std::wstring& appName, COLORREF newAppColor) {
    AppColorProfile* activeProfile = nullptr;
    
    // Phase 1: Update profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        for (auto& profile : appColorProfiles) {
            std::wstring lowerExisting = profile.appName;
            std::wstring lowerTarget = appName;
            std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
            std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
            
            if (lowerExisting == lowerTarget) {
                profile.appColor = newAppColor;
                
                // If this profile is currently displayed, we need to update colors
                if (profile.isProfileCurrInUse) {
                    activeProfile = &profile;
                }
                break;
            }
        }
    } // Mutex released here
    
    // Phase 2: Apply colors without holding mutex
    if (activeProfile) {
        ApplyProfileColorsInternal(activeProfile);
    }
}

// Update app profile highlight color
void UpdateAppProfileHighlightColor(const std::wstring& appName, COLORREF newHighlightColor) {
    AppColorProfile* activeProfile = nullptr;
    
    // Phase 1: Update profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        for (auto& profile : appColorProfiles) {
            std::wstring lowerExisting = profile.appName;
            std::wstring lowerTarget = appName;
            std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
            std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
            
            if (lowerExisting == lowerTarget) {
                profile.appHighlightColor = newHighlightColor;
                
                // If this profile is currently displayed, we need to update highlight keys
                if (profile.isProfileCurrInUse) {
                    activeProfile = &profile;
                }
                break;
            }
        }
    } // Mutex released here
    
    // Phase 2: Apply highlight colors without holding mutex
    if (activeProfile) {
        SetHighlightKeysColorWithProfile(activeProfile);
    }
}

// Update app profile lock keys enabled setting
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled) {
    AppColorProfile* activeProfile = nullptr;
    
    // Phase 1: Update profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        for (auto& profile : appColorProfiles) {
            std::wstring lowerExisting = profile.appName;
            std::wstring lowerTarget = appName;
            std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
            std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
            
            if (lowerExisting == lowerTarget) {
                profile.lockKeysEnabled = lockKeysEnabled;
                
                // If this profile is currently displayed, we need to update lock keys behavior
                if (profile.isProfileCurrInUse) {
                    activeProfile = &profile;
                }
                break;
            }
        }
    } // Mutex released here
    
    // Phase 2: Apply lock keys behavior without holding mutex
    if (activeProfile) {
        ApplyProfileColorsInternal(activeProfile);
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
    AppColorProfile* activeProfile = nullptr;
    
    // Phase 1: Update profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        for (auto& profile : appColorProfiles) {
            std::wstring lowerExisting = profile.appName;
            std::wstring lowerTarget = appName;
            std::transform(lowerExisting.begin(), lowerExisting.end(), lowerExisting.begin(), ::towlower);
            std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
            
            if (lowerExisting == lowerTarget) {
                profile.highlightKeys = highlightKeys;
                
                // If this profile is currently displayed, we need to update highlight keys
                if (profile.isProfileCurrInUse) {
                    activeProfile = &profile;
                }
                break;
            }
        }
    } // Mutex released here
    
    // Phase 2: Apply highlight keys without holding mutex
    if (activeProfile) {
        ApplyProfileColorsInternal(activeProfile);
    }
}

// Message handlers for app monitoring
void HandleAppStarted(const std::wstring& appName) {
    AppColorProfile* profileToActivate = nullptr;
    
    // Phase 1: Update profile state under lock
    {
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
                UpdateActivationHistoryInternal(profile.appName);
                
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
                    
                    profileToActivate = &profile;
                }
                break;
            }
        }
    } // Mutex released here
    
    // Phase 2: Apply colors and notify UI without holding mutex
    if (profileToActivate) {
        ApplyProfileColorsInternal(profileToActivate);
        
        // Notify UI to update combo box
        if (mainWindowHandle) {
            PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
        }
    }
}

void HandleAppStopped(const std::wstring& appName) {
    AppColorProfile* fallbackProfile = nullptr;
    bool needsDefaultRestore = false;
    
    // Phase 1: Update profile state under lock and determine fallback
    {
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
                    AppColorProfile* activeProfile = FindBestFallbackProfileInternal(profile.appName);
                    
                    if (activeProfile) {
                        // Hand off lighting to the selected active profile
                        activeProfile->isProfileCurrInUse = true;
                        
                        // Update activation history to make this the current one
                        UpdateActivationHistoryInternal(activeProfile->appName);
                        
                        // Debug message for isProfileCurrInUse change
                        std::wstringstream debugMsg3;
                        debugMsg3 << L"[DEBUG] Profile " << activeProfile->appName << L" - isProfileCurrInUse changed to TRUE (enhanced handoff from " << profile.appName << L")\n";
                        OutputDebugStringW(debugMsg3.str().c_str());
                        
                        fallbackProfile = activeProfile;
                    } else {
                        // If no monitored apps are running, restore default color and enable lock keys
                        OutputDebugStringW(L"[DEBUG] No more active profiles - restoring default colors\n");
                        activationHistory.clear(); // Enhanced: clear activation history
                        needsDefaultRestore = true;
                    }
                }
                break;
            }
        }
    } // Mutex released here
    
    // Phase 2: Apply colors and notify UI without holding mutex
    if (fallbackProfile) {
        ApplyProfileColorsInternal(fallbackProfile);
    } else if (needsDefaultRestore) {
        ApplyProfileColorsInternal(nullptr); // This will apply default colors
    }
    
    // Notify UI to update combo box
    if (mainWindowHandle && (fallbackProfile || needsDefaultRestore)) {
        PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
    }
}