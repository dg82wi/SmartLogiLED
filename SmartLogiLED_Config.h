#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "LogitechLEDLib.h"
#include "SmartLogiLED_Types.h"
#include "SmartLogiLED_Version.h"
#include "SmartLogiLED_Constants.h"

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
void UpdateAppProfileActionColorInRegistry(const std::wstring& appName, COLORREF newActionColor);
void UpdateAppProfileLockKeysEnabledInRegistry(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);
void UpdateAppProfileActionKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& actionKeys);
