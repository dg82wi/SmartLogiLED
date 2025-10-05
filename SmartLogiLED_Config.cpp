#include "SmartLogiLED_Config.h"
#include "SmartLogiLED_KeyMapping.h"
#include "SmartLogiLED_AppProfiles.h"
#include "SmartLogiLED_ProcessMonitor.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <shlobj.h>

// ======================================================================
// REGISTRY FUNCTIONS FOR START MINIMIZED SETTING
// ======================================================================

// Registry functions for start minimized setting
void SaveStartMinimizedSetting(bool minimized) {
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_ROOT, 0, NULL, 
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        DWORD value = minimized ? 1 : 0;
        RegSetValueExW(hKey, REGISTRY_VALUE_START_MINIMIZED, 0, REG_DWORD, 
                     (const BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

bool LoadStartMinimizedSetting() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_ROOT, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        DWORD type = REG_DWORD;
        if (RegQueryValueExW(hKey, REGISTRY_VALUE_START_MINIMIZED, NULL, &type, 
                           (BYTE*)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value != 0;
        }
        RegCloseKey(hKey);
    }
    return false; // Default to false if setting doesn't exist
}

// Registry functions for color settings
void SaveColorToRegistry(LPCWSTR valueName, COLORREF color) {
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_ROOT, 0, NULL, 
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        DWORD colorValue = (DWORD)color;
        RegSetValueExW(hKey, valueName, 0, REG_DWORD, 
                     (const BYTE*)&colorValue, sizeof(colorValue));
        RegCloseKey(hKey);
    }
}

COLORREF LoadColorFromRegistry(LPCWSTR valueName, COLORREF defaultValue) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_ROOT, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        DWORD colorValue = 0;
        DWORD size = sizeof(colorValue);
        DWORD type = REG_DWORD;
        if (RegQueryValueExW(hKey, valueName, NULL, &type, 
                           (BYTE*)&colorValue, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (COLORREF)colorValue;
        }
        RegCloseKey(hKey);
    }
    return defaultValue; // Return default if setting doesn't exist
}

// Save color settings to registry
void SaveLockKeyColorsToRegistry() {
    extern COLORREF numLockColor;
    extern COLORREF capsLockColor;
    extern COLORREF scrollLockColor;
    extern COLORREF defaultColor;
    SaveColorToRegistry(REGISTRY_VALUE_NUMLOCK_COLOR, numLockColor);
    SaveColorToRegistry(REGISTRY_VALUE_CAPSLOCK_COLOR, capsLockColor);
    SaveColorToRegistry(REGISTRY_VALUE_SCROLLLOCK_COLOR, scrollLockColor);
    SaveColorToRegistry(REGISTRY_VALUE_DEFAULT_COLOR, defaultColor);
}

// Load color settings from registry
void LoadLockKeyColorsFromRegistry() {
    extern COLORREF numLockColor;
    extern COLORREF capsLockColor;
    extern COLORREF scrollLockColor;
    extern COLORREF defaultColor;
    numLockColor = LoadColorFromRegistry(REGISTRY_VALUE_NUMLOCK_COLOR, RGB(0, 179, 0));
    capsLockColor = LoadColorFromRegistry(REGISTRY_VALUE_CAPSLOCK_COLOR, RGB(0, 179, 0));
    scrollLockColor = LoadColorFromRegistry(REGISTRY_VALUE_SCROLLLOCK_COLOR, RGB(0, 179, 0));
    defaultColor = LoadColorFromRegistry(REGISTRY_VALUE_DEFAULT_COLOR, RGB(0, 89, 89));
}

// App profiles registry persistence
#include <mutex>
extern std::vector<AppColorProfile> appColorProfiles;
extern std::mutex appProfilesMutex;

