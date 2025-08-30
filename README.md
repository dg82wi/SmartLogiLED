# SmartLogiLED

A Windows application that controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock) and provides application-specific color profiles, allowing users to customize colors via a GUI and system tray integration.

## Features

### Core Lighting Features
- **Lock Key Color Control**: Customize RGB colors for NumLock, CapsLock, and ScrollLock keys
- **Real-time Updates**: Lock key colors change automatically when toggled
- **Customizable Default Color**: Set a default color for all other keyboard keys
- **Application-Specific Color Profiles**: Automatically change keyboard colors based on running applications

### Application Monitoring
- **Smart App Detection**: Only monitors visible applications (no background processes or hidden services)
- **Automatic Color Switching**: Seamlessly switch keyboard colors when monitored applications start or stop
- **Lock Key Control per App**: Enable or disable lock key functionality for specific applications
- **Multiple Active Apps**: Intelligent handoff between multiple monitored applications
- **Efficient Resource Usage**: Optimized monitoring with minimal CPU impact

### User Interface & Integration
- **System Tray Integration**: Minimize to system tray with context menu support
- **Persistent Settings**: Color settings and app profiles are saved to registry and restored on startup
- **Start Minimized Option**: Configure application to start minimized to system tray

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
2. Click on any color box to open the color picker:
   - **NUM LOCK**: Color when NumLock is enabled
   - **CAPS LOCK**: Color when CapsLock is enabled
   - **SCROLL LOCK**: Color when ScrollLock is enabled
   - **Default Color**: Color for all other keyboard keys and when lock keys are disabled
3. Select your desired color and click OK
4. Colors are automatically saved to the Windows registry

### Application Monitoring
The application includes several predefined application profiles:
- **Notepad** (`notepad.exe`): Yellow color, lock keys enabled
- **Google Chrome** (`chrome.exe`): Cyan color, lock keys disabled
- **Mozilla Firefox** (`firefox.exe`): Orange color, lock keys disabled
- **Visual Studio Code** (`code.exe`): Green color, lock keys enabled
- **Visual Studio** (`devenv.exe`): Purple color, lock keys enabled

When any of these applications are visible and active:
- The keyboard automatically switches to the application's configured color
- Lock key behavior follows the application's lock key setting
- When the application closes, the keyboard returns to the default color or hands off to another active monitored app

### System Tray Features
- **Double-click**: Restore the main window
- **Right-click**: Show context menu with options:
  - **Open**: Restore the main window
  - **Start Minimized**: Toggle start minimized setting
  - **Close**: Exit the application

### Keyboard Behavior
- Lock key colors update instantly when you press NumLock, CapsLock, or ScrollLock
- The application runs a global keyboard hook to detect lock key state changes
- Lock key functionality can be disabled per application (keys will show default color)
- When the application starts, it immediately checks for running applications and applies colors

## Technical Details

### Architecture
- **Language**: C++14
- **Framework**: Win32 API
- **Dependencies**: Logitech LED SDK
- **Build System**: Visual Studio project files
- **Threading**: Multi-threaded application monitoring with mutex synchronization

### Key Components
- **Main Window**: Color picker interface with preview boxes
- **Keyboard Hook**: Global low-level keyboard hook for real-time lock key detection
- **App Monitor Thread**: Background thread monitoring visible applications every 2 seconds
- **Tray Integration**: System tray icon with context menu
- **Configuration Management**: Windows registry for persistent configuration
- **LED Control**: Integration with Logitech LED SDK for RGB lighting control

### Application Monitoring Details
- **Visibility Detection**: Only detects applications with visible, non-minimized windows
- **Process Filtering**: Excludes background services and system processes
- **Window Validation**: Checks for main application windows with titles
- **Performance Optimized**: Efficient scanning with minimal system impact
- **Thread Safety**: Mutex-protected data structures for safe multi-threading
- **Intelligent Handoff**: When multiple monitored apps are running, first active takes precedence

### File Structure
```
SmartLogiLED/
??? SmartLogiLED.cpp          # Main application code and UI
??? SmartLogiLED.h            # Application header and definitions
??? SmartLogiLED_Logic.cpp    # Core logic and app monitoring
??? SmartLogiLED_Logic.h      # Logic function declarations
??? SmartLogiLED_Config.cpp   # Registry persistence and configuration
??? SmartLogiLED_Config.h     # Configuration function declarations
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
- **Application Profiles**: Each app has its own subkey with color, highlight color, lock keys setting, and highlight keys

### Persistent App Profiles
Application profiles are now automatically saved to and loaded from the registry, including:
- Application name and colors
- Lock keys enabled/disabled setting
- Highlight colors and keys for future UI enhancements
- Active state (runtime only)

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

**Default color not restoring properly**
- This was a known issue in earlier versions where app colors would overwrite the user's default color
- Fixed in current version: SetDefaultColor no longer mutates the global defaultColor variable
- User's base default color is preserved and properly restored when apps exit

**Multiple app handoff issues**
- When multiple monitored apps are running, the first active app takes precedence
- When an active app stops, lighting automatically hands off to the next active monitored app
- If no monitored apps remain active, returns to user's default color

**Application doesn't start**
- Ensure all required DLLs are present (especially Logitech LED SDK libraries)
- Check Windows Event Viewer for error details
- Verify Visual C++ Redistributable is installed

**High CPU usage**
- The application is optimized for low CPU usage with 2-second monitoring intervals
- If experiencing issues, restart the application
- Check for conflicting keyboard software

### Debug Mode
When building in Debug configuration, additional error checking and logging may be available.

## Customization

### Adding New Application Profiles
Application profiles are now automatically persisted to the registry. To add new profiles programmatically:

1. Use the `AddAppColorProfile()` function:
   ```cpp
   AddAppColorProfile(L"yourapp.exe", RGB(255, 0, 0), true);  // Red color, lock keys enabled
   ```
2. Profiles are automatically saved to registry and restored on startup
3. Future versions will include a GUI for managing application profiles

### Modifying Monitoring Behavior
The monitoring thread can be customized in `SmartLogiLED_Logic.cpp`:
- **Scan Interval**: Modify the sleep duration in `AppMonitorThreadProc()`
- **Visibility Criteria**: Adjust the window detection logic in `IsProcessVisible()`
- **Process Filtering**: Modify the process enumeration in `GetVisibleRunningProcesses()`

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

### v2.1.0 (Current)
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