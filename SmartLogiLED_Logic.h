// SmartLogiLED_Logic.h : Header file for SmartLogiLED logic functions.
//

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"
#include <string>
#include <vector>
#include <functional>

// App monitoring structure
struct AppColorProfile {
    std::wstring appName;       // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255); // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for UI representation
    bool isActive = false;              // Whether this profile is currently active
    bool lockKeysEnabled = true;        // Whether lock keys feature is enabled for this profile
    std::vector<LogiLed::KeyName> highlightKeys; // list of keys which use the appHighlightColor
};


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
void CheckRunningAppsAndUpdateColors();
bool IsLockKeysFeatureEnabled(); // Check if lock keys feature should be enabled based on current active profile

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);