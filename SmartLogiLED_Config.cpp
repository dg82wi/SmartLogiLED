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

// Forward declaration for combo box refresh
void RefreshAppProfileCombo(HWND hWnd);

// Callback to set initial directory for SHBrowseForFolderW
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        if (lpData) {
            // lpData contains a wide string path
            SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
        }
        break;
    default:
        break;
    }
    return 0;
}

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

void AddAppProfileToRegistry(const AppColorProfile& profile) {
    HKEY hProfilesKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, NULL,
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
        RegCloseKey(hAppKey);
    }
    RegCloseKey(hProfilesKey);
}

void RemoveAppProfileFromRegistry(const std::wstring& appName) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        // Delete the subkey for this app profile
        RegDeleteKeyW(hProfilesKey, appName.c_str());
        RegCloseKey(hProfilesKey);
    }
}

void SaveAppProfilesToRegistry() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    
    // First, clear all existing profiles from registry
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_READ | KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
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
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_READ, &hProfilesKey) != ERROR_SUCCESS) {
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

// Update specific app profile color in registry
void UpdateAppProfileColorInRegistry(const std::wstring& appName, COLORREF newAppColor) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hProfilesKey, appName.c_str(), 0, KEY_WRITE, &hAppKey) == ERROR_SUCCESS) {
            DWORD d = static_cast<DWORD>(newAppColor);
            RegSetValueExW(hAppKey, REGISTRY_VALUE_APP_COLOR, 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&d), sizeof(d));
            RegCloseKey(hAppKey);
        }
        RegCloseKey(hProfilesKey);
    }
}

// Update specific app profile highlight color in registry
void UpdateAppProfileHighlightColorInRegistry(const std::wstring& appName, COLORREF newHighlightColor) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hProfilesKey, appName.c_str(), 0, KEY_WRITE, &hAppKey) == ERROR_SUCCESS) {
            DWORD d = static_cast<DWORD>(newHighlightColor);
            RegSetValueExW(hAppKey, REGISTRY_VALUE_APP_HIGHLIGHT_COLOR, 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&d), sizeof(d));
            RegCloseKey(hAppKey);
        }
        RegCloseKey(hProfilesKey);
    }
}

// Update specific app profile lock keys enabled setting in registry
void UpdateAppProfileLockKeysEnabledInRegistry(const std::wstring& appName, bool lockKeysEnabled) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hProfilesKey, appName.c_str(), 0, KEY_WRITE, &hAppKey) == ERROR_SUCCESS) {
            DWORD d = lockKeysEnabled ? 1u : 0u;
            RegSetValueExW(hAppKey, REGISTRY_VALUE_LOCK_KEYS_ENABLED, 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&d), sizeof(d));
            RegCloseKey(hAppKey);
        }
        RegCloseKey(hProfilesKey);
    }
}

// Update specific app profile highlight keys in registry
void UpdateAppProfileHighlightKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys) {
    HKEY hProfilesKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_APP_PROFILES_SUBKEY, 0, KEY_WRITE, &hProfilesKey) == ERROR_SUCCESS) {
        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hProfilesKey, appName.c_str(), 0, KEY_WRITE, &hAppKey) == ERROR_SUCCESS) {
            if (!highlightKeys.empty()) {
                std::vector<DWORD> data;
                data.reserve(highlightKeys.size());
                for (auto k : highlightKeys) data.push_back(static_cast<DWORD>(k));
                RegSetValueExW(hAppKey, REGISTRY_VALUE_HIGHLIGHT_KEYS, 0, REG_BINARY,
                               reinterpret_cast<const BYTE*>(data.data()),
                               static_cast<DWORD>(data.size() * sizeof(DWORD)));
            } else {
                RegSetValueExW(hAppKey, REGISTRY_VALUE_HIGHLIGHT_KEYS, 0, REG_BINARY, nullptr, 0);
            }
            RegCloseKey(hAppKey);
        }
        RegCloseKey(hProfilesKey);
    }
}

