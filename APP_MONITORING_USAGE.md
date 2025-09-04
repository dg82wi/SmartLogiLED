# SmartLogiLED App Monitoring Usage Guide

## Overview
The SmartLogiLED application includes advanced functionality to detect when specific applications are started by Windows and automatically change the keyboard lighting colors based on the running application. The system features intelligent priority handling with most recently activated app taking precedence, and each app profile can control whether the lock keys feature is enabled or disabled. All profiles are automatically persisted to the Windows registry.

## How It Works
- The app monitoring runs in a background thread that checks for running processes every 2 seconds
- When a monitored application starts, the keyboard lighting changes to the configured color for that app
- **Most Recently Activated Priority**: The most recently started monitored app takes control of lighting
- When a monitored application stops, the lighting intelligently hands off to the most recently activated profile that's still running
- **Lock Keys Feature Control**: Each app profile can enable or disable the lock keys highlighting feature
- **Default Behavior**: When no app profile is active, lock keys feature is always enabled
- **Persistent Storage**: All app profiles are automatically saved to and loaded from the Windows registry

## Enhanced Profile State Management

### Dual State Tracking
Each app profile maintains two separate boolean states:
- **`isAppRunning`**: Whether the application process is currently running and visible
- **`isProfileCurrentlyInUse`**: Whether this profile is currently controlling the keyboard colors

### Most Recently Activated Logic
- **`lastActivatedProfile`**: Tracks which profile was most recently activated
- **Priority System**: When multiple apps are running, the most recently activated takes precedence
- **Fallback Behavior**: When the active app stops, control passes to the most recently activated profile that's still running

### Debug Logging
Comprehensive debug output is available via `OutputDebugStringW` for troubleshooting:
```cpp
// Example debug messages
"[DEBUG] App started: notepad.exe - isAppRunning changed to TRUE"
"[DEBUG] Most recently activated profile updated to: notepad.exe"
"[DEBUG] Profile chrome.exe - isProfileCurrentlyInUse changed to FALSE (handoff to most recent)"
"[DEBUG] Profile notepad.exe - isProfileCurrentlyInUse changed to TRUE (most recently activated)"
```

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
- **Multiple profiles**: The currently displayed profile (most recently activated) determines the lock keys behavior

## Multiple Application Handoff

### Intelligent App Switching with Priority
When multiple monitored applications are running simultaneously:
- **Most Recently Activated Priority**: The app that was started most recently takes control
- **Automatic Handoff**: When the active app stops, control passes to the most recently activated profile that's still running
- **Fallback Chain**: If the most recently activated profile is not running, falls back to any other running profile
- **Default Restoration**: If no monitored apps remain active, returns to the user's default color

### Example Scenario
1. **Start Chrome** (cyan color, lock keys disabled) ? Keyboard becomes cyan, lock keys disabled, `lastActivatedProfile = "chrome.exe"`
2. **Start VS Code** (green color, lock keys enabled) ? Keyboard becomes green, lock keys enabled, `lastActivatedProfile = "code.exe"` (most recent takes control)
3. **Start Notepad** (yellow color, lock keys enabled) ? Keyboard becomes yellow, lock keys enabled, `lastActivatedProfile = "notepad.exe"` (most recent takes control)
4. **Close Notepad** ? Keyboard returns to VS Code colors (green, lock keys enabled) since it's the most recently activated of the remaining apps
5. **Close VS Code** ? Keyboard returns to Chrome colors (cyan, lock keys disabled)
6. **Close Chrome** ? Keyboard returns to user's default color, lock keys enabled

### Debug Output Example
```
[DEBUG] App started: code.exe - isAppRunning changed to TRUE
[DEBUG] Most recently activated profile updated to: code.exe
[DEBUG] Profile chrome.exe - isProfileCurrentlyInUse changed to FALSE (handoff to most recent)
[DEBUG] Profile code.exe - isProfileCurrentlyInUse changed to TRUE (most recently activated)
```

## App Profile Structure

### Enhanced AppColorProfile
```cpp
struct AppColorProfile {
    std::wstring appName;                           // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255);          // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for future UI features
    bool isAppRunning = false;                      // Whether this app is currently running and visible
    bool isProfileCurrentlyInUse = false;          // Whether this profile currently controls keyboard colors
    bool lockKeysEnabled = true;                    // Whether lock keys feature is enabled (default: true)
    std::vector<LogiLed::KeyName> highlightKeys;   // Future: specific keys to highlight
};
```

## Registry Persistence

### Automatic Storage
- **Location**: `HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles`
- **Per-App Subkeys**: Each application gets its own registry subkey
- **Automatic Save**: Profiles are saved when the application exits
- **Automatic Load**: Profiles are restored when the application starts

### Registry Structure
```
HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles\
??? notepad.exe\
?   ??? AppColor (DWORD): RGB color value
?   ??? AppHighlightColor (DWORD): Highlight color value  
?   ??? LockKeysEnabled (DWORD): 1 = enabled, 0 = disabled
?   ??? HighlightKeys (BINARY): Array of key codes (future use)
??? chrome.exe\
?   ??? AppColor (DWORD): RGB color value
?   ??? ...
```

