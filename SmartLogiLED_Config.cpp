#include "SmartLogiLED_Config.h"
#include "SmartLogiLED_KeyMapping.h"
#include "SmartLogiLED_AppProfiles.h"
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

// Export all profiles to individual INI files
void ExportAllProfilesToIniFiles() {
    // Show folder selection dialog
    BROWSEINFOW bi = {0};
    wchar_t szFolder[MAX_PATH] = {0};
    bi.hwndOwner = nullptr;
    bi.lpszTitle = L"Select folder to export profile INI files";
    bi.pszDisplayName = szFolder;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

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
        std::wstringstream iniContent;
        iniContent << L"[SmartLogiLED Profile]\n";
        iniContent << L"AppName=" << profile.appName << L"\n";
        iniContent << L"AppColor=" << std::hex << std::setfill(L'0') << std::setw(6) << (profile.appColor & 0xFFFFFF) << L"\n";
        iniContent << L"AppHighlightColor=" << std::hex << std::setfill(L'0') << std::setw(6) << (profile.appHighlightColor & 0xFFFFFF) << L"\n";
        iniContent << L"LockKeysEnabled=" << (profile.lockKeysEnabled ? L"1" : L"0") << L"\n";
        if (!profile.highlightKeys.empty()) {
            iniContent << L"HighlightKeys=";
            for (size_t i = 0; i < profile.highlightKeys.size(); ++i) {
                if (i > 0) iniContent << L",";
                iniContent << LogiLedKeyToConfigName(profile.highlightKeys[i]);
            }
            iniContent << L"\n";
        } else {
            iniContent << L"HighlightKeys=\n";
        }
        iniContent << L"\n";
        iniContent << L"; SmartLogiLED Profile Export\n";
        iniContent << L"; Generated automatically by SmartLogiLED\n";
        iniContent << L"; \n";
        iniContent << L"; AppColor and AppHighlightColor are in hexadecimal RGB format (e.g., FF0000 = Red)\n";
        iniContent << L"; LockKeysEnabled: 1 = enabled, 0 = disabled\n";
        iniContent << L"; HighlightKeys: Comma-separated list of key names to highlight\n";
        HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            std::string utf8Content;
            int utf8Size = WideCharToMultiByte(CP_UTF8, 0, iniContent.str().c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (utf8Size > 0) {
                utf8Content.resize(utf8Size - 1);
                WideCharToMultiByte(CP_UTF8, 0, iniContent.str().c_str(), -1, &utf8Content[0], utf8Size, nullptr, nullptr);
                DWORD bytesWritten = 0;
                if (WriteFile(hFile, utf8Content.c_str(), static_cast<DWORD>(utf8Content.size()), &bytesWritten, nullptr)) {
                    exportedCount++;
                } else {
                    errorMessages += L"Failed to write to file: " + filename + L"\n";
                }
            } else {
                errorMessages += L"Failed to convert content for file: " + filename + L"\n";
            }
            CloseHandle(hFile);
        } else {
            errorMessages += L"Failed to create file: " + filename + L"\n";
        }
    }
    std::wstringstream resultMessage;
    resultMessage << L"Export completed.\n";
    resultMessage << L"Exported " << exportedCount << L" profile(s) out of " << profiles.size() << L" total.\n";
    resultMessage << L"Files saved to: " << exportDir << L"\n";
    if (!errorMessages.empty()) {
        resultMessage << L"\nErrors encountered:\n" << errorMessages;
        MessageBoxW(nullptr, resultMessage.str().c_str(), L"Export Profiles", MB_OK | MB_ICONWARNING);
    } else {
        MessageBoxW(nullptr, resultMessage.str().c_str(), L"Export Profiles", MB_OK | MB_ICONINFORMATION);
    }
}

// Import profile from INI file
void ImportProfileFromIniFile(HWND hWnd) {
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
    ofn.lpstrInitialDir = nullptr;
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
        
        MessageBoxW(hWnd, L"Profile updated successfully!", L"Import Complete", MB_OK | MB_ICONINFORMATION);
    } else {
        // Add new profile
        importedProfile.isAppRunning = IsAppRunning(importedProfile.appName);
        AddAppColorProfile(importedProfile.appName, importedProfile.appColor, importedProfile.lockKeysEnabled);
        
        // Update the highlight color and keys (AddAppColorProfile doesn't handle these)
        UpdateAppProfileHighlightColor(importedProfile.appName, importedProfile.appHighlightColor);
        UpdateAppProfileHighlightKeys(importedProfile.appName, importedProfile.highlightKeys);
        
        // Save to registry
        AppColorProfile* newProfile = GetAppProfileByName(importedProfile.appName);
        if (newProfile) {
            AddAppProfileToRegistry(*newProfile);
        }
        
        MessageBoxW(hWnd, L"Profile imported successfully!", L"Import Complete", MB_OK | MB_ICONINFORMATION);
    }
    
    // Refresh the UI
    if (hWnd) {
        RefreshAppProfileCombo(hWnd); // <-- Add this line to repopulate the combo box
        PostMessage(hWnd, WM_UPDATE_PROFILE_COMBO, 0, 0);
    }
}