void AddAppProfileToRegistry(const AppColorProfile& profile) {
    HKEY hProfilesKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_PROFILES, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hProfilesKey, NULL) != ERROR_SUCCESS) {
        return;
    }
    
    HKEY hAppKey = nullptr;
    if (RegCreateKeyExW(hProfilesKey, profile.appName.c_str(), 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hAppKey, NULL) == ERROR_SUCCESS) {
        DWORD d = static_cast<DWORD>(profile.appColor);
        RegSetValueExW(hAppKey, REGISTRY_VALUE_APP_COLOR, 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&d), sizeof(d));
        d = static_cast<DWORD>(profile.appHighlightColor);
        RegSetValueExW(hAppKey, REGISTRY_VALUE_APP_HIGHLIGHT_COLOR, 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&d), sizeof(d));
        d = static_cast<DWORD>(profile.appActionColor);
        RegSetValueExW(hAppKey, REGISTRY_VALUE_APP_ACTION_COLOR, 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&d), sizeof(d));
        d = profile.lockKeysEnabled ? 1u : 0u;
        RegSetValueExW(hAppKey, REGISTRY_VALUE_LOCK_KEYS_ENABLED, 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&d), sizeof(d));
        if (!profile.highlightKeys.empty()) {
            std::vector<DWORD> data;
            data.reserve(profile.highlightKeys.size());
            for (auto k : profile.highlightKeys) data.push_back(static_cast<DWORD>(k));
            RegSetValueExW(hAppKey, REGISTRY_VALUE_HIGHLIGHT_KEYS, 0, REG_BINARY,
                           reinterpret_cast<const BYTE*>(data.data()),
                           static_cast<DWORD>(data.size() * sizeof(DWORD)));
        } else {
            RegSetValueExW(hAppKey, REGISTRY_VALUE_HIGHLIGHT_KEYS, 0, REG_BINARY, nullptr, 0);
        }
        
        // Save action keys
        if (!profile.actionKeys.empty()) {
            std::vector<DWORD> data;
            data.reserve(profile.actionKeys.size());
            for (auto k : profile.actionKeys) data.push_back(static_cast<DWORD>(k));
            RegSetValueExW(hAppKey, REGISTRY_VALUE_ACTION_KEYS, 0, REG_BINARY,
                           reinterpret_cast<const BYTE*>(data.data()),
                           static_cast<DWORD>(data.size() * sizeof(DWORD)));
        } else {
            RegSetValueExW(hAppKey, REGISTRY_VALUE_ACTION_KEYS, 0, REG_BINARY, nullptr, 0);
        }
        
        RegCloseKey(hAppKey);
    }
    RegCloseKey(hProfilesKey);
}

void RemoveAppProfileFromRegistry(const std::wstring& appName) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_PROFILES, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        // Delete the subkey for this app profile
        RegDeleteKeyW(hProfilesKey, appName.c_str());
        RegCloseKey(hProfilesKey);
    }
}

void SaveAppProfilesToRegistry() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    // First, clear all existing profiles from registry
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_PROFILES, 0, KEY_READ | KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        // Enumerate and delete all existing subkeys
        DWORD subKeyCount = 0;
        if (RegQueryInfoKeyW(hProfilesKey, NULL, NULL, NULL, &subKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            // Collect subkey names first (since we can't delete while enumerating)
            std::vector<std::wstring> subKeyNames;
            for (DWORD i = 0; i < subKeyCount; ++i) {
                wchar_t subKeyName[260];
                DWORD nameLen = static_cast<DWORD>(std::size(subKeyName));
                if (RegEnumKeyExW(hProfilesKey, i, subKeyName, &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    subKeyNames.push_back(subKeyName);
                }
            }
            // Now delete all the subkeys
            for (const auto& subKeyName : subKeyNames) {
                RegDeleteKeyW(hProfilesKey, subKeyName.c_str());
            }
        }
        RegCloseKey(hProfilesKey);
    }
    
    // Now add all current profiles
    for (const auto& profile : appColorProfiles) {
        AddAppProfileToRegistry(profile);
    }
}

void LoadAppProfilesFromRegistry() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_PROFILES, 0, KEY_READ, &hProfilesKey) != ERROR_SUCCESS) {
        return;
    }
    appColorProfiles.clear();
    DWORD subKeyCount = 0;
    if (RegQueryInfoKeyW(hProfilesKey, NULL, NULL, NULL, &subKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hProfilesKey);
        return;
    }
    for (DWORD i = 0; i < subKeyCount; ++i) {
        wchar_t subKeyName[260];
        DWORD nameLen = static_cast<DWORD>(std::size(subKeyName));
        if (RegEnumKeyExW(hProfilesKey, i, subKeyName, &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY hAppKey = nullptr;
            if (RegOpenKeyExW(hProfilesKey, subKeyName, 0, KEY_READ, &hAppKey) == ERROR_SUCCESS) {
                AppColorProfile p;
                p.appName = subKeyName;
                DWORD type = 0; DWORD cb = sizeof(DWORD); DWORD d = 0;
                if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_APP_COLOR, NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.appColor = static_cast<COLORREF>(d);
                d = 0; cb = sizeof(DWORD); type = 0;
                if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_APP_HIGHLIGHT_COLOR, NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.appHighlightColor = static_cast<COLORREF>(d);
                d = 0; cb = sizeof(DWORD); type = 0;
                if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_APP_ACTION_COLOR, NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.appActionColor = static_cast<COLORREF>(d);
                d = 1; cb = sizeof(DWORD); type = 0;
                if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_LOCK_KEYS_ENABLED, NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.lockKeysEnabled = (d != 0);
                DWORD dataSize = 0; type = 0;
                if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_HIGHLIGHT_KEYS, NULL, &type, NULL, &dataSize) == ERROR_SUCCESS && type == REG_BINARY && dataSize > 0) {
                    std::vector<DWORD> data(dataSize / sizeof(DWORD));
                    if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_HIGHLIGHT_KEYS, NULL, &type, reinterpret_cast<LPBYTE>(data.data()), &dataSize) == ERROR_SUCCESS) {
                        p.highlightKeys.clear();
                        p.highlightKeys.reserve(data.size());
                        for (DWORD v : data) p.highlightKeys.push_back(static_cast<LogiLed::KeyName>(v));
                    }
                }
                
                // Load action keys
                dataSize = 0; type = 0;
                if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_ACTION_KEYS, NULL, &type, NULL, &dataSize) == ERROR_SUCCESS && type == REG_BINARY && dataSize > 0) {
                    std::vector<DWORD> data(dataSize / sizeof(DWORD));
                    if (RegQueryValueExW(hAppKey, REGISTRY_VALUE_ACTION_KEYS, NULL, &type, reinterpret_cast<LPBYTE>(data.data()), &dataSize) == ERROR_SUCCESS) {
                        p.actionKeys.clear();
                        p.actionKeys.reserve(data.size());
                        for (DWORD v : data) p.actionKeys.push_back(static_cast<LogiLed::KeyName>(v));
                    }
                }
                
                // Initialize runtime flags properly
                p.isAppRunning = IsAppRunning(p.appName);
                p.isProfileCurrInUse = false; // Will be set correctly by CheckRunningAppsAndUpdateColors()
                appColorProfiles.push_back(std::move(p));
                RegCloseKey(hAppKey);
            }
        }
    }
    RegCloseKey(hProfilesKey);
}

