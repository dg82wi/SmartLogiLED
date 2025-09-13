#pragma once

#include <windows.h>
#include <string>
#include <vector>

// Custom Windows messages for app events
#define WM_APP_STARTED (WM_USER + 102)
#define WM_APP_STOPPED (WM_USER + 103)

// Public interface for the Process Monitor
void InitializeAppMonitoring(HWND hMainWindow);
void CleanupAppMonitoring();
bool IsAppRunning(const std::wstring& appName);
std::vector<std::wstring> GetVisibleRunningProcesses();