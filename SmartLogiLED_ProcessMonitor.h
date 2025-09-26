#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "SmartLogiLED_Types.h"


// Public interface for the Process Monitor
void InitializeAppMonitoring(HWND hMainWindow);
void CleanupAppMonitoring();
bool IsAppRunning(const std::wstring& appName);
bool IsProcessRunning(const std::wstring& processName); // New function for any process detection
std::vector<std::wstring> GetVisibleRunningProcesses();