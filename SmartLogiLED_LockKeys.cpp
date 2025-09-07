// SmartLogiLED_LockKeys.cpp : Contains lock key and color management functionality.
//

#include "framework.h"
#include "SmartLogiLED.h"
#include "SmartLogiLED_LockKeys.h"
#include "SmartLogiLED_AppProfiles.h"
#include "LogitechLEDLib.h"
#include "Resource.h"
#include <commdlg.h>
#include <sstream>

// External variables from main file
extern COLORREF capsLockColor;
extern COLORREF scrollLockColor; 
extern COLORREF numLockColor;
extern COLORREF defaultColor;
extern HHOOK keyboardHook;

// Custom window message for lock key press handling
#define WM_LOCK_KEY_PRESSED (WM_USER + 101)

// Hook management variables
static bool isKeyboardHookEnabled = false;
static HWND mainWindowHandle = nullptr;

// Set color for a key using Logitech LED SDK
void SetKeyColor(LogiLed::KeyName key, COLORREF color) {
    int r = GetRValue(color) * 100 / 255;
    int g = GetGValue(color) * 100 / 255;
    int b = GetBValue(color) * 100 / 255;
    LogiLedSetLightingForKeyWithKeyName(key, r, g, b);
}

// Set color for all keys
void SetDefaultColor(COLORREF color) {
    int r = GetRValue(color) * 100 / 255;
    int g = GetGValue(color) * 100 / 255;
    int b = GetBValue(color) * 100 / 255;
    LogiLedSetLighting(r, g, b);
}

// Set color for lock keys depending on their state (only if lock keys feature is enabled)
void SetLockKeysColor(void) {
    AppColorProfile* displayedProfile = GetDisplayedProfile();
    SetLockKeysColorWithProfile(displayedProfile);
}

// Set color for lock keys with a specific profile (unsafe version - doesn't acquire mutex)
void SetLockKeysColorWithProfile(AppColorProfile* displayedProfile) {
    SHORT keyState;
    LogiLed::KeyName pressedKey;
    COLORREF colorToSet;
    
    // Determine the color to use for "off" state - app color if profile is active, otherwise default color
    COLORREF offStateColor = defaultColor;
    if (displayedProfile) {
        offStateColor = displayedProfile->appColor;
    }

    // NumLock
    keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    pressedKey = LogiLed::KeyName::NUM_LOCK;
    colorToSet = (keyState == 0x0001) ? numLockColor : offStateColor;
    SetKeyColor(pressedKey, colorToSet);

    // ScrollLock
    keyState = GetKeyState(VK_SCROLL) & 0x0001;
    pressedKey = LogiLed::KeyName::SCROLL_LOCK;
    colorToSet = (keyState == 0x0001) ? scrollLockColor : offStateColor;
    SetKeyColor(pressedKey, colorToSet);

    // CapsLock
    keyState = GetKeyState(VK_CAPITAL) & 0x0001;
    pressedKey = LogiLed::KeyName::CAPS_LOCK;
    colorToSet = (keyState == 0x0001) ? capsLockColor : offStateColor;
    SetKeyColor(pressedKey, colorToSet);
}

// Set highlight color for keys from the currently active profile
void SetHighlightKeysColor() {
    AppColorProfile* displayedProfile = GetDisplayedProfile();
    SetHighlightKeysColorWithProfile(displayedProfile);
}

