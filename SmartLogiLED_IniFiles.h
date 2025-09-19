#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <sstream>
#include "LogitechLEDLib.h"
#include "SmartLogiLED_Types.h"

// INI file export/import functionality

// Export functionality
void ExportSelectedProfileToIniFile(HWND hWnd);
void ExportAllProfilesToIniFiles();

// Import functionality  
void ImportProfileFromIniFile(HWND hWnd);

// Helper functions for INI file operations
void UpdateOrCreateProfileIniFile(const std::wstring& filename, const AppColorProfile& profile);
void AddMissingProfileKeys(std::wstringstream& content, const AppColorProfile& profile, const std::set<std::wstring>& seenKeys);
std::wstring GetApplicationDirectory();
std::wstring GetDefaultExportDirectory();

// Callback for folder browser dialog
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);