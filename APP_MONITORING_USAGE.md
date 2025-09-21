# SmartLogiLED App Monitoring Usage Guide

## Overview
The SmartLogiLED application includes advanced functionality to detect when specific applications are started by Windows and automatically change the keyboard lighting colors based on the running application. The system features intelligent priority handling with most recently activated app taking precedence, individual key highlighting capabilities, action key assignments with mutual exclusivity, and each app profile can control whether the lock keys feature is enabled or disabled. All profiles are automatically persisted to the Windows registry with export/import functionality.

## How It Works
- The app monitoring runs in a background thread that checks for running processes every 2 seconds
- **Visible Window Detection**: Only applications with visible, non-minimized windows are monitored (excludes background services)
- When a monitored application starts, the keyboard lighting changes to the configured color for that app
- **Most Recently Activated Priority**: The most recently started monitored app takes control of lighting
- When a monitored application stops, the lighting intelligently hands off to the most recently activated profile that's still running
- **Lock Keys Feature Control**: Each app profile can enable or disable the lock keys highlighting feature
- **Individual Key Highlighting & Actions**: Each profile can specify custom keys to highlight and action keys with separate colors
- **Mutual Key Exclusivity**: Keys automatically removed from one list when added to another, preventing conflicts
- **Default Behavior**: When no app profile is active, lock keys feature is always enabled
- **Persistent Storage**: All app profiles are automatically saved to and loaded from the Windows registry
- **Export/Import**: Profiles can be exported to INI files and imported from INI files

## Enhanced Profile State Management

### Dual State Tracking
Each app profile maintains two separate boolean states:
- **`isAppRunning`**: Whether the application process is currently running and has visible windows
- **`isProfileCurrInUse`**: Whether this profile is currently controlling the keyboard colors

### Most Recently Activated Logic
- **Activation History**: Tracks up to 10 most recently activated profiles with comprehensive history management
- **Priority System**: When multiple apps are running, the most recently activated takes precedence
- **Fallback Behavior**: When the active app stops, control passes to the most recently activated profile that's still running
- **Automatic Cleanup**: Activation history is automatically cleaned when profiles are removed

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
"[DEBUG] Activation history updated. Current order: notepad.exe -> chrome.exe -> END"
```

## Individual Key Highlighting & Action Keys

### Enhanced Key Control with Mutual Exclusivity
Each app profile can specify individual keys for two distinct purposes:
- **Highlight Color**: `appHighlightColor` defines the color for highlighted keys (typical use: important keys, navigation)
- **Highlight Keys**: `highlightKeys` vector contains specific keys to highlight
- **Action Color**: `appActionColor` defines the color for action keys (typical use: shortcuts, special functions)
- **Action Keys**: `actionKeys` vector contains specific keys for actions
- **Mutual Exclusivity**: Keys cannot exist in both highlight and action lists simultaneously
- **Automatic Conflict Resolution**: Adding a key to one list automatically removes it from the other
- **Lock Key Interaction**: Highlighted/action lock keys respect the profile's lock key settings

### Key Management Behavior
```cpp
// Priority system for key colors:
if (lockKeyActive && lockKeysEnabled) {
    // Use lock key color (NumLock/CapsLock/ScrollLock colors) - highest priority
} else if (keyInActionList) {
    // Use action color from profile
} else if (keyInHighlightList) {
    // Use highlight color from profile  
} else {
    // Use base app color or default color
}

