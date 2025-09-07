# SmartLogiLED

A Windows application that controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock) and provides application-specific color profiles with individual key highlighting capabilities, allowing users to customize colors via a GUI and system tray integration.

## Features

### Core Lighting Features
- **Lock Key Color Control**: Customize RGB colors for NumLock, CapsLock, and ScrollLock keys
- **Real-time Updates**: Lock key colors change automatically when toggled
- **Customizable Default Color**: Set a default color for all other keyboard keys
- **Application-Specific Color Profiles**: Automatically change keyboard colors based on running applications
- **Individual Key Highlighting**: Configure specific keys to use custom highlight colors per application

### Application Monitoring
- **Smart App Detection**: Only monitors visible applications with main windows (no background processes or hidden services)
- **Automatic Color Switching**: Seamlessly switch keyboard colors when monitored applications start or stop
- **Lock Key Control per App**: Enable or disable lock key functionality for specific applications
- **Multiple Active Apps**: Intelligent handoff with most recently activated app priority
- **Efficient Resource Usage**: Optimized monitoring with minimal CPU impact (2-second intervals)

### Advanced Profile Management
- **GUI Profile Creation**: Add new application profiles through an intuitive dialog
- **Individual Key Highlighting**: Configure custom keys to highlight with different colors per profile
- **Visual Color Management**: Separate color controls for app background and highlight colors
- **Profile Export/Import**: Export profiles to INI files and import them back
- **Registry Persistence**: All settings automatically saved to Windows registry
- **Real-time Profile Updates**: Changes apply immediately to active profiles

### User Interface & Integration
- **System Tray Integration**: Minimize to system tray with context menu support
- **Persistent Settings**: Color settings and app profiles are saved to registry and restored on startup
- **Start Minimized Option**: Configure application to start minimized to system tray
- **Visual Profile Indicators**: Current active profile displayed in main window
- **Interactive Key Configuration**: Click-and-capture key highlighting setup

## Requirements

- Windows operating system
- Logitech RGB keyboard with per-key lighting support
- Logitech Gaming Software or G HUB installed
- Visual Studio 2019 or later (for building from source)
- C++14 compatible compiler

## Installation

### Pre-built Binary
1. Download the latest release from the releases page
2. Extract the files to your desired location
3. Run `SmartLogiLED.exe`

### Building from Source
1. Clone this repository:
   ```bash
   git clone https://github.com/dg82wi/SmartLogiLED.git
   ```
2. Open the project in Visual Studio
3. Build the solution (Debug or Release configuration)
4. Run the executable from the output directory

## Usage

### First Launch
1. Run `SmartLogiLED.exe`
2. The application will start with default settings and begin monitoring applications
3. If the Logitech LED SDK cannot be initialized, an error message will appear

### Configuring Colors
1. Double-click the system tray icon or right-click and select "Open"
2. Click on any color box in the "Lock Keys Color" section to open the color picker:
   - **NUM LOCK**: Color when NumLock is enabled
   - **CAPS LOCK**: Color when CapsLock is enabled
   - **SCROLL LOCK**: Color when ScrollLock is enabled
   - **Default Color**: Color for all other keyboard keys and when lock keys are disabled
3. Select your desired color and click OK
4. Colors are automatically saved to the Windows registry

### Application Profile Management

#### Viewing Current Profiles
- The "App Profile" section shows the currently active profile
- A dropdown lists all configured profiles plus "NONE" (no active profile)
- The "Profile in use:" label shows which profile is currently controlling keyboard colors

#### Adding New Profiles
1. Click the "+" button next to the profile dropdown
2. Select an application from the dropdown (shows currently running visible applications)
3. Or manually type an application executable name (e.g., "notepad.exe")
4. Click "Done" to create the profile with default settings:
   - **App Color**: Cyan (RGB 0,255,255)
   - **Highlight Color**: Red (RGB 255,0,0)
   - **Lock Keys**: Disabled by default

#### Configuring Profile Colors
1. Select a profile from the dropdown (not "NONE")
2. Click the "App Color" box to set the main keyboard color for this application
3. Click the "Highlight Color" box to set the color for individually highlighted keys
4. Check/uncheck "Lock Keys" to enable/disable lock key visualization for this profile