size_t GetAppProfilesCount() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return appColorProfiles.size();
}

// Generic helper function to update DWORD values in app profile registry
static void UpdateAppProfileDWordValueInRegistry(const std::wstring& appName, LPCWSTR valueName, DWORD value) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_PROFILES, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hProfilesKey, appName.c_str(), 0, KEY_WRITE, &hAppKey) == ERROR_SUCCESS) {
            RegSetValueExW(hAppKey, valueName, 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&value), sizeof(value));
            RegCloseKey(hAppKey);
        }
        RegCloseKey(hProfilesKey);
    }
}

// Generic helper function to update key vector data in app profile registry
static void UpdateAppProfileKeyVectorInRegistry(const std::wstring& appName, LPCWSTR valueName, const std::vector<LogiLed::KeyName>& keys) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SMARTLOGILED_REGISTRY_PROFILES, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hProfilesKey, appName.c_str(), 0, KEY_WRITE, &hAppKey) == ERROR_SUCCESS) {
            if (!keys.empty()) {
                std::vector<DWORD> data;
                data.reserve(keys.size());
                for (auto k : keys) data.push_back(static_cast<DWORD>(k));
                RegSetValueExW(hAppKey, valueName, 0, REG_BINARY,
                               reinterpret_cast<const BYTE*>(data.data()),
                               static_cast<DWORD>(data.size() * sizeof(DWORD)));
            } else {
                RegSetValueExW(hAppKey, valueName, 0, REG_BINARY, nullptr, 0);
            }
            RegCloseKey(hAppKey);
        }
        RegCloseKey(hProfilesKey);
    }
}

// Update specific app profile color in registry
void UpdateAppProfileColorInRegistry(const std::wstring& appName, COLORREF newAppColor) {
    UpdateAppProfileDWordValueInRegistry(appName, REGISTRY_VALUE_APP_COLOR, static_cast<DWORD>(newAppColor));
}

// Update specific app profile highlight color in registry
void UpdateAppProfileHighlightColorInRegistry(const std::wstring& appName, COLORREF newHighlightColor) {
    UpdateAppProfileDWordValueInRegistry(appName, REGISTRY_VALUE_APP_HIGHLIGHT_COLOR, static_cast<DWORD>(newHighlightColor));
}

// Update specific app profile action color in registry
void UpdateAppProfileActionColorInRegistry(const std::wstring& appName, COLORREF newActionColor) {
    UpdateAppProfileDWordValueInRegistry(appName, REGISTRY_VALUE_APP_ACTION_COLOR, static_cast<DWORD>(newActionColor));
}

// Update specific app profile lock keys enabled setting in registry
void UpdateAppProfileLockKeysEnabledInRegistry(const std::wstring& appName, bool lockKeysEnabled) {
    UpdateAppProfileDWordValueInRegistry(appName, REGISTRY_VALUE_LOCK_KEYS_ENABLED, lockKeysEnabled ? 1u : 0u);
}

// Update specific app profile highlight keys in registry
void UpdateAppProfileHighlightKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys) {
    UpdateAppProfileKeyVectorInRegistry(appName, REGISTRY_VALUE_HIGHLIGHT_KEYS, highlightKeys);
}

// Update specific app profile action keys in registry
void UpdateAppProfileActionKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& actionKeys) {
    UpdateAppProfileKeyVectorInRegistry(appName, REGISTRY_VALUE_ACTION_KEYS, actionKeys);
}
