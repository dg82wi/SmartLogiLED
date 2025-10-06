# SmartLogiLED

A sophisticated Windows application that provides intelligent RGB keyboard lighting control for Logitech keyboards, featuring application-specific color profiles, individual key highlighting, action key assignments, lock key visualization, and comprehensive profile management with export/import capabilities.

## Features Overview

### 🔒 Lock Key Visual Control
- **Dynamic Lock Key Colors**: Customize RGB colors for NumLock, CapsLock, and ScrollLock keys
- **Real-time State Updates**: Lock key colors change instantly when toggled via global keyboard hook
- **Per-Application Control**: Enable or disable lock key visualization for specific applications
- **Intelligent State Management**: Lock keys respect application profile settings and highlight/action key configurations

### 🎯 Application-Specific Profiles
- **Smart App Detection**: Automatically detects visible applications with main windows (excludes background processes)
- **Seamless Color Switching**: Instantly switch keyboard colors when monitored applications start or become active
- **Priority-Based Handling**: Most recently activated monitored app takes precedence with intelligent fallback
- **Persistent Settings**: All profiles automatically saved to Windows registry with real-time updates

### 🎨 Individual Key Highlighting & Actions
- **Interactive Key Configuration**: Press keys to add/remove them from highlight or action lists via intuitive dialogs
- **Custom Colors**: Each profile supports separate colors for highlight keys and action keys
- **Mutual Exclusivity**: Keys automatically removed from one list when added to another, preventing conflicts
- **Lock Key Integration**: Highlighted/action lock keys intelligently blend with lock state visualization
- **Visual Feedback**: Real-time preview of key configurations with immediate application

### ⚙️ Advanced Profile Management
- **GUI-Based Creation**: Add profiles through intuitive dialogs with running app detection
- **Visual Color Controls**: Owner-drawn color boxes with integrated color picker dialogs
- **Profile Export/Import**: Share profiles via INI files with comprehensive metadata preservation
- **Bulk Operations**: Export all profiles at once or import individual profile configurations
- **Profile Status Tracking**: Real-time indicators showing active profiles and their states

### 🖥️ User Interface & Integration
- **System Tray Integration**: Full minimize-to-tray functionality with context menu support
- **Persistent Configuration**: Settings and profiles stored in Windows registry with automatic restoration
- **Start Minimized Option**: Configurable startup behavior with registry persistence
- **Thread-Safe Operations**: Mutex-protected data structures ensuring stable multi-threaded operation
- **Debug Logging**: Comprehensive debug output for troubleshooting (Debug builds)

## System Requirements

- **Operating System**: Windows 7 or later
- **Hardware**: Logitech RGB keyboard with per-key lighting support
- **Software**: Logitech Gaming Software or G HUB installed and running
- **Development**: Visual Studio 2019 or later, C++14 compatible compiler (for building from source)

## Quick Start