// Update the existing INI file or create a new one while preserving comments
void UpdateOrCreateProfileIniFile(const std::wstring& filename, const AppColorProfile& profile) {
    std::wstringstream newContent;
    bool fileExists = false;
    bool foundProfileSection = false;
    
    // Check if file exists and read existing content
    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        fileExists = true;
        DWORD fileSize = GetFileSize(hFile, nullptr);
        if (fileSize != INVALID_FILE_SIZE && fileSize > 0) {
            std::vector<char> buffer(fileSize + 1);
            DWORD bytesRead = 0;
            
            if (ReadFile(hFile, buffer.data(), fileSize, &bytesRead, nullptr)) {
                buffer[bytesRead] = '\0';
                
                // Convert from UTF-8 to wide string
                int wideSize = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, nullptr, 0);
                if (wideSize > 0) {
                    std::wstring existingContent(wideSize - 1, L'\0');
                    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, &existingContent[0], wideSize);
                    
                    // Parse and update existing content
                    std::wstringstream ss(existingContent);
                    std::wstring line;
                    bool inProfileSection = false;
                    
                    while (std::getline(ss, line)) {
                        std::wstring trimmedLine = line;
                        trimmedLine.erase(0, trimmedLine.find_first_not_of(L" \t\r\n"));
                        trimmedLine.erase(trimmedLine.find_last_not_of(L" \t\r\n") + 1);
                        
                        if (trimmedLine == L"[SmartLogiLED Profile]") {
                            foundProfileSection = true;
                            inProfileSection = true;
                            newContent << line << L"\n";
                            continue;
                        }
                        
                        if (trimmedLine.empty() || trimmedLine[0] == L';') {
                            // Preserve empty lines and comments
                            newContent << line << L"\n";
                            continue;
                        }
                        
                        if (trimmedLine[0] == L'[') {
                            inProfileSection = false;
                            newContent << line << L"\n";
                            continue;
                        }
                        
                        if (inProfileSection) {
                            size_t equalPos = trimmedLine.find(L'=');
                            if (equalPos != std::wstring::npos) {
                                std::wstring key = trimmedLine.substr(0, equalPos);
                                key.erase(0, key.find_first_not_of(L" \t"));
                                key.erase(key.find_last_not_of(L" \t") + 1);
                                
                                // Update profile values, keep everything else
                                if (key == L"AppName") {
                                    newContent << L"AppName=" << profile.appName << L"\n";
                                } else if (key == L"AppColor") {
                                    newContent << L"AppColor=" << std::hex << std::setfill(L'0') << std::setw(6) << (profile.appColor & 0xFFFFFF) << L"\n";
                                } else if (key == L"AppHighlightColor") {
                                    newContent << L"AppHighlightColor=" << std::hex << std::setfill(L'0') << std::setw(6) << (profile.appHighlightColor & 0xFFFFFF) << L"\n";
                                } else if (key == L"LockKeysEnabled") {
                                    newContent << L"LockKeysEnabled=" << (profile.lockKeysEnabled ? L"1" : L"0") << L"\n";
                                } else if (key == L"HighlightKeys") {
                                    newContent << L"HighlightKeys=";
                                    if (!profile.highlightKeys.empty()) {
                                        for (size_t i = 0; i < profile.highlightKeys.size(); ++i) {
                                            if (i > 0) newContent << L",";
                                            newContent << LogiLedKeyToConfigName(profile.highlightKeys[i]);
                                        }
                                    }
                                    newContent << L"\n";
                                } else {
                                    // Keep any other unknown keys unchanged
                                    newContent << line << L"\n";
                                }
                            } else {
                                // Keep non-key=value lines unchanged
                                newContent << line << L"\n";
                            }
                        } else {
                            // Keep lines outside profile section unchanged
                            newContent << line << L"\n";
                        }
                    }
                }
            }
        }
        CloseHandle(hFile);
    }
    
    // If file doesn't exist or no profile section found, create new content
    if (!fileExists || !foundProfileSection) {
        if (foundProfileSection) {
            // Profile section exists but needs update - content already built above
        } else {
            // Create completely new content
            newContent.str(L"");
            newContent.clear();
            newContent << L"[SmartLogiLED Profile]\n";
            newContent << L"AppName=" << profile.appName << L"\n";
            newContent << L"AppColor=" << std::hex << std::setfill(L'0') << std::setw(6) << (profile.appColor & 0xFFFFFF) << L"\n";
            newContent << L"AppHighlightColor=" << std::hex << std::setfill(L'0') << std::setw(6) << (profile.appHighlightColor & 0xFFFFFF) << L"\n";
            newContent << L"LockKeysEnabled=" << (profile.lockKeysEnabled ? L"1" : L"0") << L"\n";
            if (!profile.highlightKeys.empty()) {
                newContent << L"HighlightKeys=";
                for (size_t i = 0; i < profile.highlightKeys.size(); ++i) {
                    if (i > 0) newContent << L",";
                    newContent << LogiLedKeyToConfigName(profile.highlightKeys[i]);
                }
                newContent << L"\n";
            } else {
                newContent << L"HighlightKeys=\n";
            }
            newContent << L"\n";
            newContent << L"; SmartLogiLED Profile Export\n";
            newContent << L"; Generated by " << SMARTLOGILED_PRODUCT_NAME << L" v" << SMARTLOGILED_VERSION_STRING << L" (" << SMARTLOGILED_BUILD_TYPE << L")\n";
            newContent << L"; " << SMARTLOGILED_COPYRIGHT << L"\n";
            newContent << L"; \n";
            newContent << L"; AppColor and AppHighlightColor are in hexadecimal RGB format (e.g., FF0000 = Red)\n";
            newContent << L"; LockKeysEnabled: 1 = enabled, 0 = disabled\n";
            newContent << L"; HighlightKeys: Comma-separated list of key names to highlight\n";
        }
    }
    
    // Write the updated content back to file
    hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        std::string utf8Content;
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, newContent.str().c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Size > 0) {
            utf8Content.resize(utf8Size - 1);
            WideCharToMultiByte(CP_UTF8, 0, newContent.str().c_str(), -1, &utf8Content[0], utf8Size, nullptr, nullptr);
            DWORD bytesWritten = 0;
            WriteFile(hFile, utf8Content.c_str(), static_cast<DWORD>(utf8Content.size()), &bytesWritten, nullptr);
        }
        CloseHandle(hFile);
    }
}

