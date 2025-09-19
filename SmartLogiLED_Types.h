// SmartLogiLED_Types.h : Common type definitions for SmartLogiLED application.
//

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"
#include <string>
#include <vector>

// App monitoring structure
struct AppColorProfile {
    std::wstring appName;       // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255); // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for UI representation
    COLORREF appActionColor = RGB(255, 255, 0); // Action color for action keys
    bool isAppRunning = false;              // Whether this profile is currently active
    bool isProfileCurrInUse = false;              // Whether this profile currently defines the key colors
    bool lockKeysEnabled = true;        // Whether lock keys feature is enabled for this profile
    std::vector<LogiLed::KeyName> highlightKeys; // list of keys which use the appHighlightColor
    std::vector<LogiLed::KeyName> actionKeys; // list of keys which use the appActionColor
};

// Message data structure for process communication
struct ProcessMessageData {
    std::wstring processName;
    bool isStarted; // true = started, false = stopped
};

// Custom Windows messages for UI and thread communication
#define WM_UPDATE_PROFILE_COMBO (WM_USER + 100)
#define WM_LOCK_KEY_PRESSED (WM_USER + 101)
#define WM_APP_STARTED (WM_USER + 102)
#define WM_APP_STOPPED (WM_USER + 103)
#define WM_PROCESS_LIST_UPDATE (WM_USER + 104)