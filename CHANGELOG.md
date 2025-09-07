# SmartLogiLED Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.0.0] - Current Release

### ✨ Added - Major Feature Release
- **Complete GUI Profile Management**: Intuitive add/remove functionality with visual indicators
- **Individual Key Highlighting**: Interactive key capture with separate highlight colors per profile
- **Profile Export/Import System**: Share profiles via INI files with full metadata preservation
- **Enhanced Color Management**: Owner-drawn color controls with integrated color pickers
- **Real-time Profile Updates**: Immediate visual feedback and automatic settings persistence
- **Smart Profile Creation**: Detect and add profiles from currently running applications
- **Comprehensive Version Information**: Added VERSIONINFO resource with proper metadata
- **Enhanced Documentation**: Complete README overhaul with detailed usage guides and troubleshooting

### 🔧 Improved
- **Intelligent Key Highlighting**: Respects lock key settings and state-based color priority
- **Enhanced User Experience**: Streamlined dialogs with better visual feedback
- **Modular Architecture**: Separated components for better maintainability
- **Thread Safety**: Comprehensive mutex protection for all profile operations
- **Help System**: Updated help dialogs with comprehensive feature documentation

### 🐛 Fixed
- **Profile State Consistency**: Fixed race conditions in profile activation/deactivation
- **Color Application**: Resolved issues with highlight keys overriding lock key colors
- **Registry Persistence**: Improved reliability of settings storage and restoration

### 📚 Documentation
- **README.md**: Complete rewrite with modern formatting, comprehensive feature documentation
- **Technical Architecture**: Detailed component breakdown and threading model documentation
- **Troubleshooting Guide**: Extensive troubleshooting section with solutions for common issues
- **Usage Examples**: Detailed examples for different use cases (gaming, development, productivity)
- **Development Guide**: Instructions for building from source and customization options

## [2.2.0]

### 🔧 Improved - Multi-Application Handling Enhancement
- **Most Recently Activated Priority**: Better handling of multiple monitored applications
- **Enhanced Debug Logging**: Comprehensive troubleshooting information for app switching
- **Profile State Tracking**: Separate `isAppRunning` and `isProfileCurrInUse` flags
- **Intelligent Fallback Logic**: Smart handoff when active profiles stop
- **Thread Safety**: Improved mutex protection for profile operations

## [2.1.0]

### 🐛 Fixed - Stability and Multi-App Improvements
- **Default Color Preservation**: App colors no longer overwrite user's base default color
- **Multi-App Handoff**: Intelligent switching between monitored applications

### 🔧 Improved
- **Code Organization**: Separate configuration module for better maintainability
- **Registry Persistence**: Robust app profile storage and retrieval
- **Lock Key Consistency**: Better respect for app-specific lock key settings
- **Error Handling**: Enhanced thread safety and error recovery

## [2.0.0]

### ✨ Added - Application Monitoring Introduction
- **Application Monitoring**: Automatic color switching based on running applications
- **Visible App Detection**: Smart filtering excludes background processes and services
- **Per-Application Lock Keys**: Individual lock key control for each profile
- **Start Minimized Option**: Configurable startup behavior with registry persistence

### 🔧 Improved
- **Registry Configuration**: Moved from INI files to Windows registry
- **Performance Optimization**: Efficient resource usage and thread management
- **Enhanced Error Handling**: Better recovery from SDK and system errors

### ⚠️ Breaking Changes
- Configuration moved from INI files to Windows registry
- Profile data structure changes require profile recreation

## [1.0.0]

### ✨ Added - Initial Release
- **Basic Lock Key Control**: RGB color customization for NumLock, CapsLock, ScrollLock
- **System Tray Integration**: Minimize to tray with context menu support
- **Settings Persistence**: INI file-based configuration storage
- **Real-time Updates**: Global keyboard hook for instant lock key detection
- **Color Picker Integration**: Windows color chooser dialog for easy color selection

### 🏗️ Technical Foundation
- **Logitech LED SDK Integration**: Support for per-key RGB lighting
- **Windows API Integration**: Native Windows application with proper resource management
- **Multi-threaded Architecture**: Separate threads for UI and keyboard monitoring
- **Error Handling**: Basic error handling for SDK initialization and device communication

---

## Version Numbering

This project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html):

- **MAJOR** version when making incompatible API changes
- **MINOR** version when adding functionality in a backwards compatible manner
- **PATCH** version when making backwards compatible bug fixes

## Support Information

- **Supported OS**: Windows 7 and later
- **Supported Hardware**: Logitech RGB keyboards with per-key lighting
- **Dependencies**: Logitech Gaming Software or G HUB
- **Build Requirements**: Visual Studio 2019+, C++14

## Links

- [GitHub Repository](https://github.com/dg82wi/SmartLogiLED)
- [Latest Release](https://github.com/dg82wi/SmartLogiLED/releases/latest)
- [Issue Tracker](https://github.com/dg82wi/SmartLogiLED/issues)
- [MIT License](LICENSE)