// When lock keys are disabled for profile:
// Lock keys use action/highlight colors if assigned, otherwise app color
```

### Interactive Key Configuration
- **Live Key Capture**: Press keys in configuration dialogs to add/remove them from lists
- **Visual Feedback**: Real-time preview of key assignments with immediate text updates
- **Reset Functionality**: Clear all keys from current list with Reset button
- **Conflict Prevention**: Automatic removal from other lists when keys are assigned

## Lock Key Behavior

### When Lock Keys Feature is Enabled (Default)
- **Lock key ON**: Uses the specific lock key color (e.g., green for NumLock)
- **Lock key OFF**: Uses the appropriate color (app color if profile active, otherwise default color)
- **Highlighted/Action Lock Keys**: Lock key color takes precedence when lock is active
- **Priority**: Lock key state colors override highlight/action colors for better visibility

### When Lock Keys Feature is Disabled (Per App Profile)
- **All lock keys**: Always use the app's default color or highlight/action colors if assigned
- **No state highlighting**: Lock key state changes don't affect keyboard lighting
- **Individual Assignment**: Lock keys can still be assigned to highlight or action lists
- **Clean Interface**: Useful for gaming or applications where lock key distractions are unwanted

### Feature Control Logic
- **No active profile**: Lock keys feature is enabled (default behavior)
- **Active profile exists**: Uses the `lockKeysEnabled` flag from that profile
- **Multiple profiles**: The currently displayed profile (most recently activated) determines the lock keys behavior

## Multiple Application Handoff

### Intelligent App Switching with Enhanced Priority
When multiple monitored applications are running simultaneously:
- **Activation History Tracking**: Maintains ordered history of up to 10 most recently activated profiles
- **Smart Fallback Chain**: When active app stops, falls back through activation history to find next running app
- **Automatic History Cleanup**: Removes deleted profiles from activation history automatically
- **Default Restoration**: If no monitored apps remain active, returns to the user's default color

### Example Scenario
1. **Start Chrome** (cyan color, lock keys disabled) → Keyboard becomes cyan, lock keys disabled, added to activation history
2. **Start VS Code** (green color, lock keys enabled) → Keyboard becomes green, lock keys enabled, becomes most recent
3. **Start Notepad** (yellow color, lock keys enabled, F1/F2 highlighted red, Ctrl+S action yellow) → Keyboard becomes yellow, special keys colored
4. **Close Notepad** → Keyboard returns to VS Code colors (green, lock keys enabled) since it's the most recently activated of remaining apps
5. **Close VS Code** → Keyboard returns to Chrome colors (cyan, lock keys disabled)
6. **Close Chrome** → Keyboard returns to user's default color, lock keys enabled, activation history cleared

### Debug Output Example
```
[DEBUG] App started: code.exe - isAppRunning changed to TRUE
[DEBUG] Activation history updated. Current order: code.exe -> chrome.exe -> END
[DEBUG] Profile chrome.exe - isProfileCurrInUse changed to FALSE (handoff to most recent)
[DEBUG] Profile code.exe - isProfileCurrInUse changed to TRUE (most recently activated)
[DEBUG] Applying profile: code.exe
```

## App Profile Structure

### Enhanced AppColorProfile
```cpp
struct AppColorProfile {
    std::wstring appName;                           // Application executable name (e.g., L"notepad.exe")
    COLORREF appColor = RGB(0, 255, 255);          // Color to set when app starts
    COLORREF appHighlightColor = RGB(255, 255, 255); // Highlight color for individual keys
    COLORREF appActionColor = RGB(255, 255, 0);    // Action color for action keys
    bool isAppRunning = false;                      // Whether this app is currently running and visible
    bool isProfileCurrInUse = false;               // Whether this profile currently controls keyboard colors
    bool lockKeysEnabled = true;                    // Whether lock keys feature is enabled (default: true)
    std::vector<LogiLed::KeyName> highlightKeys;   // Specific keys to highlight with appHighlightColor
    std::vector<LogiLed::KeyName> actionKeys;      // Specific keys for actions with appActionColor
};
```

## Registry Persistence

### Automatic Storage
- **Location**: `HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles`
- **Per-App Subkeys**: Each application gets its own registry subkey
- **Automatic Save**: Profiles are saved when profile management functions are called
- **Automatic Load**: Profiles are restored when the application starts via `LoadAppProfilesFromRegistry()`
- **Individual Updates**: Registry values can be updated individually without full save
- **Action Key Support**: Action keys and colors are fully persisted in registry

### Registry Structure
```
HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles\
├── notepad.exe\
│   ├── AppColor (DWORD): RGB color value
│   ├── AppHighlightColor (DWORD): Highlight color value
│   ├── AppActionColor (DWORD): Action color value
│   ├── LockKeysEnabled (DWORD): 1 = enabled, 0 = disabled
│   ├── HighlightKeys (BINARY): Array of LogiLed::KeyName enum values
│   └── ActionKeys (BINARY): Array of LogiLed::KeyName enum values
├── chrome.exe\
│   ├── AppColor (DWORD): RGB color value
│   └── ...
```

## Export/Import Functionality

### INI File Export
- **Export All Profiles**: `ExportAllProfilesToIniFiles()` exports all profiles to individual INI files
- **Export Single Profile**: `ExportSelectedProfileToIniFile()` exports currently selected profile
- **Folder Selection**: User selects destination folder via folder browser dialog
- **File Naming**: Files are named `SmartLogiLED_<appname>.ini` (without .exe extension)
- **Character Sanitization**: Invalid filename characters are replaced with underscores
- **Complete Data**: All profile data including action keys and colors are exported

### Enhanced INI File Format
```ini
[SmartLogiLED Profile]
AppName=notepad.exe
AppColor=FFFF00
AppHighlightColor=FF0000
AppActionColor=00FF00
LockKeysEnabled=1
HighlightKeys=F1,F2,CAPS_LOCK,NUM_LOCK
ActionKeys=F3,F4,CTRL_LEFT,S

