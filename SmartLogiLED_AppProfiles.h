// SmartLogiLED_AppProfiles.h : Header file for app profile functionality.
//

#pragma once

#include "framework.h"
#include "SmartLogiLED_Types.h"
#include <vector>
#include <string>

// Enum to specify which color property to update
enum class ColorUpdateType {
    AppColor,
    HighlightColor,
    ActionColor
};

// App profile management functions
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);
void RemoveAppColorProfile(const std::wstring& appName);
void CheckRunningAppsAndUpdateColors();
std::vector<AppColorProfile> GetAppColorProfilesCopy();
AppColorProfile* GetDisplayedProfile();
AppColorProfile* GetAppProfileByName(const std::wstring& appName);

// Generic function to update any color property of an app profile
void UpdateAppProfileColorProperty(const std::wstring& appName, COLORREF newColor, ColorUpdateType colorType);

// App profile update functions - These have been consolidated into a generic function internally
// but maintain the same public API for backward compatibility
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);
void UpdateAppProfileActionKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& actionKeys);

// Message handlers for app monitoring
void HandleAppStarted(const std::wstring& appName);
void HandleAppStopped(const std::wstring& appName);

// Activation history functions
std::vector<std::wstring> GetActivationHistory();

// Public API functions (for external compatibility)
void UpdateActivationHistory(const std::wstring& profileName);
AppColorProfile* FindBestFallbackProfile(const std::wstring& excludeProfile = L"");
void CleanupActivationHistory();

// Window handle management for app profiles
void SetAppProfileMainWindowHandle(HWND hWnd);

// Helper functions
COLORREF GetDefaultColor();

// DEPRECATED functions (for backward compatibility - will be removed in future versions)
AppColorProfile* GetDisplayedProfileUnsafe(); // Use GetDisplayedProfile() instead