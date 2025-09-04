# SmartLogiLED App Monitoring Usage Guide

## Overview
The SmartLogiLED application includes advanced functionality to detect when specific applications are started by Windows and automatically change the keyboard lighting colors based on the running application. The system features intelligent priority handling with most recently activated app taking precedence, individual key highlighting capabilities, and each app profile can control whether the lock keys feature is enabled or disabled. All profiles are automatically persisted to the Windows registry with export/import functionality.

## How It Works
- The app monitoring runs in a background thread that checks for running processes every 2 seconds
- **Visible Window Detection**: Only applications with visible, non-minimized windows are monitored (excludes background services)
- When a monitored application starts, the keyboard lighting changes to the configured color for that app
- **Most Recently Activated Priority**: The most recently started monitored app takes control of lighting
- When a monitored application stops, the lighting intelligently hands off to the most recently activated profile that's still running
- **Lock Keys Feature Control**: Each app profile can enable or disable the lock keys highlighting feature
- **Individual Key Highlighting**: Each profile can specify custom keys to highlight with a different color
- **Default Behavior**: When no app profile is active, lock keys feature is always enabled
- **Persistent Storage**: All app profiles are automatically saved to and loaded from the Windows registry
- **Export/Import**: Profiles can be exported to INI files and imported from INI files

## Enhanced Profile State Management

### Dual State Tracking
Each app profile maintains two separate boolean states:
- **`isAppRunning`**: Whether the application process is currently running and has visible windows
- **`isProfileCurrInUse`**: Whether this profile is currently controlling the keyboard colors

### Most Recently Activated Logic
- **`lastActivatedProfile`**: Tracks which profile was most recently activated
- **Priority System**: When multiple apps are running, the most recently activated takes precedence
- **Fallback Behavior**: When the active app stops, control passes to the most recently activated profile that's still running
- **Automatic Clear**: When no monitored apps remain active, `lastActivatedProfile` is cleared

### Debug Logging
Comprehensive debug output is available via `OutputDebugStringW` for troubleshooting:
```cpp
// Example debug messages
"[DEBUG] App started: notepad.exe - isAppRunning changed to TRUE"
"[DEBUG] Most recently activated profile updated to: notepad.exe"
"[DEBUG] Profile chrome.exe - isProfileCurrInUse changed to FALSE (handoff to most recent)"
"[DEBUG] Profile notepad.exe - isProfileCurrInUse changed to TRUE (most recently activated)"
"[DEBUG] CheckRunningAppsAndUpdateColors() - Starting scan"
"[DEBUG] No more active profiles - restoring default colors"
```

## Individual Key Highlighting

### Enhanced Key Control
Each app profile can specify individual keys to highlight with a custom color:
- **Highlight Color**: `appHighlightColor` defines the color for highlighted keys
- **Highlight Keys**: `highlightKeys` vector contains specific keys to highlight
- **Lock Key Interaction**: Highlighted lock keys respect the profile's lock key settings
- **Priority**: Lock key colors (when enabled) take precedence over highlight colors for lock keys

### Key Highlighting Behavior
```cpp
// When lock keys are enabled and key is in highlight list:
if (lockKeyActive) {
    // Use lock key color (NumLock/CapsLock/ScrollLock colors)
} else {
    // Use highlight color from profile
}

// When lock keys are disabled:
// Always use highlight color regardless of lock key state
```

## Lock Key Behavior

### When Lock Keys Feature is Enabled (Default)
- **Lock key ON**: Uses the specific lock key color (e.g., green for NumLock)
- **Lock key OFF**: Uses the appropriate color (app color if profile active, otherwise default color)
- **Highlighted Lock Keys**: Lock key color takes precedence when lock is active

### When Lock Keys Feature is Disabled (Per App Profile)
- **All lock keys**: Always use the app's default color regardless of their state
- **No highlighting**: Lock key state changes don't affect keyboard lighting
- **Highlighted Lock Keys**: Use highlight color if key is in highlight list

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
6. **Close Chrome** ? Keyboard returns to user's default color, lock keys enabled, `lastActivatedProfile` cleared