#### Individual Key Highlighting
1. Select a profile from the dropdown
2. Click the "Keys" button to open the key configuration dialog
3. Press any keys you want to highlight - they will be added/removed from the highlight list
4. Use "Reset" to clear all highlighted keys
5. Click "Done" to save the configuration

#### Removing Profiles
1. Select a profile from the dropdown (not "NONE")
2. Click the "-" button
3. Confirm the removal when prompted

### Export/Import Functionality

#### Exporting Profiles
1. Go to File ? Export Profiles (or use the menu)
2. Select a destination folder
3. All profiles will be exported as individual INI files named `SmartLogiLED_<appname>.ini`

#### Importing Profiles
1. Go to File ? Import Profile (or use the menu)
2. Select an INI file to import
3. If a profile with the same name exists, you'll be prompted to overwrite

### Application Monitoring Behavior
When monitored applications are running:
- The keyboard automatically switches to the application's configured color
- Lock key behavior follows the application's lock key setting
- Individual highlighted keys use the profile's highlight color
- When the application closes, the keyboard returns to the default color or hands off to another active monitored app
- **Most Recently Activated Priority**: The most recently started monitored app takes precedence

### System Tray Features
- **Double-click**: Restore the main window
- **Right-click**: Show context menu with options:
  - **Open**: Restore the main window
  - **Start Minimized**: Toggle start minimized setting
  - **Close**: Exit the application

### Keyboard Behavior
- Lock key colors update instantly when you press NumLock, CapsLock, or ScrollLock
- The application runs a global keyboard hook to detect lock key state changes
- Lock key functionality can be disabled per application (keys will show app color instead of lock colors)
- Individual highlighted keys override the default app color
- When the application starts, it immediately checks for running applications and applies colors

## Technical Details

### Architecture
- **Language**: C++14
- **Framework**: Win32 API
- **Dependencies**: Logitech LED SDK
- **Build System**: Visual Studio project files
- **Threading**: Multi-threaded application monitoring with mutex synchronization

### Key Components
- **Main Window**: Color picker interface with profile management
- **Keyboard Hook**: Global low-level keyboard hook for real-time lock key detection and key capture
- **App Monitor Thread**: Background thread monitoring visible applications every 2 seconds
- **Tray Integration**: System tray icon with context menu
- **Configuration Management**: Windows registry for persistent configuration
- **LED Control**: Integration with Logitech LED SDK for RGB lighting control
- **Profile Manager**: GUI-based profile creation, editing, and removal
- **Export/Import System**: INI file-based profile sharing

### Application Monitoring Details
- **Visibility Detection**: Only detects applications with visible, non-minimized main windows
- **Process Filtering**: Excludes background services and system processes
- **Window Validation**: Checks for main application windows with titles
- **Performance Optimized**: Efficient scanning with minimal system impact
- **Thread Safety**: Mutex-protected data structures for safe multi-threading
- **Intelligent Handoff**: Most recently activated monitored app takes precedence
- **Debug Logging**: Comprehensive debug output for troubleshooting app switching logic

### Enhanced Profile Management
- **Profile State Tracking**: Each profile tracks both running state (`isAppRunning`) and display state (`isProfileCurrInUse`)
- **Most Recently Activated Logic**: The `lastActivatedProfile` variable tracks which profile should have priority
- **Fallback Logic**: When the active profile stops, control passes to the most recently activated profile or first available
- **Thread-Safe Operations**: All profile operations protected by mutex for safe concurrent access
- **Individual Key Control**: Each profile can specify custom highlight keys with separate colors
- **Real-time Updates**: Profile changes apply immediately to active applications

### File Structure
```
SmartLogiLED/
??? SmartLogiLED.cpp          # Main application code and UI
??? SmartLogiLED.h            # Application header and definitions
??? SmartLogiLED_Logic.cpp    # Core logic and app monitoring
??? SmartLogiLED_Logic.h      # Logic function declarations
??? SmartLogiLED_Config.cpp   # Registry persistence and configuration
??? SmartLogiLED_Config.h     # Configuration function declarations
??? SmartLogiLED_KeyMapping.cpp # Key mapping and conversion functions
??? SmartLogiLED_KeyMapping.h   # Key mapping function declarations
??? Resource.h                # Resource definitions
??? framework.h               # Framework includes
??? LogitechLEDLib.h          # Logitech SDK header
??? SmartLogiLED.rc           # Resource file
??? SmartLogiLED.vcxproj      # Visual Studio project
??? README.md                 # This file
??? APP_MONITORING_USAGE.md   # Detailed app monitoring guide
```