; SmartLogiLED Profile Export
; Generated automatically by SmartLogiLED v3.1.0
; Compatible with SmartLogiLED v3.0.0 and later
; 
; AppColor, AppHighlightColor, and AppActionColor are in hexadecimal RGB format (e.g., FF0000 = Red)
; LockKeysEnabled: 1 = enabled, 0 = disabled
; HighlightKeys: Comma-separated list of key names to highlight
; ActionKeys: Comma-separated list of key names for actions (mutually exclusive with HighlightKeys)
```

### INI File Import
- **Import Single Profile**: `ImportProfileFromIniFile()` imports a single profile from INI file
- **File Dialog**: Standard Windows file open dialog with INI filter
- **Overwrite Protection**: Prompts user before overwriting existing profiles
- **Validation**: Validates INI format and required fields before import
- **Backward Compatibility**: Handles profiles from v3.0.0+ with automatic defaults for missing fields
- **UI Refresh**: Automatically refreshes UI after successful import

## Core Functions

### App Monitoring Functions
```cpp
// Initialize and manage monitoring
void InitializeAppMonitoring(HWND hMainWindow);    // Start the monitoring thread
void CleanupAppMonitoring();                       // Stop the monitoring thread

// Query functions
bool IsAppRunning(const std::wstring& appName);    // Check if an app is currently running with visible windows
std::vector<std::wstring> GetVisibleRunningProcesses(); // Get list of all visible processes

// Manual updates
void CheckRunningAppsAndUpdateColors();            // Immediately scan and update colors
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
void UpdateAppProfileActionColor(const std::wstring& appName, COLORREF newActionColor);
void UpdateAppProfileLockKeysEnabled(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);
void UpdateAppProfileActionKeys(const std::wstring& appName, const std::vector<LogiLed::KeyName>& actionKeys);

// Message handlers for app monitoring
void HandleAppStarted(const std::wstring& appName);
void HandleAppStopped(const std::wstring& appName);

// Activation history management
std::vector<std::wstring> GetActivationHistory();
void UpdateActivationHistory(const std::wstring& profileName);
AppColorProfile* FindBestFallbackProfile(const std::wstring& excludeProfile = L"");
void CleanupActivationHistory();
```

### Key Management Functions
```cpp
// Color application functions
void SetHighlightKeysColor();                      // Apply highlight color to keys from active profile
void SetHighlightKeysColorWithProfile(AppColorProfile* profile); // Profile-specific version
void SetActionKeysColor();                         // Apply action color to keys from active profile  
void SetActionKeysColorWithProfile(AppColorProfile* profile);    // Profile-specific version

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
size_t GetAppProfilesCount();                     // Get total number of profiles

