# SmartLogiLED App Monitoring Usage Guide

## Overview
The SmartLogiLED application now includes advanced functionality to detect when specific applications are started by Windows and automatically change the keyboard lighting colors based on the running application. Additionally, each app profile can control whether the lock keys feature is enabled or disabled, and profiles are automatically persisted to the Windows registry.

## How It Works
- The app monitoring runs in a background thread that checks for running processes every 2 seconds
- When a monitored application starts, the keyboard lighting changes to the configured color for that app
- When a monitored application stops, the lighting intelligently hands off to other active monitored apps or reverts to the default color
- **Lock Keys Feature Control**: Each app profile can enable or disable the lock keys highlighting feature
- **Default Behavior**: When no app profile is active, lock keys feature is always enabled
- **Persistent Storage**: All app profiles are automatically saved to and loaded from the Windows registry

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

## Multiple Application Handoff

### Intelligent App Switching
When multiple monitored applications are running simultaneously:
- **Priority**: The first active application in the profiles list takes precedence
- **Handoff**: When the active app stops, lighting automatically switches to the next active monitored app
- **Fallback**: If no monitored apps remain active, returns to the user's default color

### Example Scenario
1. Start Chrome (cyan color, lock keys disabled) ? Keyboard becomes cyan, lock keys disabled
2. Start VS Code (green color, lock keys enabled) ? If VS Code is first in list, keyboard becomes green, lock keys enabled
3. Close VS Code ? Keyboard automatically returns to Chrome's cyan color and lock keys disabled
4. Close Chrome ? Keyboard returns to user's default color, lock keys enabled

## App Profile Structure

### Enhanced AppColorProfile
```cpp
struct AppColorProfile {
    std::wstring appName;                           // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255);          // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for future UI features
    bool isActive = false;                          // Whether this profile is currently active (runtime)
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

## Available Functions

### Core Functions
- `InitializeAppMonitoring()` - Start the monitoring thread
- `CleanupAppMonitoring()` - Stop the monitoring thread
- `IsAppRunning(const std::wstring& appName)` - Check if an app is currently running
- `IsLockKeysFeatureEnabled()` - Check if lock keys feature should be enabled based on current active profile

### Profile Management Functions
```cpp
// Add/Update profiles
void AddAppColorProfile(const std::wstring& appName, COLORREF color);
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);

// Remove profiles
void RemoveAppColorProfile(const std::wstring& appName);

// Get profile information
std::vector<AppColorProfile>& GetAppColorProfiles();           // Direct reference (not thread-safe)
std::vector<AppColorProfile> GetAppColorProfilesCopy();        // Thread-safe copy
size_t GetAppProfilesCount();                                  // Get number of profiles

// Manual updates
void CheckRunningAppsAndUpdateColors();                        // Immediately check and update colors
```

### Registry Persistence Functions
```cpp
// Configuration functions (in SmartLogiLED_Config.cpp)
void SaveAppProfilesToRegistry();     // Save all profiles to registry
void LoadAppProfilesFromRegistry();   // Load all profiles from registry

// Color settings
void SaveLockKeyColorsToRegistry();   // Save lock key colors
void LoadLockKeyColorsFromRegistry(); // Load lock key colors

// Start minimized setting
void SaveStartMinimizedSetting(bool minimized);
bool LoadStartMinimizedSetting();
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

### Thread Safety
- **Mutex Protection**: All app profile operations are thread-safe
- **Safe Access**: Use `GetAppColorProfilesCopy()` for thread-safe profile access
- **Concurrent Operations**: Registry operations are safely isolated from monitoring thread

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
- **Future**: Individual key highlighting with `appHighlightColor` and `highlightKeys`

## Customization Examples

### Adding Your Own Profiles
1. **Find the executable name**: Use Task Manager to find the exact process name (e.g., "myapp.exe")
2. **Choose colors**: Use RGB values (e.g., RGB(255, 0, 0) for red)
3. **Decide lock key behavior**: `true` for enabled, `false` for disabled
4. **Add the profile**:
   ```cpp
   AddAppColorProfile(L"myapp.exe", RGB(255, 0, 0), true);
   ```

### Color Palette Suggestions
```cpp
// Professional/Corporate
AddAppColorProfile(L"outlook.exe", RGB(0, 114, 198), true);     // Outlook blue
AddAppColorProfile(L"teams.exe", RGB(98, 100, 167), true);      // Teams purple

// Creative/Design
AddAppColorProfile(L"photoshop.exe", RGB(49, 168, 255), false); // Photoshop blue
AddAppColorProfile(L"illustrator.exe", RGB(255, 157, 0), false); // Illustrator orange

// Entertainment
AddAppColorProfile(L"spotify.exe", RGB(30, 215, 96), false);    // Spotify green
AddAppColorProfile(L"discord.exe", RGB(114, 137, 218), false);  // Discord blurple
```

## Performance Considerations
- **Monitoring Interval**: 2-second intervals balance responsiveness with CPU usage
- **Efficient Detection**: Only visible applications are monitored (excludes background services)
- **Optimized Registry Access**: Registry operations are batched and only occur during startup/shutdown
- **Memory Usage**: Minimal memory footprint with efficient data structures
- **Thread Safety**: Lock-free operations where possible, minimal mutex contention

## Troubleshooting

### App Monitoring Issues
- **App not detected**: Ensure the application has visible, non-minimized windows
- **Wrong executable name**: Use Task Manager to verify the exact process name
- **Delayed switching**: Normal behavior due to 2-second monitoring interval
- **Colors not restoring**: Fixed in current version - colors now properly restore to user defaults

### Registry Issues
- **Profiles not saving**: Check Windows registry permissions for HKEY_CURRENT_USER
- **Profiles not loading**: Verify registry entries exist under SmartLogiLED\AppProfiles
- **Corrupted settings**: Delete the registry key to reset to defaults

### Performance Issues
- **High CPU usage**: Rare - check for conflicting software or restart application
- **Memory leaks**: Fixed with proper thread cleanup and mutex management
- **Slow response**: Check if antivirus is scanning the executable repeatedly

## Future Enhancements
- **GUI Profile Management**: Visual interface for adding/editing app profiles
- **Individual Key Highlighting**: Use `highlightKeys` and `appHighlightColor` for specific key customization
- **Export/Import Profiles**: Save/load profile configurations to/from files
- **Conditional Profiles**: Time-based or condition-based profile activation
- **Integration with Other RGB Devices**: Support for mice, headsets, and other peripherals

## Thread Safety Details
The application uses several thread safety mechanisms:
- **Mutex Protection**: `appProfilesMutex` protects all profile operations
- **Copy Functions**: Thread-safe data access via `GetAppColorProfilesCopy()`
- **Atomic Operations**: Simple state checks use atomic reads where possible
- **Isolated Registry Access**: Configuration operations are separated from real-time monitoring

---

**Note**: All app profiles are automatically saved to the Windows registry and will persist across application restarts. The monitoring system is designed to be efficient and responsive while maintaining system stability and performance.