### Installation
1. Download the latest release from [GitHub Releases](https://github.com/dg82wi/SmartLogiLED/releases)
2. Extract files to your desired location
3. Run `SmartLogiLED.exe`
4. The application will initialize and begin monitoring for applications

### Basic Configuration
1. **System Tray Access**: Double-click the system tray icon to open the main window
2. **Lock Key Colors**: Click any color box in the "Lock Keys Color" section to customize colors
3. **Default Color**: Set the base color for all keyboard keys when no profiles are active
4. **Application Profiles**: Use the "+" button to create profiles for specific applications

## Detailed Usage Guide

### Lock Key Configuration
The lock key system provides visual feedback for NumLock, CapsLock, and ScrollLock states:

```
• NUM LOCK Color: Active when NumLock is enabled
• CAPS LOCK Color: Active when CapsLock is enabled  
• SCROLL LOCK Color: Active when ScrollLock is enabled
• Default Color: Used for all other keys and inactive lock keys
```

### Application Profile Workflow

#### Creating Profiles
1. Click the **"+"** button in the App Profile section
2. Select from detected running applications or manually enter executable name
3. Configure profile settings:
   - **App Color**: Primary keyboard color when application is active
   - **Highlight Color**: Color for individually highlighted keys
   - **Action Color**: Color for action keys (separate from highlight keys)
   - **Lock Keys**: Toggle lock key visualization for this application

#### Managing Highlight and Action Keys
1. Select a profile from the dropdown (must not be "NONE")
2. Click the **"H-Keys"** button to configure keys that should be highlighted
3. Click the **"A-Keys"** button to configure keys for actions/shortcuts
4. **Mutual Exclusivity**: Adding a key to one list automatically removes it from the other
5. Press keys in the configuration dialog to add/remove them from the respective lists
6. Use **"Reset"** to clear all keys in the current list
7. Click **"Done"** to save configuration

The key management system intelligently handles conflicts:
- **No Overlap**: Keys cannot exist in both highlight and action lists simultaneously
- **Priority**: When a key is added to one list, it's automatically removed from the other
- **Lock Key Handling**: If a lock key is highlighted/actionized and lock keys are enabled for the profile, the lock state color takes precedence when active

#### Profile Priority System
When multiple monitored applications are running:
- **Most Recently Activated**: The most recently started/focused app controls keyboard colors
- **Intelligent Fallback**: When active app closes, control passes to next most recent active profile
- **Default Restoration**: Returns to default colors when all monitored apps are closed

### Export/Import Functionality

#### Exporting Profiles
1. Navigate to **Menu → Export Profiles**
2. Select destination folder
3. All profiles exported as individual INI files: `SmartLogiLED_<appname>.ini`

#### Importing Profiles
1. Navigate to **Menu → Import Profile**
2. Select an INI file to import
3. Existing profiles with same name will prompt for overwrite confirmation

#### INI File Format
```ini
[SmartLogiLED Profile]
AppName=notepad.exe
AppColor=FFFF00
AppHighlightColor=FF0000
AppActionColor=00FF00
LockKeysEnabled=1
HighlightKeys=F1,F2,CAPS_LOCK
ActionKeys=F3,F4,NUM_LOCK

; Profile exported by SmartLogiLED v3.1.0
; Compatible with SmartLogiLED v3.0.0 and later
```

### System Tray Features
- **Double-click**: Restore main window from tray
- **Right-click Context Menu**:
  - **Open**: Restore main window
  - **Start Minimized**: Toggle startup behavior
  - **Close**: Exit application

## Technical Architecture

### Core Components
```
📁 SmartLogiLED/
├── SmartLogiLED.cpp              # Main UI and window management
├── SmartLogiLED_LockKeys.cpp     # Lock key control and keyboard hook
├── SmartLogiLED_AppProfiles.cpp  # Application monitoring and profile management  
├── SmartLogiLED_Config.cpp       # Registry persistence and configuration
├── SmartLogiLED_KeyMapping.cpp   # Key mapping and conversion utilities
├── SmartLogiLED_IniFiles.cpp     # Profile export/import functionality
├── SmartLogiLED_Dialogs.cpp      # Dialog management and UI interactions
├── SmartLogiLED_ProcessMonitor.cpp # Process monitoring and detection
├── Resource files                # UI resources and version information
└── Headers and project files
```

### Threading Model
- **Main Thread**: UI handling and user interaction
- **Monitor Thread**: Background application detection (1-second intervals)
- **Keyboard Hook**: Global low-level keyboard hook for real-time lock key detection
- **Mutex Protection**: Thread-safe access to shared profile data structures

### Performance Characteristics
- **CPU Usage**: Optimized monitoring with minimal system impact
- **Memory Footprint**: Efficient data structures with registry-based persistence
- **Response Time**: Real-time keyboard updates with immediate visual feedback
- **Resource Management**: Smart resource cleanup and proper handle management

## Configuration Details

### Registry Storage
Settings are stored in Windows Registry under:

```
HKEY_CURRENT_USER\Software\SmartLogiLED\
├── Color settings (DWORD RGB values)
├── StartMinimized (DWORD boolean)
└── AppProfiles\
    └── [ApplicationName]\
        ├── AppColor (DWORD)
        ├── AppHighlightColor (DWORD)
        ├── AppActionColor (DWORD)
        ├── LockKeysEnabled (DWORD)
        ├── HighlightKeys (BINARY array)
        └── ActionKeys (BINARY array)
```

### Application Monitoring Logic
The monitoring system uses sophisticated window detection:

```cpp
// Visibility criteria for application detection:
• Has visible main window (not minimized)
• Window has title text
• Process has main executable (not service)
• Excludes: Background services, hidden processes, system components
```

### Debug and Troubleshooting
Debug builds provide comprehensive logging via `OutputDebugStringW()`:
- Application start/stop detection events
- Profile state transitions and handoff logic
- Keyboard hook installation and key detection
- Color application and highlight/action key processing
- Registry operations and error conditions

## Troubleshooting Guide

### Common Issues and Solutions

**❗ "Couldn't initialize LogiTech LED SDK" Error**
```
💡 Solutions:
• Ensure Logitech Gaming Software or G HUB is installed and running
• Verify keyboard supports per-key RGB lighting
• Try running application as administrator
• Check if another application is controlling keyboard lighting
```

**❗ Application monitoring not detecting apps**
```
💡 Solutions:  
• Verify target application has visible windows (not minimized to tray)
• Check that executable name matches profile configuration exactly
• Ensure application is not running as background service
• Restart SmartLogiLED if hook becomes unresponsive
```

**❗ Colors not changing or stuck**
```
💡 Solutions:
• Check if multiple profiles are conflicting (most recent takes priority)
• Verify Logitech software is not overriding colors
• Restart application to reset keyboard hook
• Check profile settings - lock keys might be disabled
```

**❗ Highlight/Action keys not working properly**
```
💡 Solutions:
• Ensure highlight/action colors differ from app color for visibility
• Verify keys are properly configured using respective "H-Keys" and "A-Keys" buttons
• Check that profile is currently active (shown in dropdown)
• Lock keys override highlight/action colors when in active state
• Remember keys are mutually exclusive between highlight and action lists
```

**❗ Export/Import issues**
```
💡 Solutions:
• Verify write permissions to export destination folder
• Check INI file format matches expected structure
• Ensure key names in HighlightKeys/ActionKeys fields are valid
• Try importing to different profile name if conflicts occur
```

### Performance Optimization
- **Monitoring Interval**: Configurable in code (default: 1 second)
- **Hook Efficiency**: Optimized keyboard hook with minimal CPU overhead
- **Memory Management**: Automatic cleanup of unused profile data
- **Registry Optimization**: Batched registry operations for better performance

## Development and Customization

### Building from Source
```bash
git clone https://github.com/dg82wi/SmartLogiLED.git
cd SmartLogiLED
# Open SmartLogiLED.sln in Visual Studio
# Build in Debug or Release configuration
```

### Customization Options

#### Adding Custom Monitoring Logic
Modify `SmartLogiLED_ProcessMonitor.cpp` to customize application detection:
```cpp
// Adjust monitoring interval (default: 1000ms)
const DWORD MONITOR_INTERVAL = 1000;

// Customize visibility detection criteria
bool IsProcessVisible(DWORD processId, const std::wstring& processName);
```

#### Extending Profile Functionality
Add custom profile properties by extending the `AppColorProfile` structure:
```cpp
struct AppColorProfile {
    // Existing properties...
    
    // Add custom properties:
    COLORREF customColor1 = RGB(255, 255, 255);
    bool enableCustomFeature = false;
    std::vector<int> customSettings;
};
```

### Contributing Guidelines
1. **Fork and Branch**: Create feature branches from main
2. **Code Style**: Follow existing C++14 conventions and formatting
3. **Thread Safety**: Ensure new features are thread-safe with proper mutex usage
4. **Testing**: Test on multiple Logitech keyboard models if possible
5. **Documentation**: Update README and comments for new features

## Version History

### 🚀 v3.1.0 - Current Release
**Action Keys and Enhanced Key Management**

#### ✨ New Features
- **Action Key System**: Separate action keys with dedicated colors for shortcuts and special functions
- **Mutual Key Exclusivity**: Keys automatically removed from one list when added to another, preventing conflicts
- **Enhanced Key Management**: Improved key configuration dialogs with separate highlight and action key controls
- **Extended Export/Import**: INI files now include action keys and action colors for complete profile sharing

#### 🔧 Improvements
- **Intelligent Key Handling**: Smart conflict resolution when keys are added to different lists
- **Enhanced UI Feedback**: Clear visual indicators for key assignments and conflicts
- **Thread-Safe Key Updates**: Improved mutex protection for key list modifications
- **Registry Persistence**: Action keys and colors properly stored and restored from Windows registry

#### 📚 Documentation
- **Updated Usage Guide**: Comprehensive documentation for action key functionality
- **Enhanced Troubleshooting**: Additional solutions for key assignment conflicts
- **Technical Details**: Detailed explanation of mutual exclusivity implementation

---

### v3.0.0
**Major Feature Release - Complete Profile Management Overhaul**

#### ✨ New Features
- **Complete GUI Profile Management**: Intuitive add/remove functionality with visual indicators
- **Individual Key Highlighting**: Interactive key capture with separate highlight colors per profile
- **Profile Export/Import System**: Share profiles via INI files with full metadata preservation
- **Enhanced Color Management**: Owner-drawn color controls with integrated color pickers
- **Real-time Profile Updates**: Immediate visual feedback and automatic settings persistence
- **Smart Profile Creation**: Detect and add profiles from currently running applications

#### 🔧 Improvements
- **Intelligent Key Highlighting**: Respects lock key settings and state-based color priority
- **Enhanced User Experience**: Streamlined dialogs with better visual feedback
- **Modular Architecture**: Separated components for better maintainability
- **Thread Safety**: Comprehensive mutex protection for all profile operations

#### 🐛 Bug Fixes
- **Profile State Consistency**: Fixed race conditions in profile activation/deactivation
- **Color Application**: Resolved issues with highlight keys overriding lock key colors
- **Registry Persistence**: Improved reliability of settings storage and restoration

---

### v2.2.0
**Multi-Application Handling Enhancement**

#### 🔧 Improvements
- **Most Recently Activated Priority**: Better handling of multiple monitored applications
- **Enhanced Debug Logging**: Comprehensive troubleshooting information for app switching
- **Profile State Tracking**: Separate `isAppRunning` and `isProfileCurrInUse` flags
- **Intelligent Fallback Logic**: Smart handoff when active profiles stop
- **Thread Safety**: Improved mutex protection for profile operations

---

### v2.1.0
**Stability and Multi-App Improvements**

#### 🐛 Bug Fixes
- **Default Color Preservation**: App colors no longer overwrite user's base default color
- **Multi-App Handoff**: Intelligent switching between monitored applications

#### 🔧 Improvements  
- **Code Organization**: Separate configuration module for better maintainability
- **Registry Persistence**: Robust app profile storage and retrieval
- **Lock Key Consistency**: Better respect for app-specific lock key settings
- **Error Handling**: Enhanced thread safety and error recovery

---

### v2.0.0
**Application Monitoring Introduction**

#### ✨ New Features
- **Application Monitoring**: Automatic color switching based on running applications
- **Visible App Detection**: Smart filtering excludes background processes and services
- **Per-Application Lock Keys**: Individual lock key control for each profile
- **Start Minimized Option**: Configurable startup behavior with registry persistence

#### 🔧 Improvements
- **Registry Configuration**: Moved from INI files to Windows registry
- **Performance Optimization**: Efficient resource usage and thread management
- **Enhanced Error Handling**: Better recovery from SDK and system errors

---

### v1.0.0
**Initial Release**

#### ✨ Core Features
- **Basic Lock Key Control**: RGB color customization for NumLock, CapsLock, ScrollLock
- **System Tray Integration**: Minimize to tray with context menu support
- **Settings Persistence**: INI file-based configuration storage
- **Real-time Updates**: Global keyboard hook for instant lock key detection

## Support and Community

### Getting Help
- **GitHub Issues**: [Report bugs and request features](https://github.com/dg82wi/SmartLogiLED/issues)
- **Documentation**: Check this README and inline code comments
- **Debug Logs**: Enable debug builds for detailed troubleshooting information

### Supported Hardware
- **Tested Models**: Logitech G513
- **Compatibility**: Any Logitech RGB keyboard with per-key lighting support like G915, G815, G513, G413, G910, G810
- **Requirements**: Must be recognized by Logitech Gaming Software or G HUB

### Reporting Issues
When reporting issues, please include:
- Windows version and keyboard model
- Logitech software version (Gaming Software or G HUB)
- Steps to reproduce the issue  
- Any error messages or unexpected behavior
- Debug logs if available (Debug builds)

## License and Acknowledgments

### License
This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

### Acknowledgments
- **Logitech** for providing the LED SDK and comprehensive documentation
- **Microsoft** for Win32 API documentation and development tools
- **Open Source Community** for inspiration and feedback on RGB keyboard applications
- **Contributors** for bug reports, feature requests, and code contributions

### Third-Party Components
- **Logitech LED SDK**: Proprietary SDK for RGB lighting control
- **Windows API**: Core system integration and UI framework
- **Visual Studio**: Development environment and debugging tools

---

**💡 Pro Tip**: For the best experience, ensure your Logitech software is up to date and that SmartLogiLED is added to any antivirus exclusion lists to prevent interference with the global keyboard hook.

**🔗 Links**: [GitHub Repository](https://github.com/dg82wi/SmartLogiLED) | [Latest Release](https://github.com/dg82wi/SmartLogiLED/releases/latest) | [Issue Tracker](https://github.com/dg82wi/SmartLogiLED/issues)