// Individual updates (immediate registry write)
void UpdateAppProfileColorInRegistry(const std::wstring& appName, COLORREF newAppColor);
void UpdateAppProfileHighlightColorInRegistry(const std::wstring& appName, COLORREF newHighlightColor);
void UpdateAppProfileActionColorInRegistry(const std::wstring& appName, COLORREF newActionColor);
void UpdateAppProfileLockKeysEnabledInRegistry(const std::wstring& appName, bool lockKeysEnabled);
void UpdateAppProfileHighlightKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& highlightKeys);
void UpdateAppProfileActionKeysInRegistry(const std::wstring& appName, const std::vector<LogiLed::KeyName>& actionKeys);

// Export/Import
void ExportAllProfilesToIniFiles();               // Export all profiles to INI files
void ExportSelectedProfileToIniFile(HWND hWnd);   // Export currently selected profile
void ImportProfileFromIniFile(HWND hWnd);         // Import profile from INI file
```

### Dialog Functions
```cpp
// Profile management dialogs
void ShowKeysDialog(HWND hWnd);                    // Show highlight keys configuration dialog
void ShowActionKeysDialog(HWND hWnd);              // Show action keys configuration dialog
void ShowAddProfileDialog(HWND hWnd);              // Show add new profile dialog
void ShowAppColorPicker(HWND hWnd, int colorType); // Show color picker (0=app, 1=highlight, 2=action)
void RefreshAppProfileCombo(HWND hWnd);            // Refresh UI after profile changes
```

## GUI Profile Management

### Main Window Controls
- **App Profile Dropdown**: Select from configured profiles or "NONE"
- **Current Profile Label**: Shows active profile name or "No active profile"
- **Add Profile Button (+)**: Create new profiles from running applications
- **Remove Profile Button (-)**: Delete selected profile with confirmation
- **Color Boxes**: Click to change app, highlight, and action colors
- **Lock Keys Checkbox**: Toggle lock keys feature for selected profile
- **Highlight Keys Button**: Configure keys to highlight with custom color
- **Action Keys Button**: Configure action keys with separate custom color

### Add Profile Dialog
- **Application Selector**: Combo box populated with visible running processes
- **Smart Filtering**: Excludes applications that already have profiles
- **Manual Entry**: Type application name manually if not in list
- **Conflict Detection**: Prevents duplicate profiles with case-insensitive checking
- **Default Settings**: New profiles created with sensible defaults (cyan app color, red highlights, yellow actions, lock keys disabled)

### Key Configuration Dialogs
- **Interactive Key Capture**: Global keyboard hook captures key presses during configuration
- **Add/Remove Toggle**: Press key to add if not in list, remove if already present
- **Live Preview**: Text field updates in real-time showing current key assignments
- **Reset Functionality**: Clear all keys from current list
- **Mutual Exclusivity**: Keys automatically removed from other lists when added
- **User-Friendly Display**: Keys shown with readable names (e.g., "Ctrl Left", "F1", "Caps Lock")

## Adding App Color Profiles

### GUI-Based Profile Creation
1. **Click "+" Button**: Opens Add Profile dialog
2. **Select Application**: Choose from running processes or type manually
3. **Automatic Creation**: Profile created with default settings
4. **Customize Colors**: Click color boxes to change app, highlight, and action colors
5. **Configure Keys**: Use "Highlight Keys" and "Action Keys" buttons
6. **Lock Keys Setting**: Toggle checkbox for lock keys behavior

### Function-Based Profile Creation
```cpp
// Full profile with all features
void AddAppColorProfile(const std::wstring& appName, COLORREF color, bool lockKeysEnabled);
```

### Examples

#### Basic Profiles via GUI
```
After using Add Profile dialog:
- notepad.exe: Cyan app color, red highlights, yellow actions, lock keys off
- calculator.exe: Cyan app color, red highlights, yellow actions, lock keys off  
- chrome.exe: Cyan app color, red highlights, yellow actions, lock keys off
```

#### Customizing After Creation
```cpp
// After GUI creation, programmatically customize:
UpdateAppProfileHighlightColor(L"notepad.exe", RGB(255, 0, 0));     // Red highlights
UpdateAppProfileActionColor(L"notepad.exe", RGB(0, 255, 0));        // Green actions

