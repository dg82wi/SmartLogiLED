// SmartLogiLED_AppProfiles.cpp : Contains app profile and monitoring functionality.
//

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_AppProfiles.h"
#include "SmartLogiLED_LockKeys.h"
#include "SmartLogiLED_ProcessMonitor.h" // Include the new module
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>
#include <deque>

// External variables from main file
extern COLORREF defaultColor;

// App monitoring variables
std::vector<AppColorProfile> appColorProfiles;
std::mutex appProfilesMutex;
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
// OPTIMIZED PROFILE SEARCH HELPERS
// ======================================================================

// Helper function for case-insensitive string comparison
inline std::wstring ToLowerCase(const std::wstring& str) {
    std::wstring lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::towlower);
    return lowerStr;
}

// Optimized helper function to find profile by name (INTERNAL - ASSUMES MUTEX LOCKED)
AppColorProfile* FindProfileByNameInternal(const std::wstring& appName) {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    std::wstring lowerAppName = ToLowerCase(appName);
    
    auto it = std::find_if(appColorProfiles.begin(), appColorProfiles.end(),
        [&lowerAppName](const AppColorProfile& profile) {
            return ToLowerCase(profile.appName) == lowerAppName;
        });
    
    return (it != appColorProfiles.end()) ? &(*it) : nullptr;
}

// Optimized helper function to check if profile exists (INTERNAL - ASSUMES MUTEX LOCKED)
bool ProfileExistsInternal(const std::wstring& appName) {
    return FindProfileByNameInternal(appName) != nullptr;
}

