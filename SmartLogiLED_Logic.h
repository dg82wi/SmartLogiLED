// SmartLogiLED_Logic.h : Header file for SmartLogiLED logic functions.
//

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"
#include <string>
#include <vector>
#include <functional>

// Custom Windows message for UI updates
#define WM_UPDATE_PROFILE_COMBO (WM_USER + 100)

// App monitoring structure
struct AppColorProfile {
    std::wstring appName;       // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255); // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for UI representation
    bool isAppRunning = false;              // Whether this profile is currently active
    bool isProfileCurrentlyInUse = false;              // Whether this profile currently defines the key colors
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
void SetMainWindowHandle(HWND hWnd); // Set main window handle for UI updates

// App profile access functions
std::vector<AppColorProfile> GetAppColorProfilesCopy(); // Thread-safe copy
size_t GetAppProfilesCount(); // Get number of profiles

// Get the currently displayed profile (the one controlling colors)
AppColorProfile* GetDisplayedProfile(); // Returns nullptr if no profile is displayed

// Get the name of the most recently activated profile
std::wstring GetLastActivatedProfileName(); // Returns empty string if none

// App profile color update functions
void UpdateAppProfileColor(const std::wstring& appName, COLORREF newAppColor);
void UpdateAppProfileHighlightColor(const std::wstring& appName, COLORREF newHighlightColor);
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled);
AppColorProfile* GetAppProfileByName(const std::wstring& appName);

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);