### Debug Output Example
```
[DEBUG] App started: code.exe - isAppRunning changed to TRUE
[DEBUG] Most recently activated profile updated to: code.exe
[DEBUG] Profile chrome.exe - isProfileCurrInUse changed to FALSE (handoff to most recent)
[DEBUG] Profile code.exe - isProfileCurrInUse changed to TRUE (most recently activated)
```

## App Profile Structure

### Enhanced AppColorProfile
```cpp
struct AppColorProfile {
    std::wstring appName;                           // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255);          // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for individual keys
    bool isAppRunning = false;                      // Whether this app is currently running and visible
    bool isProfileCurrInUse = false;               // Whether this profile currently controls keyboard colors
    bool lockKeysEnabled = true;                    // Whether lock keys feature is enabled (default: true)
    std::vector<LogiLed::KeyName> highlightKeys;   // Specific keys to highlight with appHighlightColor
};
```

## Registry Persistence

### Automatic Storage
- **Location**: `HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles`
- **Per-App Subkeys**: Each application gets its own registry subkey
- **Automatic Save**: Profiles are saved when `SaveAppProfilesToRegistry()` is called
- **Automatic Load**: Profiles are restored when the application starts via `LoadAppProfilesFromRegistry()`
- **Individual Updates**: Registry values can be updated individually without full save

### Registry Structure
```
HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles\
??? notepad.exe\
?   ??? AppColor (DWORD): RGB color value
?   ??? AppHighlightColor (DWORD): Highlight color value  
?   ??? LockKeysEnabled (DWORD): 1 = enabled, 0 = disabled
?   ??? HighlightKeys (BINARY): Array of LogiLed::KeyName enum values
??? chrome.exe\
?   ??? AppColor (DWORD): RGB color value
?   ??? ...
```

## Export/Import Functionality

### INI File Export
- **Export All Profiles**: `ExportAllProfilesToIniFiles()` exports all profiles to individual INI files
- **Folder Selection**: User selects destination folder via folder browser dialog
- **File Naming**: Files are named `SmartLogiLED_<appname>.ini` (without .exe extension)
- **Character Sanitization**: Invalid filename characters are replaced with underscores

### INI File Format
```ini
[SmartLogiLED Profile]
AppName=notepad.exe
AppColor=FFFF00
AppHighlightColor=FFFFFF
LockKeysEnabled=1
HighlightKeys=F1,F2,CAPS_LOCK,NUM_LOCK

; SmartLogiLED Profile Export
; Generated automatically by SmartLogiLED
; 
; AppColor and AppHighlightColor are in hexadecimal RGB format (e.g., FF0000 = Red)
; LockKeysEnabled: 1 = enabled, 0 = disabled
; HighlightKeys: Comma-separated list of key names to highlight
```

### INI File Import
- **Import Single Profile**: `ImportProfileFromIniFile()` imports a single profile from INI file
- **File Dialog**: Standard Windows file open dialog with INI filter
- **Overwrite Protection**: Prompts user before overwriting existing profiles
- **Validation**: Validates INI format and required fields before import
- **UI Refresh**: Automatically refreshes combo boxes after successful import

## Core Functions

### App Monitoring Functions
```cpp
// Initialize and manage monitoring
void InitializeAppMonitoring();                    // Start the monitoring thread
void CleanupAppMonitoring();                      // Stop the monitoring thread

// Query functions
bool IsAppRunning(const std::wstring& appName);   // Check if an app is currently running with visible windows
bool IsLockKeysFeatureEnabled();                  // Check if lock keys should be enabled
std::wstring GetLastActivatedProfileName();        // Get the most recently activated profile name
std::vector<std::wstring> GetVisibleRunningProcesses(); // Get list of all visible processes

// Manual updates
void CheckRunningAppsAndUpdateColors();           // Immediately scan and update colors
```

### Profile Management Functions
```cpp
// Add/Update profiles
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);

// Profile access (thread-safe)
std::vector<AppColorProfile> GetAppColorProfilesCopy();  // Get thread-safe copy of all profiles
AppColorProfile* GetDisplayedProfile();                   // Get currently active/displayed profile
AppColorProfile* GetAppProfileByName(const std::wstring& appName); // Get specific profile

// Remove profiles
void RemoveAppColorProfile(const std::wstring& appName);

// Update functions
void UpdateAppProfileColor(const std::wstring& appName, COLORREF newAppColor);
void UpdateAppProfileHighlightColor(const std::wstring& appName, COLORREF newHighlightColor);
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);

// UI integration
void SetMainWindowHandle(HWND hWnd);               // Set handle for UI update messages
```

