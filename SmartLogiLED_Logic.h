// SmartLogiLED_Logic.h : Header file for SmartLogiLED logic functions.
//

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"

// Registry constants
#define REGISTRY_KEY L"SOFTWARE\\SmartLogiLED"
#define REGISTRY_VALUE_START_MINIMIZED L"StartMinimized"
#define REGISTRY_VALUE_NUMLOCK_COLOR L"NumLockColor"
#define REGISTRY_VALUE_CAPSLOCK_COLOR L"CapsLockColor"
#define REGISTRY_VALUE_SCROLLLOCK_COLOR L"ScrollLockColor"
#define REGISTRY_VALUE_OFF_COLOR L"OffColor"
#define REGISTRY_VALUE_DEFAULT_COLOR L"DefaultColor"

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

// Keyboard hook procedure
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);