// Optimized helper function to find profile iterator by name (INTERNAL - ASSUMES MUTEX LOCKED)
std::vector<AppColorProfile>::iterator FindProfileIteratorInternal(const std::wstring& appName) {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    std::wstring lowerAppName = ToLowerCase(appName);
    
    return std::find_if(appColorProfiles.begin(), appColorProfiles.end(),
        [&lowerAppName](const AppColorProfile& profile) {
            return ToLowerCase(profile.appName) == lowerAppName;
        });
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

void RemoveFromActivationHistoryInternal(const std::wstring& profileName) {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    auto it = std::find(activationHistory.begin(), activationHistory.end(), profileName);
    if (it != activationHistory.end()) {
        activationHistory.erase(it);
        
        // Debug logging
        std::wstringstream debugMsg;
        debugMsg << L"[DEBUG] Removed profile from activation history: " << profileName << L"\n";
        OutputDebugStringW(debugMsg.str().c_str());
    }
}

// Enhanced function to find the best fallback profile (INTERNAL - NO LOCK)
AppColorProfile* FindBestFallbackProfileInternal(const std::wstring& excludeProfile) {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    std::wstringstream debugMsg;
    debugMsg << L"[DEBUG] FindBestFallbackProfileInternal called, excluding: '" << excludeProfile << L"'\n";
    OutputDebugStringW(debugMsg.str().c_str());
    
    // Go through activation history from most recent to oldest
    for (const auto& historicalProfile : activationHistory) {
        // Skip the excluded profile (usually the one that just stopped)
        if (!excludeProfile.empty() && ToLowerCase(historicalProfile) == ToLowerCase(excludeProfile)) {
            std::wstringstream debugMsg2;
            debugMsg2 << L"[DEBUG] Skipping excluded profile from history: " << historicalProfile << L"\n";
            OutputDebugStringW(debugMsg2.str().c_str());
            continue;
        }
        
        // Find this profile in our current profiles using optimized search
        AppColorProfile* profile = FindProfileByNameInternal(historicalProfile);
        if (profile) {
            std::wstringstream debugMsg2;
            debugMsg2 << L"[DEBUG] Checking profile from history: " << profile->appName 
                     << L" (isAppRunning: " << (profile->isAppRunning ? L"TRUE" : L"FALSE") << L")\n";
            OutputDebugStringW(debugMsg2.str().c_str());
            
            if (profile->isAppRunning) {
                // Found a running profile from our history - this is our best fallback
                std::wstringstream debugMsg3;
                debugMsg3 << L"[DEBUG] Best fallback profile found: " << profile->appName 
                         << L" (from activation history)\n";
                OutputDebugStringW(debugMsg3.str().c_str());
                return profile;
            }
        } else {
            std::wstringstream debugMsg2;
            debugMsg2 << L"[DEBUG] Profile from history no longer exists: " << historicalProfile << L"\n";
            OutputDebugStringW(debugMsg2.str().c_str());
        }
    }
    
    // If no profile from history is available, find any running profile
    for (auto& profile : appColorProfiles) {
        if (profile.isAppRunning && 
            (excludeProfile.empty() || ToLowerCase(profile.appName) != ToLowerCase(excludeProfile))) {
            std::wstringstream debugMsg2;
            debugMsg2 << L"[DEBUG] Fallback profile found (not in history): " << profile.appName << L"\n";
            OutputDebugStringW(debugMsg2.str().c_str());
            return &profile;
        }
    }
    
    OutputDebugStringW(L"[DEBUG] No fallback profile available - should restore defaults\n");
    return nullptr;
}

// Enhanced function to clean up activation history (INTERNAL - NO LOCK)
void CleanupActivationHistoryInternal() {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    
    // Remove profiles from history that are no longer configured
    auto it = activationHistory.begin();
    while (it != activationHistory.end()) {
        // Use optimized search to check if profile exists
        if (!ProfileExistsInternal(*it)) {
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
        OutputDebugStringW(L"[DEBUG] ApplyProfileColorsInternal: Applying default colors (no active profile)\n");
        SetDefaultColor(defaultColor);
        SetLockKeysColor(); // This will use safe version
        UpdateKeyboardHookState(); // This will use safe version
        return;
    }
    
    // Apply profile colors
    std::wstringstream debugMsg;
    debugMsg << L"[DEBUG] ApplyProfileColorsInternal: Applying profile colors for " << profile->appName 
             << L" (appColor: RGB(" << GetRValue(profile->appColor) << L"," 
             << GetGValue(profile->appColor) << L"," << GetBValue(profile->appColor) << L"), "
             << L"lockKeysEnabled: " << (profile->lockKeysEnabled ? L"TRUE" : L"FALSE") << L")\n";
    OutputDebugStringW(debugMsg.str().c_str());
    
    SetDefaultColor(profile->appColor);
    
    // Re-apply lock key colors based on the profile's setting
    SetLockKeysColorWithProfile(profile);
    
    // Apply highlight keys
    SetHighlightKeysColorWithProfile(profile);
    
    // Update hook state
    UpdateKeyboardHookState();
}

// Centralized function to determine and apply the active profile
void UpdateAndApplyActiveProfile() {
    AppColorProfile* profileToApply = nullptr;
    bool changed = false;
    std::wstring previousProfileName = L""; // Track the name instead of pointer

    // Phase 1: Determine the correct active profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);

        // Get the currently displayed profile and remember its name
        AppColorProfile* currentDisplayed = GetDisplayedProfileInternal();
        if (currentDisplayed) {
            previousProfileName = currentDisplayed->appName;
        }

        // Find the best fallback profile
        AppColorProfile* bestFallback = FindBestFallbackProfileInternal(L"");

        // Clear all isProfileCurrInUse flags
        for (auto& p : appColorProfiles) {
            if (p.isProfileCurrInUse) {
                // Debug logging for flag changes
                std::wstringstream debugMsg;
                debugMsg << L"[DEBUG] Profile " << p.appName << L" - isProfileCurrInUse changed to FALSE (UpdateAndApplyActiveProfile)\n";
                OutputDebugStringW(debugMsg.str().c_str());
            }
            p.isProfileCurrInUse = false;
        }

        // Set the new displayed profile if we have one
        if (bestFallback) {
            bestFallback->isProfileCurrInUse = true;
            // Debug logging for flag changes
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile " << bestFallback->appName << L" - isProfileCurrInUse changed to TRUE (UpdateAndApplyActiveProfile)\n";
            OutputDebugStringW(debugMsg.str().c_str());
        }

        // Check if the displayed profile has changed by comparing names
        std::wstring newProfileName = bestFallback ? bestFallback->appName : L"";
        if (previousProfileName != newProfileName) {
            changed = true;
            profileToApply = bestFallback; // This can be nullptr, which is correct for default colors
            
            // Debug logging
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile handoff: '" << previousProfileName 
                     << L"' -> '" << newProfileName << L"'\n";
            OutputDebugStringW(debugMsg.str().c_str());
        }
        
        // Special case: If we have no active profile and no previous profile was set,
        // we should still apply default colors on the first call
        if (!bestFallback && previousProfileName.empty()) {
            changed = true;
            profileToApply = nullptr;
            OutputDebugStringW(L"[DEBUG] No profiles available - forcing default color application\n");
        }
    } // Mutex is released here

    // Phase 2: Apply colors and notify UI if a change occurred
    if (changed) {
        if (profileToApply) {
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Applying profile: " << profileToApply->appName << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
        } else {
            OutputDebugStringW(L"[DEBUG] Applying default colors (no active profile)\n");
        }
        
        ApplyProfileColorsInternal(profileToApply); // nullptr will trigger default colors
        
        if (mainWindowHandle) {
            PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
        }
    } else {
        OutputDebugStringW(L"[DEBUG] UpdateAndApplyActiveProfile - No change detected\n");
    }
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

// Add an app color profile with lock keys feature control
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled) {
    bool shouldApplyColors = false;
    bool isNewProfile = false;
    
    // Phase 1: Add/update profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        // Check if profile already exists using optimized search
        AppColorProfile* existingProfile = FindProfileByNameInternal(appName);
        if (existingProfile) {
            // Update existing profile
            existingProfile->appColor = color;
            existingProfile->lockKeysEnabled = lockKeysEnabled;
            bool wasRunning = existingProfile->isAppRunning;
            existingProfile->isAppRunning = IsAppRunning(appName);
            
            // If app is running and profile is currently displayed, we need to update colors
            if (existingProfile->isAppRunning && existingProfile->isProfileCurrInUse) {
                shouldApplyColors = true;
            }
            
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Updated existing profile: " << appName 
                     << L" (isAppRunning: " << (existingProfile->isAppRunning ? L"TRUE" : L"FALSE") 
                     << L", shouldApplyColors: " << (shouldApplyColors ? L"TRUE" : L"FALSE") << L")\n";
            OutputDebugStringW(debugMsg.str().c_str());
            
            return;
        }
        
        // Add new profile
        AppColorProfile newProfile;
        newProfile.appName = appName;
        newProfile.appColor = color;
        newProfile.lockKeysEnabled = lockKeysEnabled;
        newProfile.isAppRunning = IsAppRunning(appName);
        newProfile.isProfileCurrInUse = false; // Initialize as not displayed
        
        isNewProfile = true;
        
        // If the app is already running, this new profile should take control
        if (newProfile.isAppRunning) {
            // Clear all other isProfileCurrInUse flags
            for (auto& p : appColorProfiles) {
                if (p.isProfileCurrInUse) {
                    p.isProfileCurrInUse = false;
                    std::wstringstream debugMsg;
                    debugMsg << L"[DEBUG] Profile " << p.appName << L" - isProfileCurrInUse changed to FALSE (new profile taking over)\n";
                    OutputDebugStringW(debugMsg.str().c_str());
                }
            }
            
            // Set this new profile as displayed
            newProfile.isProfileCurrInUse = true;
            
            // Update activation history
            UpdateActivationHistoryInternal(newProfile.appName);
            
            shouldApplyColors = true;
            
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] New profile added for running app: " << appName 
                     << L" - taking control of colors\n";
            OutputDebugStringW(debugMsg.str().c_str());
        } else {
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] New profile added for non-running app: " << appName << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
        }
        
        appColorProfiles.push_back(newProfile);
    } // Mutex released here
    
    // Phase 2: Apply colors if needed and notify UI
    if (shouldApplyColors) {
        // Get the profile again (safely) to apply its colors
        AppColorProfile* profileToApply = GetAppProfileByName(appName);
        if (profileToApply) {
            ApplyProfileColorsInternal(profileToApply);
            
            // Notify UI to update combo box
            if (mainWindowHandle) {
                PostMessage(mainWindowHandle, WM_UPDATE_PROFILE_COMBO, 0, 0);
            }
        }
    }
}

