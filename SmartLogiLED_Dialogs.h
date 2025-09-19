// SmartLogiLED_Dialogs.h : Header file for dialog functionality.
//
// This file contains declarations for all dialog-related functions including:
// - About, Help, and message box dialogs
// - Keys configuration dialog for highlight keys
// - Action Keys configuration dialog
// - Add Profile dialog for creating new app profiles
// - Dialog utility functions

#pragma once

#include "framework.h"
#include "LogitechLEDLib.h"
#include <vector>
#include <string>

// Dialog procedures
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Help(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK KeysDialog(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ActionKeysDialog(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AddProfileDialog(HWND, UINT, WPARAM, LPARAM);

// Dialog launcher functions
void ShowKeysDialog(HWND hWnd);
void ShowActionKeysDialog(HWND hWnd);
void ShowAddProfileDialog(HWND hWnd);

// Dialog hook procedures for key capture
LRESULT CALLBACK KeysDialogKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ActionKeysDialogKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

// Color picker dialogs
void ShowAppColorPicker(HWND hWnd, int colorType);

// Dialog utility functions
void RefreshAppProfileCombo(HWND hWnd);