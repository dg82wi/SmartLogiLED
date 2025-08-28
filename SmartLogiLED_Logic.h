// SmartLogiLED_Logic.h : Header file for SmartLogiLED logic functions.
//

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"
#include <string>
#include <vector>
#include <functional>

// Registry constants
#define REGISTRY_KEY L"SOFTWARE\\SmartLogiLED"
#define REGISTRY_VALUE_START_MINIMIZED L"StartMinimized"
#define REGISTRY_VALUE_NUMLOCK_COLOR L"NumLockColor"
#define REGISTRY_VALUE_CAPSLOCK_COLOR L"CapsLockColor"
#define REGISTRY_VALUE_SCROLLLOCK_COLOR L"ScrollLockColor"
#define REGISTRY_VALUE_DEFAULT_COLOR L"DefaultColor"

// App monitoring structure
struct AppColorProfile {
    std::wstring appName;       // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor;          // Color to set when app starts
    bool isActive;              // Whether this profile is currently active
    bool lockKeysEnabled;       // Whether lock keys feature is enabled for this profile (default: true)
};

// Registry functions for start minimized setting
void SaveStartMinimizedSetting(bool minimized);
bool LoadStartMinimizedSetting();

// Registry functions for color settings
void SaveColorToRegistry(LPCWSTR valueName, COLORREF color);
COLORREF LoadColorFromRegistry(LPCWSTR valueName, COLORREF defaultValue);
void SaveColorsToRegistry();
void LoadColorsFromRegistry();

// Color and LED functions
void SetKeyColor(LogiLed::KeyName key, COLORREF color);
void SetDefaultColor(COLORREF color);
void SetLockKeysColor(void);
void ShowColorPicker(HWND hWnd, COLORREF& color, LogiLed::KeyName key = LogiLed::KeyName::ESC);

// App monitoring functions
void InitializeAppMonitoring();
void CleanupAppMonitoring();
bool IsAppRunning(const std::wstring& appName);
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);
void RemoveAppColorProfile(const std::wstring& appName);
void SetAppColorProfile(const std::wstring& appName, COLORREF color, bool active);
void SetAppColorProfile(const std::wstring& appName, COLORREF color, bool active, bool lockKeysEnabled);
std::vector<AppColorProfile>& GetAppColorProfiles();
std::vector<AppColorProfile> GetAppColorProfilesCopy();
void CheckRunningAppsAndUpdateColors();
bool IsLockKeysFeatureEnabled(); // Check if lock keys feature should be enabled based on current active profile

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);