// Add specific keys via UpdateAppProfileHighlightKeys/UpdateAppProfileActionKeys
std::vector<LogiLed::KeyName> highlightKeys = {
    LogiLed::KeyName::F1, 
    LogiLed::KeyName::F2, 
    LogiLed::KeyName::CAPS_LOCK
};
UpdateAppProfileHighlightKeys(L"notepad.exe", highlightKeys);

std::vector<LogiLed::KeyName> actionKeys = {
    LogiLed::KeyName::LEFT_CONTROL,
    LogiLed::KeyName::S,  // Ctrl+S shortcut
    LogiLed::KeyName::F5
};
UpdateAppProfileActionKeys(L"notepad.exe", actionKeys);
```

### Current Default Profiles
The application starts with no predefined profiles. Users create profiles as needed through the GUI or by importing INI files.

## Architecture Improvements

### Separated Module Structure
- **SmartLogiLED.cpp**: Main UI, window management, and user interaction
- **SmartLogiLED_AppProfiles.cpp**: Core profile management, monitoring, and color application logic
- **SmartLogiLED_LockKeys.cpp**: Lock key control, keyboard hook, and color management
- **SmartLogiLED_Config.cpp**: All registry-related functions and persistence
- **SmartLogiLED_ProcessMonitor.cpp**: Application detection and process monitoring
- **SmartLogiLED_Dialogs.cpp**: All dialog functionality and UI interactions
- **SmartLogiLED_KeyMapping.cpp**: Key mapping and conversion utilities
- **SmartLogiLED_IniFiles.cpp**: Export/import functionality and file handling
- **SmartLogiLED_Types.h**: Common data structures and type definitions

### Color Management Enhancements
- **Preserved Default Color**: User's base color is preserved when apps activate/deactivate
- **Smart Lock Key Colors**: Lock keys use appropriate base color (app color when active, default otherwise)
- **Three-Tier Key Priority**: Lock key colors > action key colors > highlight key colors > base colors
- **Mutual Key Exclusivity**: Automatic conflict resolution prevents keys from having multiple special assignments
- **Individual Key Control**: Each profile can specify custom highlight and action keys with separate colors

### Thread Safety Enhancements
- **Comprehensive Mutex Protection**: All app profile operations are thread-safe with `appProfilesMutex`
- **Safe Access Functions**: Use `GetAppColorProfilesCopy()` for thread-safe profile access
- **Optimized Internal Functions**: Internal helper functions with `Internal` suffix assume mutex already held
- **Phase-Based Operations**: Critical operations split into mutex-protected and mutex-free phases
- **Atomic Operations**: Simple state checks use atomic reads where possible
- **Isolated Registry Access**: Configuration operations are separated from real-time monitoring
- **State Synchronization**: Profile state changes are properly synchronized across threads
- **UI Updates**: Thread-safe UI notifications via `PostMessage` with custom window messages
- **Deadlock Prevention**: Careful ordering of mutex acquisition and color application

### Enhanced Monitoring Logic
```cpp
// Improved app monitoring with activation history and mutual exclusivity
void UpdateAndApplyActiveProfile() {
    // Phase 1: Determine correct active profile under mutex lock
    // Phase 2: Apply colors and notify UI without holding mutex
    // Ensures thread safety while preventing deadlocks
}

