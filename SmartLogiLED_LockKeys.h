// SmartLogiLED_LockKeys.h : Header file for lock key functionality.
//

#pragma once

#include "framework.h"
#include "SmartLogiLED_Types.h"
#include "LogitechLEDLib.h"

// Lock key color management functions
void SetLockKeysColor(void);
void SetLockKeysColorWithProfile(AppColorProfile* displayedProfile); // Unsafe version for mutex-locked contexts
void HandleLockKeyPressed(DWORD vkCode, DWORD vkState);

// Keyboard hook management functions
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void UpdateKeyboardHookState();
void UpdateKeyboardHookStateUnsafe();
void EnableKeyboardHook();
void DisableKeyboardHook();
bool IsKeyboardHookEnabled();

// Lock keys feature status functions
bool IsLockKeysFeatureEnabled();
bool IsLockKeysFeatureEnabledUnsafe();

// Color management functions
void SetKeyColor(LogiLed::KeyName key, COLORREF color);
void SetDefaultColor(COLORREF color);
void SetHighlightKeysColor();
void SetHighlightKeysColorWithProfile(AppColorProfile* displayedProfile); // Unsafe version for mutex-locked contexts
void SetActionKeysColor();
void SetActionKeysColorWithProfile(AppColorProfile* displayedProfile);

// Main window handle management
void SetMainWindowHandle(HWND hWnd);void SetMainWindowHandle(HWND hWnd);