// Helper function to get the application directory
std::wstring GetApplicationDirectory() {
    wchar_t exePath[MAX_PATH];
    DWORD pathLen = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (pathLen == 0 || pathLen == MAX_PATH) {
        return L""; // Failed to get path
    }
    
    std::wstring appDir = exePath;
    size_t lastSlash = appDir.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        appDir = appDir.substr(0, lastSlash);
    }
    return appDir;
}

// Helper function to get the default AppProfiles export directory
std::wstring GetDefaultExportDirectory() {
    std::wstring appDir = GetApplicationDirectory();
    if (appDir.empty()) {
        return L"";
    }
    
    std::wstring exportDir = appDir + L"\\AppProfiles";
    
    // Create the directory if it doesn't exist
    CreateDirectoryW(exportDir.c_str(), nullptr);
    
    return exportDir;
}

// Export all profiles to individual INI files
void ExportAllProfilesToIniFiles() {
    // Get default export directory
    std::wstring defaultDir = GetDefaultExportDirectory();
    
    // Show folder selection dialog with default directory via callback
    BROWSEINFOW bi = {0};
    wchar_t szFolder[MAX_PATH] = {0};
    bi.hwndOwner = nullptr;
    bi.lpszTitle = L"Select folder to export profile INI files";
    bi.pszDisplayName = szFolder; // output buffer
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = defaultDir.empty() ? 0 : reinterpret_cast<LPARAM>(defaultDir.c_str());

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) {
        MessageBoxW(nullptr, L"Export cancelled.", L"Export Profiles", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!SHGetPathFromIDListW(pidl, szFolder)) {
        CoTaskMemFree(pidl);
        MessageBoxW(nullptr, L"Failed to get selected folder path.", L"Export Error", MB_OK | MB_ICONERROR);
        return;
    }
    CoTaskMemFree(pidl);
    std::wstring exportDir = szFolder;
    if (!exportDir.empty() && exportDir.back() != L'\\') exportDir += L'\\';

    // Get all app profiles
    std::vector<AppColorProfile> profiles = GetAppColorProfilesCopy();
    if (profiles.empty()) {
        MessageBoxW(nullptr, L"No app profiles to export", L"Export Profiles", MB_OK | MB_ICONINFORMATION);
        return;
    }
    int exportedCount = 0;
    int updatedCount = 0;
    std::wstring errorMessages;
    for (const auto& profile : profiles) {
        // Create filename: SmartLogiLED_<appname>.ini
        std::wstring appBaseName = profile.appName;
        if (appBaseName.length() > 4 && appBaseName.substr(appBaseName.length() - 4) == L".exe") {
            appBaseName = appBaseName.substr(0, appBaseName.length() - 4);
        }
        std::wstring validChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
        for (auto& ch : appBaseName) {
            if (validChars.find(ch) == std::wstring::npos) {
                ch = L'_';
            }
        }
        std::wstring filename = exportDir + L"SmartLogiLED_" + appBaseName + L".ini";
        
        // Check if file already exists
        bool fileExists = (GetFileAttributesW(filename.c_str()) != INVALID_FILE_ATTRIBUTES);
        
        try {
            UpdateOrCreateProfileIniFile(filename, profile);
            if (fileExists) {
                updatedCount++;
            } else {
                exportedCount++;
            }
        } catch (...) {
            errorMessages += L"Failed to update/create file: " + filename + L"\n";
        }
    }
    std::wstringstream resultMessage;
    resultMessage << L"Export completed.\n";
    if (exportedCount > 0) {
        resultMessage << L"Created " << exportedCount << L" new profile file(s).\n";
    }
    if (updatedCount > 0) {
        resultMessage << L"Updated " << updatedCount << L" existing profile file(s).\n";
    }
    resultMessage << L"Total profiles processed: " << profiles.size() << L"\n";
    resultMessage << L"Files location: " << exportDir << L"\n";
    if (!errorMessages.empty()) {
        resultMessage << L"\nErrors encountered:\n" << errorMessages;
        MessageBoxW(nullptr, resultMessage.str().c_str(), L"Export Profiles", MB_OK | MB_ICONWARNING);
    } else {
        MessageBoxW(nullptr, resultMessage.str().c_str(), L"Export Profiles", MB_OK | MB_ICONINFORMATION);
    }
}