// Activation history management with automatic cleanup
void UpdateActivationHistoryInternal(const std::wstring& profileName) {
    // Maintains ordered history of up to MAX_ACTIVATION_HISTORY profiles
    // Automatic deduplication and size management
    // Comprehensive debug logging for troubleshooting
}
```

## Use Cases

### Gaming Applications
Disable lock keys for games to prevent accidental highlighting and distractions, use action keys for important shortcuts:
```
Via GUI:
1. Add profile for "game.exe"
2. Uncheck "Lock Keys" checkbox
3. Set app color to red
4. Use "Action Keys" button to add WASD movement keys
5. Set action color to yellow for visibility
```

### Development Tools
Keep lock keys enabled for coding applications where Caps Lock/Num Lock status is important, highlight frequently used keys:
```
Via GUI:
1. Add profile for "devenv.exe" (Visual Studio)
2. Keep "Lock Keys" checked
3. Set app color to green
4. Use "Highlight Keys" button to add F5, F9, F10, F11 (debug keys)
5. Use "Action Keys" button to add Ctrl+S, Ctrl+Z, Ctrl+Y (common shortcuts)
```

### Web Browsers and Media
Disable lock keys for clean browsing, highlight navigation keys:
```
Via GUI:
1. Add profile for "chrome.exe"
2. Uncheck "Lock Keys" checkbox  
3. Set app color to cyan
4. Use "Action Keys" button to add Ctrl+T, Ctrl+W, Ctrl+Shift+T (tab management)
5. Use "Highlight Keys" button to add arrow keys for navigation
```

### Office Applications
Enable lock keys for productivity apps where case sensitivity matters, highlight important shortcuts:
```
Via GUI:
1. Add profile for "winword.exe" (Microsoft Word)
2. Keep "Lock Keys" checked
3. Set app color to blue
4. Use "Action Keys" button to add Ctrl+S, Ctrl+C, Ctrl+V, Ctrl+Z
5. Use "Highlight Keys" button to add function keys for Word-specific features
```

## Color Configuration
The application uses a comprehensive color scheme with intelligent management:
- **Default Color**: User's base color for all regular keys and inactive lock keys (preserved across app switches)
- **Lock Key Colors**: Individual colors for NumLock, CapsLock, and ScrollLock when active (only when lock keys enabled)
- **App Colors**: Custom colors that activate when specific applications are running (don't overwrite user's default)
- **Highlight Colors**: Individual key highlighting with `appHighlightColor` and `highlightKeys` per profile
- **Action Colors**: Individual key highlighting with `appActionColor` and `actionKeys` per profile (mutually exclusive with highlight)
- **Lock Keys Control**: Per-app setting to enable/disable lock key highlighting
- **Priority System**: Most recently activated app takes precedence in multi-app scenarios
- **Intelligent Handoff**: Proper color restoration when switching between apps or returning to defaults

## Performance Considerations
- **Monitoring Interval**: 2-second intervals balance responsiveness with CPU usage
- **Efficient Detection**: Only visible applications with non-minimized windows are monitored (excludes background services)
- **Optimized Registry Access**: Registry operations are batched where possible and individual updates are available
- **Memory Usage**: Minimal memory footprint with efficient data structures and activation history limits
- **Thread Safety**: Optimized mutex usage with phase-based operations to minimize contention
- **Smart State Tracking**: Dual state tracking and activation history minimizes unnecessary color updates
- **Individual Updates**: Registry values can be updated individually without full profile save
- **Conflict Resolution**: Efficient mutual exclusivity with optimized key list operations

## Troubleshooting

### App Monitoring Issues
- **App not detected**: Ensure the application has visible, non-minimized windows (background services are ignored)
- **Wrong executable name**: Use Task Manager to verify the exact process name
- **Delayed switching**: Normal behavior due to 2-second monitoring interval
- **Colors not restoring**: Fixed in current version - colors now properly restore to user defaults
- **Priority conflicts**: Check debug output to understand activation history and fallback logic

### Key Assignment Issues
- **Keys not highlighting**: Ensure highlight/action colors differ from app color for visibility
- **Conflicting assignments**: Keys are automatically moved between highlight/action lists (mutually exclusive)
- **Lock keys not working**: Check if profile has lock keys enabled, lock key colors take priority when active
- **Keys not saving**: Verify dialog was closed with "Done" button, not Cancel or X

### Profile Management Issues
- **Can't add profile**: Check if application name already exists (case-insensitive)
- **Profile not found**: Ensure exact process name matches, use Add Profile dialog for accuracy
- **Colors not applying**: Verify profile is currently active (shown in dropdown)
- **Settings not persisting**: Check Windows registry permissions for HKEY_CURRENT_USER

### Debug Logging
Enable debug output to troubleshoot behavior (Debug builds only):
```cpp
// Look for these debug messages in your debug output window
"[DEBUG] App started: appname.exe - isAppRunning changed to TRUE"
"[DEBUG] Activation history updated. Current order: appname.exe -> otherapp.exe -> END"
"[DEBUG] Profile appname.exe - isProfileCurrInUse changed to TRUE (most recently activated)"
"[DEBUG] Applying profile: appname.exe"
"[DEBUG] No more active profiles - restoring default colors"
"[DEBUG] Mutual exclusivity: Removing key from highlightKeys when adding to actionKeys"
```

### Registry Issues
- **Profiles not saving**: Check Windows registry permissions for HKEY_CURRENT_USER
- **Profiles not loading**: Verify registry entries exist under SmartLogiLED\AppProfiles
- **Corrupted settings**: Delete the registry key to reset to defaults
- **Binary data corruption**: HighlightKeys and ActionKeys use BINARY data type for LogiLed::KeyName arrays

### Export/Import Issues
- **Export fails**: Check folder permissions and disk space in destination folder
- **Import validation**: Ensure INI file has proper format with [SmartLogiLED Profile] section
- **Version compatibility**: v3.1.0 INI files are backward compatible to v3.0.0
- **Key name errors**: Invalid highlight/action key names are ignored during import
- **Character encoding**: Files are saved/loaded with proper Unicode handling

### Performance Issues
- **High CPU usage**: Rare - check for conflicting software or restart application
- **Memory leaks**: Fixed with proper thread cleanup and mutex management
- **Slow response**: Check if antivirus is scanning the executable repeatedly
- **Hook conflicts**: Other applications with keyboard hooks may interfere

## Future Enhancements
- **Visual Key Configuration**: Graphical keyboard layout for easier key selection
- **Advanced Key Effects**: Time-based or pattern-based key highlighting effects
- **Profile Templates**: Predefined profile templates for common application types
- **Conditional Profiles**: Time-based or condition-based profile activation
- **Integration with Other RGB Devices**: Support for mice, headsets, and other peripherals
- **Manual Priority Override**: User control over which app takes precedence beyond activation history
- **Profile Sharing**: Cloud-based profile sharing and synchronization
- **Macro Integration**: Bind specific actions to highlighted/action keys
- **Game Integration**: Specific game detection and genre-based templates

## Thread Safety Details
The application uses several enhanced thread safety mechanisms:
- **Comprehensive Mutex Protection**: `appProfilesMutex` protects all profile operations
- **Phase-Based Operations**: Critical sections split to minimize mutex hold time
- **Copy Functions**: Thread-safe data access via `GetAppColorProfilesCopy()`
- **Internal Helper Functions**: Optimized functions with `Internal` suffix assume mutex already held
- **Atomic Operations**: Simple state checks use atomic reads where possible
- **Isolated Registry Access**: Configuration operations are separated from real-time monitoring
- **State Synchronization**: Profile state changes are properly synchronized across threads
- **UI Updates**: Thread-safe UI notifications via `PostMessage` with custom window messages
- **Deadlock Prevention**: Careful ordering of mutex acquisition and color application

---

**Note**: All app profiles are automatically saved to the Windows registry and will persist across application restarts. The monitoring system is designed to be efficient and responsive while maintaining system stability and performance. The activation history system ensures intuitive behavior when working with multiple monitored applications. Individual key highlighting with mutual exclusivity and export/import functionality provide extensive customization capabilities for power users. The GUI-based profile management makes the application accessible to users of all technical levels.