## Core Functions

### App Monitoring Functions
```cpp
// Initialize and manage monitoring
void InitializeAppMonitoring();                    // Start the monitoring thread
void CleanupAppMonitoring();                      // Stop the monitoring thread

// Query functions
bool IsAppRunning(const std::wstring& appName);   // Check if an app is currently running
bool IsLockKeysFeatureEnabled();                  // Check if lock keys should be enabled
std::wstring GetLastActivatedProfileName();        // Get the most recently activated profile name

// Manual updates
void CheckRunningAppsAndUpdateColors();           // Immediately scan and update colors
```

### Profile Management Functions
```cpp
// Add/Update profiles (backward compatible)
void AddAppColorProfile(const std::wstring& appName, COLORREF color);
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);

// Profile access (thread-safe)
std::vector<AppColorProfile> GetAppColorProfilesCopy();  // Get thread-safe copy of all profiles
AppColorProfile* GetDisplayedProfile();                   // Get currently active/displayed profile

// Remove profiles
void RemoveAppColorProfile(const std::wstring& appName);

// UI integration
void SetMainWindowHandle(HWND hWnd);               // Set handle for UI update messages
```

### Enhanced Profile Queries
```cpp
// Get the profile that's currently controlling colors
AppColorProfile* GetDisplayedProfile() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    for (auto& profile : appColorProfiles) {
        if (profile.isProfileCurrentlyInUse) {
            return &profile;
        }
    }
    return nullptr; // No profile is currently displayed
}

// Get the name of the most recently activated profile
std::wstring GetLastActivatedProfileName() {
    std::lock_guard<std::mutex> lock(appProfilesMutex);
    return lastActivatedProfile;
}
```

## Adding App Color Profiles

### Function Signatures
```cpp
// Basic profile (lock keys enabled by default, backward compatible)
void AddAppColorProfile(const std::wstring& appName, COLORREF color);

// Full profile with lock keys control
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);
```

### Examples

#### Basic Profiles (Lock Keys Enabled by Default)
```cpp
AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0));      // Yellow for Notepad, lock keys enabled
AddAppColorProfile(L"calculator.exe", RGB(0, 255, 0));     // Green for Calculator, lock keys enabled
```

#### Profiles with Lock Keys Control
```cpp
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);   // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);        // Green for VS Code, lock keys enabled
AddAppColorProfile(L"game.exe", RGB(255, 0, 0), false);       // Red for games, lock keys disabled
```

### Current Predefined Profiles
```cpp
AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0), true);      // Yellow for Notepad, lock keys enabled
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);     // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"firefox.exe", RGB(255, 165, 0), false);    // Orange for Firefox, lock keys disabled  
AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);          // Green for VS Code, lock keys enabled
AddAppColorProfile(L"devenv.exe", RGB(128, 0, 128), true);      // Purple for Visual Studio, lock keys enabled
```

## Architecture Improvements

### Separated Configuration Module
- **SmartLogiLED_Config.cpp**: All registry-related functions
- **SmartLogiLED_Config.h**: Configuration function declarations
- **SmartLogiLED_Logic.cpp**: Core application logic and monitoring
- **SmartLogiLED_Logic.h**: Logic function declarations

### Color Management Fixes
- **Fixed Issue**: `SetDefaultColor()` no longer mutates the global `defaultColor` variable
- **Behavior**: User's base default color is preserved when apps activate/deactivate
- **Result**: Proper color restoration when returning from app-specific colors

### Thread Safety Enhancements
- **Mutex Protection**: All app profile operations are thread-safe with `appProfilesMutex`
- **Safe Access**: Use `GetAppColorProfilesCopy()` for thread-safe profile access
- **Concurrent Operations**: Registry operations are safely isolated from monitoring thread
- **Atomic State Changes**: Profile state changes are properly synchronized

### Enhanced Monitoring Logic
```cpp
// Improved app monitoring with most recently activated priority
void AppMonitorThreadProc() {
    std::vector<std::wstring> lastRunningApps;
    
    while (appMonitoringRunning) {
        // Check for newly started apps
        for (const auto& app : currentRunningApps) {
            if (std::find(lastRunningApps.begin(), lastRunningApps.end(), app) == lastRunningApps.end()) {
                // New app detected - update lastActivatedProfile and give it control
                lastActivatedProfile = profile.appName;
                profile.isProfileCurrentlyInUse = true;
                // Clear other profiles' display flags
                // Apply colors
            }
        }
        
        // Check for stopped apps with intelligent handoff
        for (const auto& app : lastRunningApps) {
            if (std::find(currentRunningApps.begin(), currentRunningApps.end(), app) == currentRunningApps.end()) {
                // App stopped - find replacement based on most recently activated
                // Fallback to any other running profile if needed
            }
        }
    }
}
```

## Use Cases

### Gaming Applications
Disable lock keys for games to prevent accidental highlighting and distractions:
```cpp
AddAppColorProfile(L"game.exe", RGB(255, 0, 0), false);   // Red for games, no lock key distractions
AddAppColorProfile(L"steam.exe", RGB(100, 149, 237), false); // Steel blue for Steam, clean interface
```