// Remove an app color profile  
void RemoveAppColorProfile(const std::wstring& appName) {
    bool wasDisplayedProfile = false;
    
    // Phase 1: Remove profile under lock and determine if a color update is needed
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        AppColorProfile* profileToRemove = FindProfileByNameInternal(appName);
        if (profileToRemove && profileToRemove->isProfileCurrInUse) {
            wasDisplayedProfile = true;
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Removing currently displayed profile: " << profileToRemove->appName << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
        }
        
        auto it = FindProfileIteratorInternal(appName);
        if (it != appColorProfiles.end()) {
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Removing profile from memory: " << it->appName << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
            appColorProfiles.erase(it);
        } else {
            std::wstringstream debugMsg;
            debugMsg << L"[DEBUG] Profile not found in memory for removal: " << appName << L"\n";
            OutputDebugStringW(debugMsg.str().c_str());
        }
        
        // Clean up activation history AFTER removing the profile
        CleanupActivationHistoryInternal();
        
        // Debug: Show remaining profiles
        std::wstringstream debugMsg2;
        debugMsg2 << L"[DEBUG] Remaining profiles after deletion (" << appColorProfiles.size() << L"): ";
        for (const auto& p : appColorProfiles) {
            debugMsg2 << p.appName << L"(running:" << (p.isAppRunning ? L"Y" : L"N") << L") ";
        }
        debugMsg2 << L"\n";
        OutputDebugStringW(debugMsg2.str().c_str());
        
    } // Mutex released here
    
    // Phase 2: If the displayed profile was removed, update the active profile
    if (wasDisplayedProfile) {
        std::wstringstream debugMsg;
        debugMsg << L"[DEBUG] Calling UpdateAndApplyActiveProfile due to profile removal\n";
        OutputDebugStringW(debugMsg.str().c_str());
        UpdateAndApplyActiveProfile();
    } else {
        std::wstringstream debugMsg;
        debugMsg << L"[DEBUG] Profile removal complete - no active profile change needed\n";
        OutputDebugStringW(debugMsg.str().c_str());
    }
}

