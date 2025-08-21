# SmartLogiLED

A Windows application that controls Logitech RGB keyboard lighting for lock keys (NumLock, CapsLock, ScrollLock), allowing users to customize colors via a GUI and system tray integration.

## Features

- **Lock Key Color Control**: Customize RGB colors for NumLock, CapsLock, and ScrollLock keys
- **Real-time Updates**: Lock key colors change automatically when toggled
- **System Tray Integration**: Minimize to system tray with context menu support
- **Persistent Settings**: Color settings are saved to `settings.ini` and restored on startup
- **Customizable Default Color**: Set a default color for all other keyboard keys
- **Off Color Configuration**: Define a specific color for when lock keys are disabled

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
   git clone https://github.com/your-username/SmartLogiLED.git
   ```
2. Open the project in Visual Studio
3. Build the solution (Debug or Release configuration)
4. Run the executable from the output directory

## Usage

### First Launch
1. Run `SmartLogiLED.exe`
2. The application will minimize to the system tray automatically
3. If the Logitech LED SDK cannot be initialized, an error message will appear

### Configuring Colors
1. Double-click the system tray icon or right-click and select "Open"
2. Click on any color box to open the color picker:
   - **NUM LOCK**: Color when NumLock is enabled
   - **CAPS LOCK**: Color when CapsLock is enabled
   - **SCROLL LOCK**: Color when ScrollLock is enabled
   - **Off**: Color when lock keys are disabled
   - **Other Keys**: Default color for all other keyboard keys
3. Select your desired color and click OK
4. Colors are automatically saved to `settings.ini`

### System Tray Features
- **Double-click**: Restore the main window
- **Right-click**: Show context menu with options:
  - **Open**: Restore the main window
  - **Close**: Exit the application

### Keyboard Behavior
- Lock key colors update instantly when you press NumLock, CapsLock, or ScrollLock
- The application runs a global keyboard hook to detect lock key state changes
- When the application starts, it sets all keys to the default color and applies current lock key states

## Configuration File

Settings are stored in `settings.ini` in the application directory:

```ini
[Colors]
NumLock=0,179,0
CapsLock=0,179,0
ScrollLock=0,179,0
LockKeyOff=0,89,89
Default=0,89,89
```

Colors are stored as RGB values (0-255 range).

## Technical Details

### Architecture
- **Language**: C++14
- **Framework**: Win32 API
- **Dependencies**: Logitech LED SDK
- **Build System**: Visual Studio project files

### Key Components
- **Main Window**: Color picker interface with preview boxes
- **Keyboard Hook**: Global low-level keyboard hook for real-time lock key detection
- **Tray Integration**: System tray icon with context menu
- **Settings Management**: INI file parsing for persistent configuration
- **LED Control**: Integration with Logitech LED SDK for RGB lighting control

### File Structure
```
SmartLogiLED/
??? SmartLogiLED.cpp          # Main application code
??? SmartLogiLED.h            # Application header
??? Resource.h                # Resource definitions
??? framework.h               # Framework includes
??? LogitechLEDLib.h          # Logitech SDK header
??? SmartLogiLED.rc           # Resource file
??? SmartLogiLED.vcxproj      # Visual Studio project
??? settings.ini              # Configuration file (created at runtime)
```

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

**Application doesn't start**
- Ensure all required DLLs are present (especially Logitech LED SDK libraries)
- Check Windows Event Viewer for error details
- Verify Visual C++ Redistributable is installed

### Debug Mode
When building in Debug configuration, additional error checking and logging may be available.

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

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Logitech for providing the LED SDK
- Microsoft for the Win32 API documentation
- The RGB keyboard community for inspiration and feedback

## Version History

### v1.0.0 (Current)
- Initial release
- Basic lock key color control
- System tray integration
- Settings persistence
- Real-time keyboard hook integration

## Support

For bugs, feature requests, or general questions:
- Create an issue on GitHub
- Check existing issues for similar problems
- Provide detailed information about your system and keyboard model

---

**Note**: This application requires a compatible Logitech RGB keyboard and the official Logitech software to function properly.