### Key Management Functions
```cpp
// Color application
void SetHighlightKeysColor();                     // Apply highlight color to keys from active profile

// Key conversion utilities
LogiLed::KeyName VirtualKeyToLogiLedKey(DWORD vkCode);        // Convert Windows VK to LogiLed key
std::wstring LogiLedKeyToDisplayName(LogiLed::KeyName key);   // Convert to user-friendly name
std::wstring LogiLedKeyToConfigName(LogiLed::KeyName key);    // Convert to INI config name
LogiLed::KeyName ConfigNameToLogiLedKey(const std::wstring& configName); // Convert from INI config name
std::wstring FormatHighlightKeysForDisplay(const std::vector<LogiLed::KeyName>& keys); // Format for UI display
```

### Registry Functions
```cpp
// Configuration persistence
void SaveAppProfilesToRegistry();                 // Save all profiles to registry
void LoadAppProfilesFromRegistry();               // Load all profiles from registry
void AddAppProfileToRegistry(const AppColorProfile& profile); // Add single profile
void RemoveAppProfileFromRegistry(const std::wstring& appName); // Remove single profile

// Individual updates (immediate registry write)
void UpdateAppProfileColorInRegistry(const std::wstring& appName, COLORREF newAppColor);
void UpdateAppProfileHighlightColorInRegistry(const std::wstring& appName, COLORREF newHighlightColor);
void UpdateAppProfileLockKeysEnabledInRegistry(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);

// Export/Import
void ExportAllProfilesToIniFiles();               // Export all profiles to INI files
void ImportProfileFromIniFile(HWND hWnd);         // Import profile from INI file
```

## Adding App Color Profiles

### Function Signature
```cpp
// Full profile with all features
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);
```

### Examples

#### Basic Profiles
```cpp
AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0), true);      // Yellow for Notepad, lock keys enabled
AddAppColorProfile(L"calculator.exe", RGB(0, 255, 0), true);     // Green for Calculator, lock keys enabled
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);     // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);          // Green for VS Code, lock keys enabled
```

#### Adding Individual Key Highlighting
```cpp
// After adding the basic profile, add highlight keys:
std::vector<LogiLed::KeyName> highlightKeys = {
    LogiLed::KeyName::F1, 
    LogiLed::KeyName::F2, 
    LogiLed::KeyName::CAPS_LOCK
};
UpdateAppProfileHighlightKeys(L"notepad.exe", highlightKeys);
UpdateAppProfileHighlightColor(L"notepad.exe", RGB(255, 0, 0)); // Red highlight color
```

### Current Predefined Profiles
Based on the current implementation, the application may include these predefined profiles:
```cpp
AddAppColorProfile(L"notepad.exe", RGB(255, 255, 0), true);      // Yellow for Notepad, lock keys enabled
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);     // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"firefox.exe", RGB(255, 165, 0), false);    // Orange for Firefox, lock keys disabled  
AddAppColorProfile(L"code.exe", RGB(0, 255, 0), true);          // Green for VS Code, lock keys enabled
AddAppColorProfile(L"devenv.exe", RGB(128, 0, 128), true);      // Purple for Visual Studio, lock keys enabled
```

## Architecture Improvements

### Separated Configuration Module
- **SmartLogiLED_Config.cpp**: All registry-related functions and export/import functionality
- **SmartLogiLED_Config.h**: Configuration function declarations
- **SmartLogiLED_Logic.cpp**: Core application logic, monitoring, and color management
- **SmartLogiLED_Logic.h**: Logic function declarations

