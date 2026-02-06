#pragma once

// Single instance mutex name
#define SMARTLOGILED_SINGLE_INSTANCE_MUTEX L"SmartLogiLED_SingleInstance_Mutex"

// Registry constants for registry operations
#define SMARTLOGILED_REGISTRY_ROOT      L"Software\\SmartLogiLED"
#define SMARTLOGILED_REGISTRY_PROFILES  L"Software\\SmartLogiLED\\AppProfiles"
#define REGISTRY_VALUE_START_MINIMIZED L"StartMinimized"
#define REGISTRY_VALUE_NUMLOCK_COLOR L"NumLockColor"
#define REGISTRY_VALUE_CAPSLOCK_COLOR L"CapsLockColor"
#define REGISTRY_VALUE_SCROLLLOCK_COLOR L"ScrollLockColor"
#define REGISTRY_VALUE_DEFAULT_COLOR L"DefaultColor"
#define REGISTRY_VALUE_APP_COLOR L"AppColor"
#define REGISTRY_VALUE_APP_HIGHLIGHT_COLOR L"AppHighlightColor"
#define REGISTRY_VALUE_APP_ACTION_COLOR L"AppActionColor"
#define REGISTRY_VALUE_LOCK_KEYS_ENABLED L"LockKeysEnabled"
#define REGISTRY_VALUE_HIGHLIGHT_KEYS L"HighlightKeys"
#define REGISTRY_VALUE_ACTION_KEYS L"ActionKeys"

// Monitoring interval for checking running applications (in milliseconds)
#define APP_MONITOR_INTERVAL_MS 1000

// enable/disable debug logging
//#define ENABLE_DEBUG_LOGGING