// Set highlight color for keys with a specific profile (unsafe version - doesn't acquire mutex)
void SetHighlightKeysColorWithProfile(AppColorProfile* displayedProfile) {
    if (!displayedProfile) return;
    
    // Apply highlight color to all keys in the highlight list
    for (const auto& key : displayedProfile->highlightKeys) {
        // Check if this is a lock key and if lock keys are enabled
        bool isLockKey = (key == LogiLed::KeyName::NUM_LOCK || 
                        key == LogiLed::KeyName::CAPS_LOCK || 
                        key == LogiLed::KeyName::SCROLL_LOCK);
        
        if (isLockKey && displayedProfile->lockKeysEnabled) {
            // For lock keys, check their state and apply appropriate color
            SHORT keyState = 0;
            COLORREF lockColor = defaultColor;
            
            if (key == LogiLed::KeyName::NUM_LOCK) {
                keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
                lockColor = (keyState == 0x0001) ? numLockColor : defaultColor;
            } else if (key == LogiLed::KeyName::CAPS_LOCK) {
                keyState = GetKeyState(VK_CAPITAL) & 0x0001;
                lockColor = (keyState == 0x0001) ? capsLockColor : defaultColor;
            } else if (key == LogiLed::KeyName::SCROLL_LOCK) {
                keyState = GetKeyState(VK_SCROLL) & 0x0001;
                lockColor = (keyState == 0x0001) ? scrollLockColor : defaultColor;
            }
            
            // Lock key color takes precedence over highlight color when lock is active
            if (keyState == 0x0001) {
                SetKeyColor(key, lockColor);
            } else {
                SetKeyColor(key, displayedProfile->appHighlightColor);
            }
        } else {
            // For non-lock keys or when lock keys are disabled, apply highlight color
            SetKeyColor(key, displayedProfile->appHighlightColor);
        }
    }
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

// Keyboard hook procedure to handle lock key presses
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;

        // Check if lock key was pressed
        if ((pKeyStruct->vkCode == VK_NUMLOCK) || (pKeyStruct->vkCode == VK_SCROLL) || (pKeyStruct->vkCode == VK_CAPITAL)) {
            // Send message to main window to handle lock key press
            if (mainWindowHandle) {
                PostMessage(mainWindowHandle, WM_LOCK_KEY_PRESSED, pKeyStruct->vkCode, 0);
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Handle lock key press in main thread
void HandleLockKeyPressed(DWORD vkCode) {
    // Determine the color to use for "off" state - app color if profile is active, otherwise default color
    COLORREF offStateColor = defaultColor;
    AppColorProfile* displayedProfile = GetDisplayedProfile();
    if (displayedProfile) {
        offStateColor = displayedProfile->appColor;
    }

    // Check if lock keys feature is enabled
    if (!IsLockKeysFeatureEnabled()) {
        LogiLed::KeyName pressedKey;
        switch (vkCode) {
        case VK_NUMLOCK:
            pressedKey = LogiLed::KeyName::NUM_LOCK;
            break;
        case VK_SCROLL:
            pressedKey = LogiLed::KeyName::SCROLL_LOCK;
            break;
        case VK_CAPITAL:
            pressedKey = LogiLed::KeyName::CAPS_LOCK;
            break;
        default:
            return; // Unknown key
        }
        SetKeyColor(pressedKey, offStateColor);
        return;
    }

    SHORT keyState;
    LogiLed::KeyName pressedKey;
    COLORREF colorToSet;

    // The lock key state is updated only after this callback, so current state off means the key will be turned on
    switch (vkCode) {
    case VK_NUMLOCK:
        keyState = GetKeyState(VK_NUMLOCK) & 0x0001;
        pressedKey = LogiLed::KeyName::NUM_LOCK;
        colorToSet = (keyState == 0x0000) ? numLockColor : offStateColor;
        break;
    case VK_SCROLL:
        keyState = GetKeyState(VK_SCROLL) & 0x0001;
        pressedKey = LogiLed::KeyName::SCROLL_LOCK;
        colorToSet = (keyState == 0x0000) ? scrollLockColor : offStateColor;
        break;
    case VK_CAPITAL:
        keyState = GetKeyState(VK_CAPITAL) & 0x0001;
        pressedKey = LogiLed::KeyName::CAPS_LOCK;
        colorToSet = (keyState == 0x0000) ? capsLockColor : offStateColor;
        break;
    default:
        return; // Unknown key
    }

    SetKeyColor(pressedKey, colorToSet);
    
    // Also reapply highlight keys in case the pressed key is in the highlight list
    SetHighlightKeysColorWithProfile(displayedProfile); // Use safe version since this doesn't hold the mutex
}

// Hook management functions

// Check if keyboard hook is currently enabled
bool IsKeyboardHookEnabled() {
    return isKeyboardHookEnabled;
}

// Enable the keyboard hook
void EnableKeyboardHook() {
    if (!isKeyboardHookEnabled && !keyboardHook) {
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
        if (keyboardHook) {
            isKeyboardHookEnabled = true;
            OutputDebugStringW(L"[DEBUG] Keyboard hook enabled\n");
        } else {
            OutputDebugStringW(L"[DEBUG] Failed to enable keyboard hook\n");
        }
    }
}

// Disable the keyboard hook
void DisableKeyboardHook() {
    if (isKeyboardHookEnabled && keyboardHook) {
        if (UnhookWindowsHookEx(keyboardHook)) {
            keyboardHook = nullptr;
            isKeyboardHookEnabled = false;
            OutputDebugStringW(L"[DEBUG] Keyboard hook disabled\n");
        } else {
            OutputDebugStringW(L"[DEBUG] Failed to disable keyboard hook\n");
        }
    }
}

// Update hook state based on lock keys feature
void UpdateKeyboardHookState() {
    bool lockKeysEnabled = IsLockKeysFeatureEnabled();
    
    if (lockKeysEnabled && !isKeyboardHookEnabled) {
        // Lock keys feature is enabled but hook is disabled - enable it
        EnableKeyboardHook();
        OutputDebugStringW(L"[DEBUG] Keyboard hook enabled due to lock keys feature\n");
    } else if (!lockKeysEnabled && isKeyboardHookEnabled) {
        // Lock keys feature is disabled but hook is enabled - disable it
        DisableKeyboardHook();
        OutputDebugStringW(L"[DEBUG] Keyboard hook disabled due to lock keys feature being disabled\n");
    }
}

// Update hook state based on lock keys feature (unsafe version - assumes mutex already held)
void UpdateKeyboardHookStateUnsafe() {
    bool lockKeysEnabled = IsLockKeysFeatureEnabledUnsafe();
    
    if (lockKeysEnabled && !isKeyboardHookEnabled) {
        // Lock keys feature is enabled but hook is disabled - enable it
        EnableKeyboardHook();
        OutputDebugStringW(L"[DEBUG] Keyboard hook enabled due to lock keys feature\n");
    } else if (!lockKeysEnabled && isKeyboardHookEnabled) {
        // Lock keys feature is disabled but hook is enabled - disable it
        DisableKeyboardHook();
        OutputDebugStringW(L"[DEBUG] Keyboard hook disabled due to lock keys feature being disabled\n");
    }
}

// Check if lock keys feature should be enabled based on current displayed profile
bool IsLockKeysFeatureEnabled() {
    AppColorProfile* displayedProfile = GetDisplayedProfile();
    if (displayedProfile) {
        return displayedProfile->lockKeysEnabled;
    }
    
    // If no profile is displayed, lock keys feature is always enabled (default behavior)
    return true;
}

// Internal version that assumes mutex is already locked
bool IsLockKeysFeatureEnabledUnsafe() {
    // Note: This function assumes the appProfilesMutex is already locked by the caller
    // We cannot call GetAppColorProfilesCopy() here as it would try to acquire the mutex again
    
    // This function should only be called from contexts where we already have the mutex
    // and need to access the profile data directly. However, since this is in LockKeys module
    // and we don't have direct access to appColorProfiles, we need a different approach.
    
    // For now, fall back to the safe version since this function is problematic
    // TODO: This should be refactored to not be needed at all
    return IsLockKeysFeatureEnabled();
}

// Set main window handle for UI updates
void SetMainWindowHandle(HWND hWnd) {
    mainWindowHandle = hWnd;
    // Also set it for app profiles
    SetAppProfileMainWindowHandle(hWnd);
}