// Export a single profile to INI file
void ExportSelectedProfileToIniFile(HWND hWnd) {
    // Get the selected profile from the combo box
    HWND hCombo = GetDlgItem(hWnd, IDC_COMBO_APPPROFILE);
    if (!hCombo) {
        MessageBoxW(hWnd, L"Could not access profile selection.", L"Export Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR || selectedIndex == 0) {
        MessageBoxW(hWnd, L"Please select a valid app profile to export.\n\n\"NONE\" cannot be exported.", L"Export Selected Profile", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Get the selected app name
    WCHAR appName[256];
    SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)appName);
    
    // Get the profile data
    AppColorProfile* profile = GetAppProfileByName(appName);
    if (!profile) {
        MessageBoxW(hWnd, L"Selected profile not found.", L"Export Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Get default export directory
    std::wstring defaultExportDir = GetDefaultExportDirectory();
    
    // Show save file dialog
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = { 0 };
    
    // Create default filename: SmartLogiLED_<appname>.ini
    std::wstring defaultName = L"SmartLogiLED_";
    std::wstring appBaseName = profile->appName;
    if (appBaseName.length() > 4 && appBaseName.substr(appBaseName.length() - 4) == L".exe") {
        appBaseName = appBaseName.substr(0, appBaseName.length() - 4);
    }
    // Sanitize filename characters
    std::wstring validChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
    for (auto& ch : appBaseName) {
        if (validChars.find(ch) == std::wstring::npos) {
            ch = L'_';
        }
    }
    defaultName += appBaseName + L".ini";
    
    // If we have a default directory, include it in the full path
    if (!defaultExportDir.empty()) {
        std::wstring fullPath = defaultExportDir + L"\\" + defaultName;
        wcscpy_s(szFile, fullPath.c_str());
    } else {
        wcscpy_s(szFile, defaultName.c_str());
    }
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"SmartLogiLED Profile Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = defaultExportDir.empty() ? nullptr : defaultExportDir.c_str();  // Set default directory
    ofn.lpstrTitle = L"Export Profile";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY; // Removed OFN_OVERWRITEPROMPT since we handle updates
    ofn.lpstrDefExt = L"ini";
    
    if (!GetSaveFileNameW(&ofn)) {
        return; // User cancelled
    }
    
    // Check if file already exists
    bool fileExists = (GetFileAttributesW(szFile) != INVALID_FILE_ATTRIBUTES);
    std::wstring actionMessage;
    
    if (fileExists) {
        actionMessage = L"The file already exists and will be updated while preserving all comments.\n\nDo you want to continue?";
        int result = MessageBoxW(hWnd, actionMessage.c_str(), L"Update Existing Profile", MB_YESNO | MB_ICONQUESTION);
        if (result == IDNO) {
            return; // User cancelled
        }
    }
    
    // Update or create the file
    try {
        UpdateOrCreateProfileIniFile(szFile, *profile);
        
        std::wstring successMessage = fileExists ? 
            L"Profile file updated successfully!\n\nAll comments have been preserved." :
            L"Profile file created successfully!";
        MessageBoxW(hWnd, successMessage.c_str(), L"Export Complete", MB_OK | MB_ICONINFORMATION);
    } catch (...) {
        MessageBoxW(hWnd, L"Failed to update/create the profile file.", L"Export Error", MB_OK | MB_ICONERROR);
    }
}

// Import profile from INI file
void ImportProfileFromIniFile(HWND hWnd) {
    // Get default export directory for imports as well
    std::wstring defaultImportDir = GetDefaultExportDirectory();
    
    // Show file open dialog
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = { 0 };
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"SmartLogiLED Profile Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = defaultImportDir.empty() ? nullptr : defaultImportDir.c_str();  // Set default directory
    ofn.lpstrTitle = L"Select Profile to Import";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    
    if (!GetOpenFileNameW(&ofn)) {
        return; // User cancelled
    }
    
    // Read and parse the INI file
    AppColorProfile importedProfile;
    bool profileValid = false;
    std::wstring errorMessage;
    
    // Read file content
    HANDLE hFile = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, nullptr, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD fileSize = GetFileSize(hFile, nullptr);
        if (fileSize != INVALID_FILE_SIZE && fileSize > 0) {
            std::vector<char> buffer(fileSize + 1);
            DWORD bytesRead = 0;
            
            if (ReadFile(hFile, buffer.data(), fileSize, &bytesRead, nullptr)) {
                buffer[bytesRead] = '\0'; // Null terminate
                
                // Convert from UTF-8 to wide string
                int wideSize = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, nullptr, 0);
                if (wideSize > 0) {
                    std::wstring fileContent(wideSize - 1, L'\0');
                    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, &fileContent[0], wideSize);
                    
                    // Parse the INI content
                    std::wstringstream ss(fileContent);
                    std::wstring line;
                    bool inProfileSection = false;
                    
                    // Initialize with defaults
                    importedProfile.appColor = RGB(0, 255, 255);
                    importedProfile.appHighlightColor = RGB(255, 255, 255);
                    importedProfile.lockKeysEnabled = true;
                    importedProfile.isAppRunning = false;
                    importedProfile.isProfileCurrInUse = false;
                    
                    while (std::getline(ss, line)) {
                        // Trim whitespace
                        line.erase(0, line.find_first_not_of(L" \t\r\n"));
                        line.erase(line.find_last_not_of(L" \t\r\n") + 1);
                        
                        if (line.empty() || line[0] == L';') {
                            continue; // Skip empty lines and comments
                        }
                        
                        if (line == L"[SmartLogiLED Profile]") {
                            inProfileSection = true;
                            continue;
                        }
                        
                        if (line[0] == L'[') {
                            inProfileSection = false;
                            continue;
                        }
                        
                        if (inProfileSection) {
                            size_t equalPos = line.find(L'=');
                            if (equalPos != std::wstring::npos) {
                                std::wstring key = line.substr(0, equalPos);
                                std::wstring value = line.substr(equalPos + 1);
                                
                                // Trim key and value
                                key.erase(0, key.find_first_not_of(L" \t"));
                                key.erase(key.find_last_not_of(L" \t") + 1);
                                value.erase(0, value.find_first_not_of(L" \t"));
                                value.erase(value.find_last_not_of(L" \t") + 1);
                                
                                if (key == L"AppName") {
                                    importedProfile.appName = value;
                                    profileValid = true;
                                } else if (key == L"AppColor") {
                                    try {
                                        unsigned long colorValue = std::wcstoul(value.c_str(), nullptr, 16);
                                        importedProfile.appColor = static_cast<COLORREF>(colorValue);
                                    } catch (...) {
                                        // Keep default color on parse error
                                    }
                                } else if (key == L"AppHighlightColor") {
                                    try {
                                        unsigned long colorValue = std::wcstoul(value.c_str(), nullptr, 16);
                                        importedProfile.appHighlightColor = static_cast<COLORREF>(colorValue);
                                    } catch (...) {
                                        // Keep default color on parse error
                                    }
                                } else if (key == L"LockKeysEnabled") {
                                    importedProfile.lockKeysEnabled = (value == L"1");
                                } else if (key == L"HighlightKeys") {
                                    if (!value.empty()) {
                                        // Parse comma-separated key names
                                        std::wstringstream keyStream(value);
                                        std::wstring keyName;
                                        
                                        while (std::getline(keyStream, keyName, L',')) {
                                            // Trim whitespace
                                            keyName.erase(0, keyName.find_first_not_of(L" \t"));
                                            keyName.erase(keyName.find_last_not_of(L" \t") + 1);
                                            
                                            if (!keyName.empty()) {
                                                LogiLed::KeyName logiKey = ConfigNameToLogiLedKey(keyName);
                                                if (logiKey != LogiLed::KeyName::ESC || keyName == L"ESC") {
                                                    importedProfile.highlightKeys.push_back(logiKey);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    errorMessage = L"Failed to convert file encoding";
                }
            } else {
                errorMessage = L"Failed to read file content";
            }
        } else {
            errorMessage = L"File is empty or invalid";
        }
        CloseHandle(hFile);
    } else {
        errorMessage = L"Failed to open file";
    }
    
    if (!profileValid || importedProfile.appName.empty()) {
        std::wstring message = L"Invalid profile file format";
        if (!errorMessage.empty()) {
            message += L": " + errorMessage;
        }
        MessageBoxW(hWnd, message.c_str(), L"Import Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Check if profile already exists
    AppColorProfile* existingProfile = GetAppProfileByName(importedProfile.appName);
    if (existingProfile) {
        std::wstring message = L"A profile for '" + importedProfile.appName + L"' already exists.\n\nDo you want to overwrite it?";
        int result = MessageBoxW(hWnd, message.c_str(), L"Profile Exists", MB_YESNO | MB_ICONQUESTION);
        
        if (result == IDNO) {
            return; // User cancelled or chose not to overwrite
        }
        
        // Update existing profile
        existingProfile->appColor = importedProfile.appColor;
        existingProfile->appHighlightColor = importedProfile.appHighlightColor;
        existingProfile->lockKeysEnabled = importedProfile.lockKeysEnabled;
        existingProfile->highlightKeys = importedProfile.highlightKeys;
        
        // Update in registry
        UpdateAppProfileColorInRegistry(importedProfile.appName, importedProfile.appColor);
        UpdateAppProfileHighlightColorInRegistry(importedProfile.appName, importedProfile.appHighlightColor);
        UpdateAppProfileLockKeysEnabledInRegistry(importedProfile.appName, importedProfile.lockKeysEnabled);
        UpdateAppProfileHighlightKeysInRegistry(importedProfile.appName, importedProfile.highlightKeys);
        
    } else {
        // Add new profile
        importedProfile.isAppRunning = IsAppRunning(importedProfile.appName);
        AddAppColorProfile(importedProfile.appName, importedProfile.appColor, importedProfile.lockKeysEnabled);
        
        // Update the highlight color and keys (AddAppColorProfile doesn't handle这些)
        UpdateAppProfileHighlightColor(importedProfile.appName, importedProfile.appHighlightColor);
        UpdateAppProfileHighlightKeys(importedProfile.appName, importedProfile.highlightKeys);
        
        // Save to registry
        AppColorProfile* newProfile = GetAppProfileByName(importedProfile.appName);
        if (newProfile) {
            AddAppProfileToRegistry(*newProfile);
        }
        
    }
    
    // Refresh the UI
    if (hWnd) {
        RefreshAppProfileCombo(hWnd); // <-- Add this line to repopulate the combo box
        PostMessage(hWnd, WM_UPDATE_PROFILE_COMBO, 0, 0);
    }
}