### Color Management Enhancements
- **Preserved Default Color**: User's base default color is preserved when apps activate/deactivate
- **Smart Lock Key Colors**: Lock keys use appropriate base color (app color when active, default otherwise)
- **Highlight Key Priority**: Lock key colors take precedence over highlight colors when lock is active
- **Individual Key Control**: Each profile can specify custom highlight keys and colors

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
        std::vector<std::wstring> currentRunningApps = GetVisibleRunningProcesses();
        
        // Check for newly started apps (only visible windows)
        for (const auto& app : currentRunningApps) {
            if (std::find(lastRunningApps.begin(), lastRunningApps.end(), app) == lastRunningApps.end()) {
                // New app detected - update lastActivatedProfile and give it control
                // Apply colors and highlight keys
            }
        }
        
        // Check for stopped apps with intelligent handoff
        for (const auto& app : lastRunningApps) {
            if (std::find(currentRunningApps.begin(), currentRunningApps.end(), app) == currentRunningApps.end()) {
                // App stopped - find replacement based on most recently activated
                // Clear lastActivatedProfile if no apps remain
            }
        }
        
        lastRunningApps = currentRunningApps;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}
```

## Use Cases

### Gaming Applications
Disable lock keys for games to prevent accidental highlighting and distractions:
```cpp
AddAppColorProfile(L"game.exe", RGB(255, 0, 0), false);   // Red for games, no lock key distractions
AddAppColorProfile(L"steam.exe", RGB(100, 149, 237), false); // Steel blue for Steam, clean interface

// Add WASD highlighting for gaming
std::vector<LogiLed::KeyName> wasdKeys = {
    LogiLed::KeyName::W, LogiLed::KeyName::A, 
    LogiLed::KeyName::S, LogiLed::KeyName::D
};
UpdateAppProfileHighlightKeys(L"game.exe", wasdKeys);
UpdateAppProfileHighlightColor(L"game.exe", RGB(255, 255, 0)); // Yellow WASD keys
```

### Development Tools
Keep lock keys enabled for coding applications where Caps Lock/Num Lock status is important:
```cpp
AddAppColorProfile(L"devenv.exe", RGB(0, 255, 0), true);     // Green for Visual Studio, lock keys enabled
AddAppColorProfile(L"code.exe", RGB(0, 200, 255), true);     // Light blue for VS Code, lock keys enabled
AddAppColorProfile(L"notepad++.exe", RGB(255, 165, 0), true); // Orange for Notepad++, lock keys enabled

// Highlight function keys for debugging
std::vector<LogiLed::KeyName> debugKeys = {
    LogiLed::KeyName::F5, LogiLed::KeyName::F9, 
    LogiLed::KeyName::F10, LogiLed::KeyName::F11
};
UpdateAppProfileHighlightKeys(L"devenv.exe", debugKeys);
UpdateAppProfileHighlightColor(L"devenv.exe", RGB(255, 255, 0)); // Yellow debug keys
```

### Web Browsers and Media
Disable lock keys for clean browsing and media consumption:
```cpp
AddAppColorProfile(L"chrome.exe", RGB(0, 255, 255), false);  // Cyan for Chrome, lock keys disabled
AddAppColorProfile(L"firefox.exe", RGB(255, 165, 0), false); // Orange for Firefox, lock keys disabled
AddAppColorProfile(L"vlc.exe", RGB(255, 140, 0), false);     // Dark orange for VLC, lock keys disabled

// Highlight media control keys for VLC
std::vector<LogiLed::KeyName> mediaKeys = {
    LogiLed::KeyName::SPACE, LogiLed::KeyName::ARROW_LEFT, 
    LogiLed::KeyName::ARROW_RIGHT, LogiLed::KeyName::ARROW_UP, LogiLed::KeyName::ARROW_DOWN
};
UpdateAppProfileHighlightKeys(L"vlc.exe", mediaKeys);
UpdateAppProfileHighlightColor(L"vlc.exe", RGB(255, 255, 255)); // White media keys
```

### Office Applications
Enable lock keys for productivity apps where case sensitivity matters:
```cpp
AddAppColorProfile(L"winword.exe", RGB(45, 125, 190), true);    // Word blue, lock keys enabled
AddAppColorProfile(L"excel.exe", RGB(21, 115, 71), true);       // Excel green, lock keys enabled
AddAppColorProfile(L"powerpnt.exe", RGB(183, 71, 42), true);    // PowerPoint red, lock keys enabled

