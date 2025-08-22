// SmartLogiLED_Logic.cpp : Contains the logic functions for SmartLogiLED application.
//
// This file contains registry functions, color management, and keyboard hook logic.

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_Logic.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <commdlg.h>

// External variables from main file
extern COLORREF capsLockColor;
extern COLORREF scrollLockColor; 
extern COLORREF numLockColor;
extern COLORREF offColor;
extern COLORREF defaultColor;
extern HHOOK keyboardHook;

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
void SaveColorsToRegistry() {
    SaveColorToRegistry(REGISTRY_VALUE_NUMLOCK_COLOR, numLockColor);
    SaveColorToRegistry(REGISTRY_VALUE_CAPSLOCK_COLOR, capsLockColor);
    SaveColorToRegistry(REGISTRY_VALUE_SCROLLLOCK_COLOR, scrollLockColor);
    SaveColorToRegistry(REGISTRY_VALUE_OFF_COLOR, offColor);
    SaveColorToRegistry(REGISTRY_VALUE_DEFAULT_COLOR, defaultColor);
}

// Load color settings from registry
void LoadColorsFromRegistry() {
    numLockColor = LoadColorFromRegistry(REGISTRY_VALUE_NUMLOCK_COLOR, RGB(0, 179, 0));
    capsLockColor = LoadColorFromRegistry(REGISTRY_VALUE_CAPSLOCK_COLOR, RGB(0, 179, 0));
    scrollLockColor = LoadColorFromRegistry(REGISTRY_VALUE_SCROLLLOCK_COLOR, RGB(0, 179, 0));
    offColor = LoadColorFromRegistry(REGISTRY_VALUE_OFF_COLOR, RGB(0, 89, 89));
    defaultColor = LoadColorFromRegistry(REGISTRY_VALUE_DEFAULT_COLOR, RGB(0, 89, 89));
}

// Set color for a key using Logitech LED SDK
void SetKeyColor(LogiLed::KeyName key, COLORREF color) {
    int r = GetRValue(color) * 100 / 255;
    int g = GetGValue(color) * 100 / 255;
    int b = GetBValue(color) * 100 / 255;
    LogiLedSetLightingForKeyWithKeyName(key, r, g, b);
}

// Set color for all keys
void SetDefaultColor(COLORREF color) {
    defaultColor = color;
    int r = GetRValue(defaultColor) * 100 / 255;
    int g = GetGValue(defaultColor) * 100 / 255;
    int b = GetBValue(defaultColor) * 100 / 255;
    LogiLedSetLighting(r, g, b);
}

// Set color for lock keys depending on their state
void SetLockKeysColor(void) {
    SHORT keyState;
    LogiLed::KeyName pressedKey;
    COLORREF colorToSet;

    // NumLock
    keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    pressedKey = LogiLed::KeyName::NUM_LOCK;
    colorToSet = (keyState == 0x0001) ? numLockColor : offColor;
    SetKeyColor(pressedKey, colorToSet);

    // ScrollLock
    keyState = GetKeyState(VK_SCROLL) & 0x0001;
    pressedKey = LogiLed::KeyName::SCROLL_LOCK;
    colorToSet = (keyState == 0x0001) ? scrollLockColor : offColor;
    SetKeyColor(pressedKey, colorToSet);

    // CapsLock
    keyState = GetKeyState(VK_CAPITAL) & 0x0001;
    pressedKey = LogiLed::KeyName::CAPS_LOCK;
    colorToSet = (keyState == 0x0001) ? capsLockColor : offColor;
    SetKeyColor(pressedKey, colorToSet);
}

// Show color picker dialog and update color
void ShowColorPicker(HWND hWnd, COLORREF& color, LogiLed::KeyName key) {
    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.rgbResult = color;
    COLORREF custColors[16] = {};
    cc.lpCustColors = custColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc)) {
        color = cc.rgbResult;
        if (key != LogiLed::KeyName::ESC) {
            SetKeyColor(key, color);
        }
        InvalidateRect(hWnd, NULL, TRUE);
    }
}

// Keyboard hook procedure to update lock key colors on key press
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;

        // Check if lock key was pressed
        if ((pKeyStruct->vkCode == VK_NUMLOCK) || (pKeyStruct->vkCode == VK_SCROLL) || (pKeyStruct->vkCode == VK_CAPITAL)) {
            SHORT keyState;
            LogiLed::KeyName pressedKey;
            COLORREF colorToSet;

            // The lock key state is updated only after this callback, so current state off means the key will be turned on
            switch (pKeyStruct->vkCode) {
            case VK_NUMLOCK:
                keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
                pressedKey = LogiLed::KeyName::NUM_LOCK;
                colorToSet = (keyState == 0x0000) ? numLockColor : offColor;
                break;
            case VK_SCROLL:
                keyState = GetKeyState(VK_SCROLL) & 0x0001;
                pressedKey = LogiLed::KeyName::SCROLL_LOCK;
                colorToSet = (keyState == 0x0000) ? scrollLockColor : offColor;
                break;
            case VK_CAPITAL:
                keyState = GetKeyState(VK_CAPITAL) & 0x0001;
                pressedKey = LogiLed::KeyName::CAPS_LOCK;
                colorToSet = (keyState == 0x0000) ? capsLockColor : offColor;
                break;
            }

            SetKeyColor(pressedKey, colorToSet);
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}