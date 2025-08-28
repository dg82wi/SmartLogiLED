# SmartLogiLED App Monitoring Usage Guide

## Overview
The SmartLogiLED application now includes functionality to detect when specific applications are started by Windows and automatically change the keyboard lighting colors based on the running application. Additionally, each app profile can control whether the lock keys feature is enabled or disabled.

## How It Works
- The app monitoring runs in a background thread that checks for running processes every 2 seconds
- When a monitored application starts, the keyboard lighting changes to the configured color for that app
- When a monitored application stops, the lighting reverts to the default color (unless another monitored app is still running)
- **Lock Keys Feature Control**: Each app profile can enable or disable the lock keys highlighting feature
- **Default Behavior**: When no app profile is active, lock keys feature is always enabled

## Lock Key Behavior

### When Lock Keys Feature is Enabled (Default)
- **Lock key ON**: Uses the specific lock key color (e.g., green for NumLock)
- **Lock key OFF**: Uses the default color (same as all other keys)

### When Lock Keys Feature is Disabled (Per App Profile)
- **All lock keys**: Always use the default color regardless of their state
- **No highlighting**: Lock key state changes don't affect keyboard lighting

### Feature Control Logic
- **No active profile**: Lock keys feature is enabled (default behavior)
- **Active profile exists**: Uses the `lockKeysEnabled` flag from that profile
- **Multiple profiles**: The first active profile determines the lock keys behavior

## Adding App Color Profiles

### Basic Profile (Lock Keys Enabled by Default)
```cpp
AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0));      // Yellow for Notepad, lock keys enabled
```

### Profile with Lock Keys Control
```cpp
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);   // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);        // Green for VS Code, lock keys enabled
```

### Current Examples in Code
```cpp
AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0), true);      // Yellow for Notepad, lock keys enabled
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);     // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"firefox.exe", RGB(255, 165, 0), false);    // Orange for Firefox, lock keys disabled  
AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);          // Green for VS Code, lock keys enabled
AddAppColorProfile(L"devenv.exe", RGB(128, 0, 128), true);      // Purple for Visual Studio, lock keys enabled
```

## Available Functions

### Core Functions
- `InitializeAppMonitoring()` - Start the monitoring thread
- `CleanupAppMonitoring()` - Stop the monitoring thread
- `IsAppRunning(const std::wstring& appName)` - Check if an app is currently running
- `IsLockKeysFeatureEnabled()` - Check if lock keys feature should be enabled based on current active profile

### Profile Management
- `AddAppColorProfile(appName, color)` - Add profile with lock keys enabled (backward compatible)
- `AddAppColorProfile(appName, color, lockKeysEnabled)` - Add profile with lock keys control
- `RemoveAppColorProfile(appName)` - Remove an app profile
- `SetAppColorProfile(appName, color, active)` - Update profile state (backward compatible)
- `SetAppColorProfile(appName, color, active, lockKeysEnabled)` - Update profile with lock keys control
- `GetAppColorProfiles()` - Get reference to profiles (not thread-safe)
- `GetAppColorProfilesCopy()` - Get copy of profiles (thread-safe)

### Manual Updates
- `CheckRunningAppsAndUpdateColors()` - Immediately check and update colors

## AppColorProfile Structure
```cpp
struct AppColorProfile {
    std::wstring appName;       // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor;          // Color to set when app starts
    bool isActive;              // Whether this profile is currently active
    bool lockKeysEnabled;       // Whether lock keys feature is enabled for this profile (default: true)
};
```

## Use Cases

### Gaming Applications
Disable lock keys for games to prevent accidental highlighting:
```cpp
AddAppColorProfile(L"game.exe", RGB(255, 0, 0), false);   // Red for games, no lock key distractions
```

### Development Tools
Keep lock keys enabled for coding applications where Caps Lock/Num Lock status is important:
```cpp
AddAppColorProfile(L"devenv.exe", RGB(0, 255, 0), true);  // Green for Visual Studio, lock keys enabled
```

### Web Browsers
Disable lock keys for clean browsing experience:
```cpp
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);  // Cyan for Chrome, lock keys disabled
```

## Color Configuration
The application now uses a simplified color scheme with app-specific lock key control:
- **Default Color**: Used for all regular keys and inactive lock keys (when lock keys enabled)
- **Lock Key Colors**: Individual colors for NumLock, CapsLock, and ScrollLock when active (only when lock keys enabled)
- **App Colors**: Custom colors that activate when specific applications are running
- **Lock Keys Control**: Per-app setting to enable/disable lock key highlighting

## Customization
To add your own app profiles:

1. Find the executable name of your application (e.g., "myapp.exe")
2. Choose an RGB color (e.g., RGB(255, 0, 0) for red)
3. Decide if you want lock keys enabled or disabled for this app
4. Add the line: `AddAppColorProfile(L"myapp.exe", RGB(255, 0, 0), true);` (true = lock keys enabled, false = disabled)

## Thread Safety
The app monitoring uses thread-safe operations with mutexes to prevent race conditions between the monitoring thread and the main application thread.

## Performance
- Monitoring checks occur every 2 seconds to balance responsiveness with system performance
- Process enumeration is efficient and should not impact system performance significantly
- Lock keys feature checking is optimized and only occurs when needed