## Configuration

Settings are stored in the Windows registry under:
- **Main Settings**: `HKEY_CURRENT_USER\Software\SmartLogiLED`
- **App Profiles**: `HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles`

### Registry Values
- **Colors**: RGB values for lock keys and default color (stored as DWORD)
- **StartMinimized**: Boolean flag for start minimized behavior
- **Application Profiles**: Each app has its own subkey with:
  - `AppColor` (DWORD): Main application color
  - `AppHighlightColor` (DWORD): Color for highlighted keys
  - `LockKeysEnabled` (DWORD): 1 = enabled, 0 = disabled
  - `HighlightKeys` (BINARY): Array of key codes for highlighting

### Profile INI File Format
```ini
[SmartLogiLED Profile]
AppName=notepad.exe
AppColor=FFFF00
AppHighlightColor=FF0000
LockKeysEnabled=1
HighlightKeys=F1,F2,CAPS_LOCK,NUM_LOCK

; Comments and instructions
```

### Persistent App Profiles
Application profiles are automatically saved to and loaded from the registry, including:
- Application name and colors (app and highlight)
- Lock keys enabled/disabled setting
- Individual highlight keys configuration
- Running state (runtime only) and display state tracking

## Troubleshooting

### Common Issues

**"Couldn't initialize LogiTech LED SDK" Error**
- Ensure Logitech Gaming Software or G HUB is installed and running
- Verify your keyboard supports per-key RGB lighting
- Try running the application as administrator

**"Couldn't set LOGI_DEVICETYPE_PERKEY_RGB mode" Error**
- Your keyboard may not support per-key RGB lighting
- Check if your keyboard is recognized in Logitech Gaming Software/G HUB

**Colors not changing**
- Verify the keyboard hook is working by checking if the tray icon responds
- Restart the application
- Check if another application is controlling the keyboard lighting

**Application monitoring not working**
- Ensure the target application has visible windows (not minimized)
- Check if the application executable name matches the configured profile
- Verify the application is not running as a background service

**Profile colors not showing correctly**
- Check that the correct profile is selected in the dropdown
- Verify the profile has been saved (colors should persist after restart)
- Individual highlighted keys override the main app color

**Key highlighting not working**
- Ensure you've configured highlight keys using the "Keys" button
- Verify the highlight color is different from the app color
- Check that the profile is currently active

**Export/Import issues**
- Ensure you have write permissions to the export folder
- Check that imported INI files have the correct format
- Verify key names in HighlightKeys field are valid

**Default color not restoring properly**
- This was a known issue in earlier versions where app colors would overwrite the user's default color
- Fixed in current version: SetDefaultColor no longer mutates the global defaultColor variable
- User's base default color is preserved and properly restored when apps exit

**Multiple app handoff issues**
- **Most Recently Activated Priority**: When multiple monitored apps are running, the most recently started app takes control
- **Intelligent Fallback**: When an active app stops, control passes to the most recently activated profile that's still running
- **Debug Logging**: Check debug output for detailed app switching behavior

**Application doesn't start**
- Ensure all required DLLs are present (especially Logitech LED SDK libraries)
- Check Windows Event Viewer for error details
- Verify Visual C++ Redistributable is installed

**High CPU usage**
- The application is optimized for low CPU usage with 2-second monitoring intervals
- If experiencing issues, restart the application
- Check for conflicting keyboard software

### Debug Mode
When building in Debug configuration, comprehensive debug logging is available via `OutputDebugStringW` calls, including:
- App start/stop detection
- Profile state changes (`isAppRunning` and `isProfileCurrInUse`)
- Most recently activated profile tracking
- Color handoff logic
- Key highlighting operations

## Customization

### Adding New Application Profiles via GUI
1. Click the "+" button in the App Profile section
2. Select from running applications or manually enter an executable name
3. Configure colors and settings as needed
4. Profiles are automatically saved to registry

