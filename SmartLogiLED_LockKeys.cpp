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
    // Determine the color to use for "off" state - app color if profile is active, otherwise default color
    COLORREF offStateColor = defaultColor;
    if (displayedProfile) {
        offStateColor = displayedProfile->appColor;
    }

    // Only apply lock key colors if the feature is enabled for the current context
    bool lockKeysActive = !displayedProfile || displayedProfile->lockKeysEnabled;
    if (!lockKeysActive) {
        // If disabled for the profile, ensure lock keys have the base app color
        SetKeyColor(LogiLed::KeyName::NUM_LOCK, offStateColor);
        SetKeyColor(LogiLed::KeyName::CAPS_LOCK, offStateColor);
        SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, offStateColor);
        return;
    }

    // NumLock
    SHORT keyStateNum = GetKeyState(VK_NUMLOCK) & 0x0001;
    SetKeyColor(LogiLed::KeyName::NUM_LOCK, (keyStateNum == 0x0001) ? numLockColor : offStateColor);

    // CapsLock
    SHORT keyStateCaps = GetKeyState(VK_CAPITAL) & 0x0001;
    SetKeyColor(LogiLed::KeyName::CAPS_LOCK, (keyStateCaps == 0x0001) ? capsLockColor : offStateColor);

    // ScrollLock
    SHORT keyStateScroll = GetKeyState(VK_SCROLL) & 0x0001;
    SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, (keyStateScroll == 0x0001) ? scrollLockColor : offStateColor);
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
        SetKeyColor(key, displayedProfile->appHighlightColor);
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
        // The caller is responsible for updating the key color and UI.
        // This makes the function's purpose clearer.
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
                // Get the key state *before* this press toggles it. The low-order bit is the toggle state.
                SHORT keyState = GetKeyState(pKeyStruct->vkCode);
                // send inverted state as Lock Key state toogles only after windows handling  
                LPARAM state = !(keyState & 0x0001); // 1 if toggled on, 0 if off
                PostMessage(mainWindowHandle, WM_LOCK_KEY_PRESSED, pKeyStruct->vkCode, state);
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Handle lock key press in main thread
void HandleLockKeyPressed(DWORD vkCode, DWORD vkState) {
    
    // Check if lock keys feature is enabled
    if (!IsLockKeysFeatureEnabled()) {
        // Lock keys feature is disabled - keep lock keys at the app color
        AppColorProfile* displayedProfile = GetDisplayedProfile();
        COLORREF offStateColor = defaultColor;
        if (displayedProfile) {
            offStateColor = displayedProfile->appColor;
        }
        
        LogiLed::KeyName lockKey;
        switch (vkCode) {
        case VK_NUMLOCK:
            lockKey = LogiLed::KeyName::NUM_LOCK;
            break;
        case VK_CAPITAL:
            lockKey = LogiLed::KeyName::CAPS_LOCK;
            break;
        case VK_SCROLL:
            lockKey = LogiLed::KeyName::SCROLL_LOCK;
            break;
        default:
            return; // Unknown key
        }
        SetKeyColor(lockKey, offStateColor);
        return;
    }

    // Lock keys feature is enabled - apply lock key colors based on state
    AppColorProfile* displayedProfile = GetDisplayedProfile();
    COLORREF offStateColor = defaultColor;
    if (displayedProfile) {
        offStateColor = displayedProfile->appColor;
    }

    switch (vkCode) {
        case VK_NUMLOCK:
            if (vkState == 0x0001) {
                // Key is now ON
                SetKeyColor(LogiLed::KeyName::NUM_LOCK, numLockColor);
            } else {
                // Key is now OFF
                SetKeyColor(LogiLed::KeyName::NUM_LOCK, offStateColor);
            }
            break;
        case VK_CAPITAL:
            if (vkState == 0x0001) {
                // Key is now ON
                SetKeyColor(LogiLed::KeyName::CAPS_LOCK, capsLockColor);
            } else {
                // Key is now OFF
                SetKeyColor(LogiLed::KeyName::CAPS_LOCK, offStateColor);
            }
            break;
        case VK_SCROLL:
            if (vkState == 0x0001) {
                // Key is now ON
                SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, scrollLockColor);
            } else {
                // Key is now OFF
                SetKeyColor(LogiLed::KeyName::SCROLL_LOCK, offStateColor);
            }
            break;
        default:
            return; // Not a lock key, ignore
	}

    // Re-apply highlight colors in case a lock key was also a highlight key.
    // The lock color will correctly take precedence.
    SetHighlightKeysColor();
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