### Development Tools
Keep lock keys enabled for coding applications where Caps Lock/Num Lock status is important:
```cpp
AddAppColorProfile(L"devenv.exe", RGB(0, 255, 0), true);     // Green for Visual Studio, lock keys enabled
AddAppColorProfile(L"code.exe", RGB(0, 200, 255), true);     // Light blue for VS Code, lock keys enabled
AddAppColorProfile(L"notepad++.exe", RGB(255, 165, 0), true); // Orange for Notepad++, lock keys enabled
```

### Web Browsers and Media
Disable lock keys for clean browsing and media consumption:
```cpp
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);  // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"firefox.exe", RGB(255, 165, 0), false); // Orange for Firefox, lock keys disabled
AddAppColorProfile(L"vlc.exe", RGB(255, 140, 0), false);     // Dark orange for VLC, lock keys disabled
```

### Office Applications
Enable lock keys for productivity apps where case sensitivity matters:
```cpp
AddAppColorProfile(L"winword.exe", RGB(45, 125, 190), true);    // Word blue, lock keys enabled
AddAppColorProfile(L"excel.exe", RGB(21, 115, 71), true);       // Excel green, lock keys enabled
AddAppColorProfile(L"powerpnt.exe", RGB(183, 71, 42), true);    // PowerPoint red, lock keys enabled
```

## Color Configuration
The application uses a comprehensive color scheme with intelligent management:
- **Default Color**: User's base color for all regular keys and inactive lock keys (preserved across app switches)
- **Lock Key Colors**: Individual colors for NumLock, CapsLock, and ScrollLock when active (only when lock keys enabled)
- **App Colors**: Custom colors that activate when specific applications are running (don't overwrite user's default)
- **Lock Keys Control**: Per-app setting to enable/disable lock key highlighting
- **Priority System**: Most recently activated app takes precedence in multi-app scenarios
- **Future**: Individual key highlighting with `appHighlightColor` and `highlightKeys`

## Performance Considerations
- **Monitoring Interval**: 2-second intervals balance responsiveness with CPU usage
- **Efficient Detection**: Only visible applications are monitored (excludes background services)
- **Optimized Registry Access**: Registry operations are batched and only occur during startup/shutdown
- **Memory Usage**: Minimal memory footprint with efficient data structures
- **Thread Safety**: Lock-free operations where possible, minimal mutex contention
- **Smart State Tracking**: Dual state tracking minimizes unnecessary color updates

## Troubleshooting

### App Monitoring Issues
- **App not detected**: Ensure the application has visible, non-minimized windows
- **Wrong executable name**: Use Task Manager to verify the exact process name
- **Delayed switching**: Normal behavior due to 2-second monitoring interval
- **Colors not restoring**: Fixed in current version - colors now properly restore to user defaults
- **Priority conflicts**: Check debug output to understand most recently activated logic

### Debug Logging
Enable debug output to troubleshoot app switching behavior:
```cpp
// Look for these debug messages in your debug output window
"[DEBUG] App started: appname.exe - isAppRunning changed to TRUE"
"[DEBUG] Most recently activated profile updated to: appname.exe"
"[DEBUG] Profile appname.exe - isProfileCurrentlyInUse changed to TRUE (most recently activated)"
```

### Registry Issues
- **Profiles not saving**: Check Windows registry permissions for HKEY_CURRENT_USER
- **Profiles not loading**: Verify registry entries exist under SmartLogiLED\AppProfiles
- **Corrupted settings**: Delete the registry key to reset to defaults

### Performance Issues
- **High CPU usage**: Rare - check for conflicting software or restart application
- **Memory leaks**: Fixed with proper thread cleanup and mutex management
- **Slow response**: Check if antivirus is scanning the executable repeatedly

## Future Enhancements
- **GUI Profile Management**: Visual interface for adding/editing app profiles with priority control
- **Individual Key Highlighting**: Use `highlightKeys` and `appHighlightColor` for specific key customization
- **Export/Import Profiles**: Save/load profile configurations to/from files
- **Conditional Profiles**: Time-based or condition-based profile activation
- **Integration with Other RGB Devices**: Support for mice, headsets, and other peripherals
- **Manual Priority Override**: User control over which app takes precedence

## Thread Safety Details
The application uses several enhanced thread safety mechanisms:
- **Mutex Protection**: `appProfilesMutex` protects all profile operations
- **Copy Functions**: Thread-safe data access via `GetAppColorProfilesCopy()`
- **Atomic Operations**: Simple state checks use atomic reads where possible
- **Isolated Registry Access**: Configuration operations are separated from real-time monitoring
- **State Synchronization**: Profile state changes are properly synchronized across threads

---

**Note**: All app profiles are automatically saved to the Windows registry and will persist across application restarts. The monitoring system is designed to be efficient and responsive while maintaining system stability and performance. The most recently activated app priority system ensures intuitive behavior when working with multiple monitored applications.