// Highlight common shortcuts for Word
std::vector<LogiLed::KeyName> wordKeys = {
    LogiLed::KeyName::LEFT_CONTROL, LogiLed::KeyName::C, 
    LogiLed::KeyName::V, LogiLed::KeyName::Z, LogiLed::KeyName::Y
};
UpdateAppProfileHighlightKeys(L"winword.exe", wordKeys);
UpdateAppProfileHighlightColor(L"winword.exe", RGB(255, 255, 0)); // Yellow shortcut keys
```

## Color Configuration
The application uses a comprehensive color scheme with intelligent management:
- **Default Color**: User's base color for all regular keys and inactive lock keys (preserved across app switches)
- **Lock Key Colors**: Individual colors for NumLock, CapsLock, and ScrollLock when active (only when lock keys enabled)
- **App Colors**: Custom colors that activate when specific applications are running (don't overwrite user's default)
- **Highlight Colors**: Individual key highlighting with `appHighlightColor` and `highlightKeys` per profile
- **Lock Keys Control**: Per-app setting to enable/disable lock key highlighting
- **Priority System**: Most recently activated app takes precedence in multi-app scenarios
- **Intelligent Handoff**: Proper color restoration when switching between apps or returning to defaults

## Performance Considerations
- **Monitoring Interval**: 2-second intervals balance responsiveness with CPU usage
- **Efficient Detection**: Only visible applications with non-minimized windows are monitored (excludes background services)
- **Optimized Registry Access**: Registry operations are batched where possible and individual updates are available
- **Memory Usage**: Minimal memory footprint with efficient data structures
- **Thread Safety**: Lock-free operations where possible, minimal mutex contention
- **Smart State Tracking**: Dual state tracking minimizes unnecessary color updates
- **Individual Updates**: Registry values can be updated individually without full profile save

## Troubleshooting

### App Monitoring Issues
- **App not detected**: Ensure the application has visible, non-minimized windows (background services are ignored)
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
"[DEBUG] Profile appname.exe - isProfileCurrInUse changed to TRUE (most recently activated)"
"[DEBUG] CheckRunningAppsAndUpdateColors() - Starting scan"
"[DEBUG] No more active profiles - restoring default colors"
```

### Registry Issues
- **Profiles not saving**: Check Windows registry permissions for HKEY_CURRENT_USER
- **Profiles not loading**: Verify registry entries exist under SmartLogiLED\AppProfiles
- **Corrupted settings**: Delete the registry key to reset to defaults
- **Binary data corruption**: HighlightKeys use BINARY data type for LogiLed::KeyName arrays

### Export/Import Issues
- **Export fails**: Check folder permissions and disk space in destination folder
- **Import validation**: Ensure INI file has proper format with [SmartLogiLED Profile] section
- **Character encoding**: Files are saved/loaded as UTF-8
- **Key name errors**: Invalid highlight key names are ignored during import

### Performance Issues
- **High CPU usage**: Rare - check for conflicting software or restart application
- **Memory leaks**: Fixed with proper thread cleanup and mutex management
- **Slow response**: Check if antivirus is scanning the executable repeatedly

## Future Enhancements
- **GUI Profile Management**: Visual interface for adding/editing highlight keys with visual keyboard
- **Advanced Key Highlighting**: Time-based or pattern-based key highlighting effects
- **Profile Templates**: Predefined profile templates for common application types
- **Conditional Profiles**: Time-based or condition-based profile activation
- **Integration with Other RGB Devices**: Support for mice, headsets, and other peripherals
- **Manual Priority Override**: User control over which app takes precedence
- **Profile Sharing**: Cloud-based profile sharing and synchronization

## Thread Safety Details
The application uses several enhanced thread safety mechanisms:
- **Mutex Protection**: `appProfilesMutex` protects all profile operations
- **Copy Functions**: Thread-safe data access via `GetAppColorProfilesCopy()`
- **Atomic Operations**: Simple state checks use atomic reads where possible
- **Isolated Registry Access**: Configuration operations are separated from real-time monitoring
- **State Synchronization**: Profile state changes are properly synchronized across threads
- **UI Updates**: Thread-safe UI notifications via `PostMessage` with `WM_UPDATE_PROFILE_COMBO`

---

**Note**: All app profiles are automatically saved to the Windows registry and will persist across application restarts. The monitoring system is designed to be efficient and responsive while maintaining system stability and performance. The most recently activated app priority system ensures intuitive behavior when working with multiple monitored applications. Individual key highlighting and export/import functionality provide extensive customization capabilities for power users.