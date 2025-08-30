#include "SmartLogiLED_Config.h"
#include "SmartLogiLED_Logic.h"
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include "LogitechLEDLib.h"

// Registry constants
#define REGISTRY_KEY L"SOFTWARE\\SmartLogiLED"
#define REGISTRY_VALUE_START_MINIMIZED L"StartMinimized"
#define REGISTRY_VALUE_NUMLOCK_COLOR L"NumLockColor"
#define REGISTRY_VALUE_CAPSLOCK_COLOR L"CapsLockColor"
#define REGISTRY_VALUE_SCROLLLOCK_COLOR L"ScrollLockColor"
#define REGISTRY_VALUE_DEFAULT_COLOR L"DefaultColor"
const wchar_t* APP_PROFILES_SUBKEY = L"SOFTWARE\\SmartLogiLED\\AppProfiles";

// Registry functions for start minimized setting
void SaveStartMinimizedSetting(bool minimized) {
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 
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
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey);
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
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 
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
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey);
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

void SaveAppProfilesToRegistry() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    HKEY hProfilesKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, APP_PROFILES_SUBKEY, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hProfilesKey, NULL) != ERROR_SUCCESS) {
        return;
    }
    for (const auto& profile : appColorProfiles) {
        HKEY hAppKey = nullptr;
        if (RegCreateKeyExW(hProfilesKey, profile.appName.c_str(), 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hAppKey, NULL) == ERROR_SUCCESS) {
            DWORD d = static_cast<DWORD>(profile.appColor);
            RegSetValueExW(hAppKey, L"AppColor", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&d), sizeof(d));
            d = static_cast<DWORD>(profile.appHighlightColor);
            RegSetValueExW(hAppKey, L"AppHighlightColor", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&d), sizeof(d));
            d = profile.lockKeysEnabled ? 1u : 0u;
            RegSetValueExW(hAppKey, L"LockKeysEnabled", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&d), sizeof(d));
            if (!profile.highlightKeys.empty()) {
                std::vector<DWORD> data;
                data.reserve(profile.highlightKeys.size());
                for (auto k : profile.highlightKeys) data.push_back(static_cast<DWORD>(k));
                RegSetValueExW(hAppKey, L"HighlightKeys", 0, REG_BINARY,
                               reinterpret_cast<const BYTE*>(data.data()),
                               static_cast<DWORD>(data.size() * sizeof(DWORD)));
            } else {
                RegSetValueExW(hAppKey, L"HighlightKeys", 0, REG_BINARY, nullptr, 0);
            }
            RegCloseKey(hAppKey);
        }
    }
    RegCloseKey(hProfilesKey);
}

void LoadAppProfilesFromRegistry() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, APP_PROFILES_SUBKEY, 0, KEY_READ, &hProfilesKey) != ERROR_SUCCESS) {
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
                if (RegQueryValueExW(hAppKey, L"AppColor", NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.appColor = static_cast<COLORREF>(d);
                d = 0; cb = sizeof(DWORD); type = 0;
                if (RegQueryValueExW(hAppKey, L"AppHighlightColor", NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.appHighlightColor = static_cast<COLORREF>(d);
                d = 1; cb = sizeof(DWORD); type = 0;
                if (RegQueryValueExW(hAppKey, L"LockKeysEnabled", NULL, &type, reinterpret_cast<LPBYTE>(&d), &cb) == ERROR_SUCCESS && type == REG_DWORD)
                    p.lockKeysEnabled = (d != 0);
                DWORD dataSize = 0; type = 0;
                if (RegQueryValueExW(hAppKey, L"HighlightKeys", NULL, &type, NULL, &dataSize) == ERROR_SUCCESS && type == REG_BINARY && dataSize > 0) {
                    std::vector<DWORD> data(dataSize / sizeof(DWORD));
                    if (RegQueryValueExW(hAppKey, L"HighlightKeys", NULL, &type, reinterpret_cast<LPBYTE>(data.data()), &dataSize) == ERROR_SUCCESS) {
                        p.highlightKeys.clear();
                        p.highlightKeys.reserve(data.size());
                        for (DWORD v : data) p.highlightKeys.push_back(static_cast<LogiLed::KeyName>(v));
                    }
                }
                p.isActive = IsAppRunning(p.appName);
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
