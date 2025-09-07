#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "LogitechLEDLib.h"
#include "SmartLogiLED_Types.h"
#include "SmartLogiLED_Version.h"

// Registry constants (now using centralized constants from Version.h)
#define REGISTRY_KEY SMARTLOGILED_REGISTRY_ROOT
#define REGISTRY_VALUE_START_MINIMIZED L"StartMinimized"
#define REGISTRY_VALUE_NUMLOCK_COLOR L"NumLockColor"
#define REGISTRY_VALUE_CAPSLOCK_COLOR L"CapsLockColor"
#define REGISTRY_VALUE_SCROLLLOCK_COLOR L"ScrollLockColor"
#define REGISTRY_VALUE_DEFAULT_COLOR L"DefaultColor"
#define REGISTRY_KEY_APP_PROFILES_SUBKEY SMARTLOGILED_REGISTRY_PROFILES
#define REGISTRY_VALUE_APP_COLOR L"AppColor"
#define REGISTRY_VALUE_APP_HIGHLIGHT_COLOR L"AppHighlightColor"
#define REGISTRY_VALUE_LOCK_KEYS_ENABLED L"LockKeysEnabled"
#define REGISTRY_VALUE_HIGHLIGHT_KEYS L"HighlightKeys"



// Registry functions for start minimized setting
void SaveStartMinimizedSetting(bool minimized);
bool LoadStartMinimizedSetting();

// Registry functions for color settings
void SaveColorToRegistry(LPCWSTR valueName, COLORREF color);
COLORREF LoadColorFromRegistry(LPCWSTR valueName, COLORREF defaultValue);
void SaveLockKeyColorsToRegistry();
void LoadLockKeyColorsFromRegistry();

// Registry persistence for app profiles (including highlight color and keys)
void AddAppProfileToRegistry(const AppColorProfile& profile);
void RemoveAppProfileFromRegistry(const std::wstring& appName);
void SaveAppProfilesToRegistry();
void LoadAppProfilesFromRegistry();
size_t GetAppProfilesCount();

// Update specific app profile colors in registry
void UpdateAppProfileColorInRegistry(const std::wstring& appName, COLORREF newAppColor);
void UpdateAppProfileHighlightColorInRegistry(const std::wstring& appName, COLORREF newHighlightColor);
void UpdateAppProfileLockKeysEnabledInRegistry(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);

// Export functionality
void ExportAllProfilesToIniFiles();

// Import functionality
void ImportProfileFromIniFile(HWND hWnd);
