// SmartLogiLED_KeyMapping.h : Header file for key mapping functions.
//
// This file contains functions for converting between different key representations:
// - Virtual Key codes to LogiLed::KeyName
// - LogiLed::KeyName to display names
// - Config names to LogiLed::KeyName (for INI import/export)
// - LogiLed::KeyName to config names

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"
#include <string>
#include <vector>

// Convert Virtual Key code to LogiLed::KeyName
LogiLed::KeyName VirtualKeyToLogiLedKey(DWORD vkCode);

// Convert LogiLed::KeyName to display name for UI
std::wstring LogiLedKeyToDisplayName(LogiLed::KeyName key);

// Convert config name to LogiLed::KeyName (for INI import)
LogiLed::KeyName ConfigNameToLogiLedKey(const std::wstring& configName);

// Convert LogiLed::KeyName to config name (for INI export)
std::wstring LogiLedKeyToConfigName(LogiLed::KeyName key);

// Format highlight keys for display in text field
std::wstring FormatHighlightKeysForDisplay(const std::vector<LogiLed::KeyName>& keys);