### Programmatically Adding Profiles
Application profiles can also be added programmatically:

```cpp
// Add a basic profile
AddAppColorProfile(L"yourapp.exe", RGB(255, 0, 0), true);  // Red color, lock keys enabled

// Configure highlight keys
std::vector<LogiLed::KeyName> highlightKeys = {LogiLed::KeyName::W, LogiLed::KeyName::A, LogiLed::KeyName::S, LogiLed::KeyName::D};
UpdateAppProfileHighlightKeys(L"yourapp.exe", highlightKeys);
UpdateAppProfileHighlightColor(L"yourapp.exe", RGB(255, 255, 0)); // Yellow highlights
```

### Modifying Monitoring Behavior
The monitoring thread can be customized in `SmartLogiLED_Logic.cpp`:
- **Scan Interval**: Modify the sleep duration in `AppMonitorThreadProc()`
- **Visibility Criteria**: Adjust the window detection logic in `IsProcessVisible()`
- **Process Filtering**: Modify the process enumeration in `GetVisibleRunningProcesses()`
- **Priority Logic**: Adjust the most recently activated profile handling

### Registry Configuration
Advanced users can modify settings directly in the registry:
- Navigate to `HKEY_CURRENT_USER\Software\SmartLogiLED\AppProfiles`
- Each application has its own subkey with configurable values
- Changes take effect after restarting the application

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/new-feature`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/new-feature`)
5. Create a Pull Request

### Code Style
- Follow the existing code style and formatting
- Add comments for complex logic
- Ensure compatibility with C++14
- Test on multiple Logitech keyboard models if possible
- Ensure thread safety for any shared data structures

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Logitech for providing the LED SDK
- Microsoft for the Win32 API documentation
- The RGB keyboard community for inspiration and feedback

## Version History

### v3.0.0 (Current)
- **NEW**: Complete GUI profile management with add/remove functionality
- **NEW**: Individual key highlighting with interactive key capture
- **NEW**: Separate highlight color configuration per profile
- **NEW**: Profile export/import functionality via INI files
- **NEW**: Visual profile status indicators and real-time updates
- **NEW**: Enhanced color management with owner-drawn controls
- **IMPROVED**: Profile creation from currently running applications
- **IMPROVED**: Intelligent key highlighting that respects lock key settings
- **IMPROVED**: Better user experience with immediate visual feedback

### v2.2.0
- **IMPROVED**: Most recently activated app priority system for better multi-app handling
- **NEW**: Enhanced debug logging for troubleshooting app switching behavior
- **IMPROVED**: Profile state tracking with separate `isAppRunning` and `isProfileCurrInUse` flags
- **IMPROVED**: Intelligent fallback logic when active profiles stop
- **IMPROVED**: Better thread safety and mutex protection for profile operations

### v2.1.0
- **FIXED**: Default color preservation - app colors no longer overwrite user's base default color
- **IMPROVED**: Multiple active app handoff - intelligent switching between monitored applications
- **NEW**: Separate configuration module (`SmartLogiLED_Config.cpp`) for better code organization
- **NEW**: Persistent app profile storage in Windows registry
- **IMPROVED**: Lock keys feature respects app-specific settings more consistently
- **IMPROVED**: Enhanced thread safety and error handling

### v2.0.0
- **NEW**: Application monitoring and automatic color switching
- **NEW**: Visible application detection (excludes background processes)
- **NEW**: Per-application lock key control
- **NEW**: Start minimized option with registry persistence
- **IMPROVED**: Settings now stored in Windows registry
- **IMPROVED**: Enhanced error handling and thread safety
- **IMPROVED**: Optimized resource usage and performance

### v1.0.0
- Initial release
- Basic lock key color control
- System tray integration
- Settings persistence via INI file
- Real-time keyboard hook integration

## Support

For bugs, feature requests, or general questions:
- Create an issue on GitHub: https://github.com/dg82wi/SmartLogiLED/issues
- Check existing issues for similar problems
- Provide detailed information about your system and keyboard model

---

**Note**: This application requires a compatible Logitech RGB keyboard and the official Logitech software to function properly. The application monitoring feature works best with applications that have visible windows and may not detect console applications or background services.