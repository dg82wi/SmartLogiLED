# SmartLogiLED Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.1.0] - Current Release

### ✨ Added - Action Keys and Enhanced Key Management
- **Action Key System**: Separate action keys with dedicated colors for shortcuts and special functions
- **Mutual Key Exclusivity**: Keys automatically removed from one list when added to another, preventing conflicts
- **Enhanced Key Management**: Improved key configuration dialogs with separate highlight and action key controls
- **Extended Export/Import**: INI files now include action keys and action colors for complete profile sharing
- **AppActionColor Support**: New color property for action keys in profiles and registry storage

### 🔧 Improved
- **Intelligent Key Handling**: Smart conflict resolution when keys are added to different lists
- **Enhanced UI Feedback**: Clear visual indicators for key assignments and conflicts
- **Thread-Safe Key Updates**: Improved mutex protection for key list modifications with `RemoveKeysFromListInternal()` helper
- **Registry Persistence**: Action keys and colors properly stored and restored from Windows registry
- **INI File Format**: Enhanced export/import with action key data preservation and backward compatibility

### 🐛 Fixed
- **Key List Conflicts**: Eliminated possibility of keys existing in both highlight and action lists simultaneously
- **Profile State Consistency**: Improved thread-safe updates when modifying key assignments
- **Color Application**: Proper handling of action key colors alongside highlight and lock key colors

### 📚 Documentation
- **Updated Usage Guide**: Comprehensive documentation for action key functionality and mutual exclusivity
- **Enhanced Troubleshooting**: Additional solutions for key assignment conflicts and color visibility
- **Technical Details**: Detailed explanation of mutual exclusivity implementation and thread safety

### 🔧 Technical Changes
- **Internal Optimizations**: Added `RemoveKeysFromListInternal()` helper function for efficient key conflict resolution
- **Enhanced Error Handling**: Better validation of key assignments during profile updates
- **Memory Efficiency**: Optimized key list management with std::remove_if and erase idiom

## [3.0.0]

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