// SmartLogiLED_AppProfiles.h : Header file for app profile functionality.
//

#pragma once

#include "framework.h"
#include "SmartLogiLED_Types.h"
#include <vector>
#include <string>

// App profile management functions
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);
void RemoveAppColorProfile(const std::wstring& appName);
void CheckRunningAppsAndUpdateColors();
std::vector<AppColorProfile> GetAppColorProfilesCopy();
AppColorProfile* GetDisplayedProfile();
AppColorProfile* GetAppProfileByName(const std::wstring& appName);

// App profile update functions
void UpdateAppProfileColor(const std::wstring& appName, COLORREF newAppColor);
// Update app profile highlight color
void UpdateAppProfileHighlightColor(const std::wstring& appName, COLORREF newHighlightColor);
// Update app profile action color
void UpdateAppProfileActionColor(const std::wstring& appName, COLORREF newActionColor);
// Update app profile lock keys enabled setting
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled);
// Update app profile highlight keys
void UpdateAppProfileHighlightKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);
// Update app profile action keys
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