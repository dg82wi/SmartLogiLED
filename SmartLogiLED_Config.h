#pragma once
#include <windows.h>
#include <string>
#include "LogitechLEDLib.h"

// Registry constants
#define REGISTRY_KEY L"SOFTWARE\\SmartLogiLED"
#define REGISTRY_VALUE_START_MINIMIZED L"StartMinimized"
#define REGISTRY_VALUE_NUMLOCK_COLOR L"NumLockColor"
#define REGISTRY_VALUE_CAPSLOCK_COLOR L"CapsLockColor"
#define REGISTRY_VALUE_SCROLLLOCK_COLOR L"ScrollLockColor"
#define REGISTRY_VALUE_DEFAULT_COLOR L"DefaultColor"
#define REGISTRY_KEY_APP_PROFILES_SUBKEY L"SOFTWARE\\SmartLogiLED\\AppProfiles"
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
void SaveAppProfilesToRegistry();
void LoadAppProfilesFromRegistry();
size_t GetAppProfilesCount();