// Check running apps and update colors immediately
void CheckRunningAppsAndUpdateColors() {
    // Phase 1: Update all profiles' running status under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        for (auto& profile : appColorProfiles) {
            profile.isAppRunning = IsAppRunning(profile.appName);
        }
    } // Mutex released here
    
    // Phase 2: Determine and apply the best active profile
    UpdateAndApplyActiveProfile();
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
        
        AppColorProfile* profile = FindProfileByNameInternal(appName);
        if (profile) {
            profile->appColor = newAppColor;
            
            // If this profile is currently displayed, we need to update colors
            if (profile->isProfileCurrInUse) {
                activeProfile = profile;
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
        
        AppColorProfile* profile = FindProfileByNameInternal(appName);
        if (profile) {
            profile->appHighlightColor = newHighlightColor;
            
            // If this profile is currently displayed, we need to update highlight keys
            if (profile->isProfileCurrInUse) {
                activeProfile = profile;
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
        
        AppColorProfile* profile = FindProfileByNameInternal(appName);
        if (profile) {
            profile->lockKeysEnabled = lockKeysEnabled;
            
            // If this profile is currently displayed, we need to update lock keys behavior
            if (profile->isProfileCurrInUse) {
                activeProfile = profile;
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
    return FindProfileByNameInternal(appName);
}

// Update app profile highlight keys
void UpdateAppProfileHighlightKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys) {
    AppColorProfile* activeProfile = nullptr;
    
    // Phase 1: Update profile under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        AppColorProfile* profile = FindProfileByNameInternal(appName);
        if (profile) {
            profile->highlightKeys = highlightKeys;
            
            // If this profile is currently displayed, we need to update highlight keys
            if (profile->isProfileCurrInUse) {
                activeProfile = profile;
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
    bool changed = false;
    
    // Phase 1: Update profile state under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        AppColorProfile* profile = FindProfileByNameInternal(appName);
        if (profile) {
            if (!profile->isAppRunning) {
                profile->isAppRunning = true;
                changed = true;
            }
            // Move to the front of the activation history regardless of running state
            UpdateActivationHistoryInternal(profile->appName);
        }
    } // Mutex released here
    
    // Phase 2: Update colors and UI if the state changed
    if (changed) {
        UpdateAndApplyActiveProfile();
    }
}

void HandleAppStopped(const std::wstring& appName) {
    bool changed = false;
    
    // Phase 1: Update profile state under lock
    {
        std::lock_guard<std::mutex> lock(appProfilesMutex);
        
        AppColorProfile* profile = FindProfileByNameInternal(appName);
        if (profile) {
            changed = true;
			// Remove from activation history
			RemoveFromActivationHistoryInternal(profile->appName);
            if (profile->isAppRunning) {
                profile->isAppRunning = false;
                std::wstringstream debugMsg;
                debugMsg << L"[DEBUG] Profile " << profile->appName << L" - isAppRunning changed to FALSE (HandleAppStopped)\n";
                OutputDebugStringW(debugMsg.str().c_str());
			}
        }
    } // Mutex released here
    
    // Phase 2: Update colors and UI if the state changed
    if (changed) {
        UpdateAndApplyActiveProfile();
    }
}