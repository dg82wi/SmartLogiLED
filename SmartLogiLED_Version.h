#pragma once

// SmartLogiLED Version Information
// This file contains centralized version constants for the SmartLogiLED application

#include <string>

// Version constants
#define SMARTLOGILED_VERSION_MAJOR      3
#define SMARTLOGILED_VERSION_MINOR      1
#define SMARTLOGILED_VERSION_PATCH      0
#define SMARTLOGILED_VERSION_BUILD      0

// String representations
#define SMARTLOGILED_VERSION_STRING     L"3.1.0"
#define SMARTLOGILED_VERSION_STRING_A   "3.1.0"
#define SMARTLOGILED_VERSION_FULL       L"3.1.0.0"
#define SMARTLOGILED_VERSION_FULL_A     "3.1.0.0"

// Product information
#define SMARTLOGILED_PRODUCT_NAME       L"SmartLogiLED"
#define SMARTLOGILED_PRODUCT_NAME_A     "SmartLogiLED"
#define SMARTLOGILED_COMPANY_NAME       L"Günter DANKL"
#define SMARTLOGILED_COMPANY_NAME_A     "Günter DANKL"
#define SMARTLOGILED_COPYRIGHT          L"Copyright (c) 2025 Günter DANKL"
#define SMARTLOGILED_COPYRIGHT_A        "Copyright (c) 2025 Günter DANKL"

// Build configuration
#ifdef _DEBUG
    #define SMARTLOGILED_BUILD_TYPE     L"Debug"
    #define SMARTLOGILED_BUILD_TYPE_A   "Debug"
#else
    #define SMARTLOGILED_BUILD_TYPE     L"Release"
    #define SMARTLOGILED_BUILD_TYPE_A   "Release"
#endif

// Version comparison macros
#define SMARTLOGILED_VERSION_NUMBER     ((SMARTLOGILED_VERSION_MAJOR << 24) | \
                                        (SMARTLOGILED_VERSION_MINOR << 16) | \
                                        (SMARTLOGILED_VERSION_PATCH << 8) | \
                                        SMARTLOGILED_VERSION_BUILD)

// Helper macros for version checking
#define SMARTLOGILED_VERSION_AT_LEAST(major, minor, patch) \
    (SMARTLOGILED_VERSION_NUMBER >= (((major) << 24) | ((minor) << 16) | ((patch) << 8)))

// Application identification
#define SMARTLOGILED_APP_ID             L"SmartLogiLED"
#define SMARTLOGILED_WINDOW_CLASS       L"SmartLogiLEDClass"
#define SMARTLOGILED_APP_DESCRIPTION    L"Advanced Logitech RGB Keyboard Controller"

// Function declarations for version information (implemented in SmartLogiLED.cpp)
std::wstring GetApplicationVersion();
std::wstring GetApplicationFullVersion();
std::wstring GetApplicationName();
DWORD GetVersionNumber();

// Inline helper functions for convenience
inline bool IsVersionAtLeast(int major, int minor, int patch = 0) {
    return SMARTLOGILED_VERSION_AT_LEAST(major, minor, patch);
}

inline std::wstring GetVersionWithBuildType() {
    std::wstring version = SMARTLOGILED_VERSION_STRING;
    version += L" (";
    version += SMARTLOGILED_BUILD_TYPE;
    version += L")";
    return version;
}