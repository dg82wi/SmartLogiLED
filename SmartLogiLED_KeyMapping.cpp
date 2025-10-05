// SmartLogiLED_KeyMapping.cpp : Key mapping functions for SmartLogiLED application.
//
// This file contains functions for converting between different key representations.

#include "SmartLogiLED_KeyMapping.h"
#include <algorithm> 
#include <windows.h>

// Add this helper function
bool IsQWERTZLayout() {
    HKL layout = GetKeyboardLayout(0);
    LANGID langId = LOWORD(layout);
    // Check for German, Austrian, Czech, Slovak layouts
    return (PRIMARYLANGID(langId) == LANG_GERMAN || 
            PRIMARYLANGID(langId) == LANG_CZECH ||
            PRIMARYLANGID(langId) == LANG_SLOVAK);
}

// Convert Virtual Key code to LogiLed::KeyName with extended key information
LogiLed::KeyName VirtualKeyToLogiLedKey(DWORD vkCode, DWORD flags = 0) {
    switch (vkCode) {
        case VK_ESCAPE: return LogiLed::KeyName::ESC;
        case VK_F1: return LogiLed::KeyName::F1;
        case VK_F2: return LogiLed::KeyName::F2;
        case VK_F3: return LogiLed::KeyName::F3;
        case VK_F4: return LogiLed::KeyName::F4;
        case VK_F5: return LogiLed::KeyName::F5;
        case VK_F6: return LogiLed::KeyName::F6;
        case VK_F7: return LogiLed::KeyName::F7;
        case VK_F8: return LogiLed::KeyName::F8;
        case VK_F9: return LogiLed::KeyName::F9;
        case VK_F10: return LogiLed::KeyName::F10;
        case VK_F11: return LogiLed::KeyName::F11;
        case VK_F12: return LogiLed::KeyName::F12;
        case VK_SNAPSHOT: return LogiLed::KeyName::PRINT_SCREEN;
        case VK_SCROLL: return LogiLed::KeyName::SCROLL_LOCK;
        case VK_PAUSE: return LogiLed::KeyName::PAUSE_BREAK;
        case VK_OEM_3: return LogiLed::KeyName::TILDE; // ~ key
        case '1': return LogiLed::KeyName::ONE;
        case '2': return LogiLed::KeyName::TWO;
        case '3': return LogiLed::KeyName::THREE;
        case '4': return LogiLed::KeyName::FOUR;
        case '5': return LogiLed::KeyName::FIVE;
        case '6': return LogiLed::KeyName::SIX;
        case '7': return LogiLed::KeyName::SEVEN;
        case '8': return LogiLed::KeyName::EIGHT;
        case '9': return LogiLed::KeyName::NINE;
        case '0': return LogiLed::KeyName::ZERO;
        case VK_OEM_MINUS: return LogiLed::KeyName::MINUS;
        case VK_OEM_PLUS: return LogiLed::KeyName::EQUALS;
        case VK_BACK: return LogiLed::KeyName::BACKSPACE;
        case VK_INSERT: return LogiLed::KeyName::INSERT;
        case VK_HOME: return LogiLed::KeyName::HOME;
        case VK_PRIOR: return LogiLed::KeyName::PAGE_UP;
        case VK_NUMLOCK: return LogiLed::KeyName::NUM_LOCK;
        case VK_DIVIDE: return LogiLed::KeyName::NUM_SLASH;
        case VK_MULTIPLY: return LogiLed::KeyName::NUM_ASTERISK;
        case VK_SUBTRACT: return LogiLed::KeyName::NUM_MINUS;
        case VK_TAB: return LogiLed::KeyName::TAB;
        case 'Q': return LogiLed::KeyName::Q;
        case 'W': return LogiLed::KeyName::W;
        case 'E': return LogiLed::KeyName::E;
        case 'R': return LogiLed::KeyName::R;
        case 'T': return LogiLed::KeyName::T;
        case 'Y': 
            return IsQWERTZLayout() ? LogiLed::KeyName::Z : LogiLed::KeyName::Y;
        case 'U': return LogiLed::KeyName::U;
        case 'I': return LogiLed::KeyName::I;
        case 'O': return LogiLed::KeyName::O;
        case 'P': return LogiLed::KeyName::P;
        case VK_OEM_4: return LogiLed::KeyName::OPEN_BRACKET; // [
        case VK_OEM_6: return LogiLed::KeyName::CLOSE_BRACKET; // ]
        case VK_OEM_5: return LogiLed::KeyName::BACKSLASH; // backslash
        case VK_DELETE: return LogiLed::KeyName::KEYBOARD_DELETE;
        case VK_END: return LogiLed::KeyName::END;
        case VK_NEXT: return LogiLed::KeyName::PAGE_DOWN;
        case VK_NUMPAD7: return LogiLed::KeyName::NUM_SEVEN;
        case VK_NUMPAD8: return LogiLed::KeyName::NUM_EIGHT;
        case VK_NUMPAD9: return LogiLed::KeyName::NUM_NINE;
        case VK_ADD: return LogiLed::KeyName::NUM_PLUS;
        case VK_CAPITAL: return LogiLed::KeyName::CAPS_LOCK;
        case 'A': return LogiLed::KeyName::A;
        case 'S': return LogiLed::KeyName::S;
        case 'D': return LogiLed::KeyName::D;
        case 'F': return LogiLed::KeyName::F;
        case 'G': return LogiLed::KeyName::G;
        case 'H': return LogiLed::KeyName::H;
        case 'J': return LogiLed::KeyName::J;
        case 'K': return LogiLed::KeyName::K;
        case 'L': return LogiLed::KeyName::L;
        case VK_OEM_1: return LogiLed::KeyName::SEMICOLON; // ;
        case VK_OEM_7: return LogiLed::KeyName::APOSTROPHE; // '
        case VK_RETURN: 
            // Check if this is numpad enter - numpad enter has extended key flag NOT set
            // Regular enter has extended key flag set
            return (flags & LLKHF_EXTENDED) ? LogiLed::KeyName::ENTER : LogiLed::KeyName::NUM_ENTER;
        case VK_NUMPAD4: return LogiLed::KeyName::NUM_FOUR;
        case VK_NUMPAD5: return LogiLed::KeyName::NUM_FIVE;
        case VK_NUMPAD6: return LogiLed::KeyName::NUM_SIX;
        case VK_LSHIFT: return LogiLed::KeyName::LEFT_SHIFT;
        case 'Z': 
            return IsQWERTZLayout() ? LogiLed::KeyName::Y : LogiLed::KeyName::Z;
        case 'X': return LogiLed::KeyName::X;
        case 'C': return LogiLed::KeyName::C;
        case 'V': return LogiLed::KeyName::V;
        case 'B': return LogiLed::KeyName::B;
        case 'N': return LogiLed::KeyName::N;
        case 'M': return LogiLed::KeyName::M;
        case VK_OEM_COMMA: return LogiLed::KeyName::COMMA;
        case VK_OEM_PERIOD: return LogiLed::KeyName::PERIOD;
        case VK_OEM_2: return LogiLed::KeyName::FORWARD_SLASH; // /
        case VK_RSHIFT: return LogiLed::KeyName::RIGHT_SHIFT;
        case VK_UP: return LogiLed::KeyName::ARROW_UP;
        case VK_NUMPAD1: return LogiLed::KeyName::NUM_ONE;
        case VK_NUMPAD2: return LogiLed::KeyName::NUM_TWO;
        case VK_NUMPAD3: return LogiLed::KeyName::NUM_THREE;
        case VK_LCONTROL: return LogiLed::KeyName::LEFT_CONTROL;
        case VK_LWIN: return LogiLed::KeyName::LEFT_WINDOWS;
        case VK_LMENU: return LogiLed::KeyName::LEFT_ALT;
        case VK_SPACE: return LogiLed::KeyName::SPACE;
        case VK_RMENU: return LogiLed::KeyName::RIGHT_ALT;
        case VK_RWIN: return LogiLed::KeyName::RIGHT_WINDOWS;
        case VK_APPS: return LogiLed::KeyName::APPLICATION_SELECT;
        case VK_RCONTROL: return LogiLed::KeyName::RIGHT_CONTROL;
        case VK_LEFT: return LogiLed::KeyName::ARROW_LEFT;
        case VK_DOWN: return LogiLed::KeyName::ARROW_DOWN;
        case VK_RIGHT: return LogiLed::KeyName::ARROW_RIGHT;
        case VK_NUMPAD0: return LogiLed::KeyName::NUM_ZERO;
        case VK_DECIMAL: return LogiLed::KeyName::NUM_PERIOD;
        default: return LogiLed::KeyName::ESC; // Return ESC for unknown keys
    }
}

// Overload for backward compatibility
LogiLed::KeyName VirtualKeyToLogiLedKey(DWORD vkCode) {
    return VirtualKeyToLogiLedKey(vkCode, 0);
}

// Convert LogiLed::KeyName to display name (used for both UI display and config storage)
std::wstring LogiLedKeyToDisplayName(LogiLed::KeyName key) {
    switch (key) {
        case LogiLed::KeyName::ESC: return L"ESC";
        case LogiLed::KeyName::F1: return L"F1";
        case LogiLed::KeyName::F2: return L"F2";
        case LogiLed::KeyName::F3: return L"F3";
        case LogiLed::KeyName::F4: return L"F4";
        case LogiLed::KeyName::F5: return L"F5";
        case LogiLed::KeyName::F6: return L"F6";
        case LogiLed::KeyName::F7: return L"F7";
        case LogiLed::KeyName::F8: return L"F8";
        case LogiLed::KeyName::F9: return L"F9";
        case LogiLed::KeyName::F10: return L"F10";
        case LogiLed::KeyName::F11: return L"F11";
        case LogiLed::KeyName::F12: return L"F12";
        case LogiLed::KeyName::PRINT_SCREEN: return L"PRINT";
        case LogiLed::KeyName::SCROLL_LOCK: return L"SCROLL";
        case LogiLed::KeyName::PAUSE_BREAK: return L"PAUSE";
        case LogiLed::KeyName::TILDE: return L"~";
        case LogiLed::KeyName::ONE: return L"1";
        case LogiLed::KeyName::TWO: return L"2";
        case LogiLed::KeyName::THREE: return L"3";
        case LogiLed::KeyName::FOUR: return L"4";
        case LogiLed::KeyName::FIVE: return L"5";
        case LogiLed::KeyName::SIX: return L"6";
        case LogiLed::KeyName::SEVEN: return L"7";
        case LogiLed::KeyName::EIGHT: return L"8";
        case LogiLed::KeyName::NINE: return L"9";
        case LogiLed::KeyName::ZERO: return L"0";
        case LogiLed::KeyName::MINUS: return L"-";
        case LogiLed::KeyName::EQUALS: return L"=";
        case LogiLed::KeyName::BACKSPACE: return L"BACKSPACE";
        case LogiLed::KeyName::INSERT: return L"INSERT";
        case LogiLed::KeyName::HOME: return L"HOME";
        case LogiLed::KeyName::PAGE_UP: return L"PGUP";
        case LogiLed::KeyName::NUM_LOCK: return L"NUMLOCK";
        case LogiLed::KeyName::NUM_SLASH: return L"NUM/";
        case LogiLed::KeyName::NUM_ASTERISK: return L"NUM*";
        case LogiLed::KeyName::NUM_MINUS: return L"NUM-";
        case LogiLed::KeyName::TAB: return L"TAB";
        case LogiLed::KeyName::Q: return L"Q";
        case LogiLed::KeyName::W: return L"W";
        case LogiLed::KeyName::E: return L"E";
        case LogiLed::KeyName::R: return L"R";
        case LogiLed::KeyName::T: return L"T";
        case LogiLed::KeyName::Y: return L"Y";
        case LogiLed::KeyName::U: return L"U";
        case LogiLed::KeyName::I: return L"I";
        case LogiLed::KeyName::O: return L"O";
        case LogiLed::KeyName::P: return L"P";
        case LogiLed::KeyName::OPEN_BRACKET: return L"[";
        case LogiLed::KeyName::CLOSE_BRACKET: return L"]";
        case LogiLed::KeyName::BACKSLASH: return L"\\";
        case LogiLed::KeyName::KEYBOARD_DELETE: return L"DELETE";
        case LogiLed::KeyName::END: return L"END";
        case LogiLed::KeyName::PAGE_DOWN: return L"PGDN";
        case LogiLed::KeyName::NUM_SEVEN: return L"NUM7";
        case LogiLed::KeyName::NUM_EIGHT: return L"NUM8";
        case LogiLed::KeyName::NUM_NINE: return L"NUM9";
        case LogiLed::KeyName::NUM_PLUS: return L"NUM+";
        case LogiLed::KeyName::CAPS_LOCK: return L"CAPS";
        case LogiLed::KeyName::A: return L"A";
        case LogiLed::KeyName::S: return L"S";
        case LogiLed::KeyName::D: return L"D";
        case LogiLed::KeyName::F: return L"F";
        case LogiLed::KeyName::G: return L"G";
        case LogiLed::KeyName::H: return L"H";
        case LogiLed::KeyName::J: return L"J";
        case LogiLed::KeyName::K: return L"K";
        case LogiLed::KeyName::L: return L"L";
        case LogiLed::KeyName::SEMICOLON: return L";";
        case LogiLed::KeyName::APOSTROPHE: return L"'";
        case LogiLed::KeyName::ENTER: return L"ENTER";
        case LogiLed::KeyName::NUM_FOUR: return L"NUM4";
        case LogiLed::KeyName::NUM_FIVE: return L"NUM5";
        case LogiLed::KeyName::NUM_SIX: return L"NUM6";
        case LogiLed::KeyName::LEFT_SHIFT: return L"LSHIFT";
        case LogiLed::KeyName::Z: return L"Z";
        case LogiLed::KeyName::X: return L"X";
        case LogiLed::KeyName::C: return L"C";
        case LogiLed::KeyName::V: return L"V";
        case LogiLed::KeyName::B: return L"B";
        case LogiLed::KeyName::N: return L"N";
        case LogiLed::KeyName::M: return L"M";
        case LogiLed::KeyName::COMMA: return L",";
        case LogiLed::KeyName::PERIOD: return L".";
        case LogiLed::KeyName::FORWARD_SLASH: return L"/";
        case LogiLed::KeyName::RIGHT_SHIFT: return L"RSHIFT";
        case LogiLed::KeyName::ARROW_UP: return L"UP";
        case LogiLed::KeyName::NUM_ONE: return L"NUM1";
        case LogiLed::KeyName::NUM_TWO: return L"NUM2";
        case LogiLed::KeyName::NUM_THREE: return L"NUM3";
        case LogiLed::KeyName::NUM_ENTER: return L"NUMENTER";
        case LogiLed::KeyName::LEFT_CONTROL: return L"LCTRL";
        case LogiLed::KeyName::LEFT_WINDOWS: return L"LWIN";
        case LogiLed::KeyName::LEFT_ALT: return L"LALT";
        case LogiLed::KeyName::SPACE: return L"SPACE";
        case LogiLed::KeyName::RIGHT_ALT: return L"RALT";
        case LogiLed::KeyName::RIGHT_WINDOWS: return L"RWIN";
        case LogiLed::KeyName::APPLICATION_SELECT: return L"MENU";
        case LogiLed::KeyName::RIGHT_CONTROL: return L"RCTRL";
        case LogiLed::KeyName::ARROW_LEFT: return L"LEFT";
        case LogiLed::KeyName::ARROW_DOWN: return L"DOWN";
        case LogiLed::KeyName::ARROW_RIGHT: return L"RIGHT";
        case LogiLed::KeyName::NUM_ZERO: return L"NUM0";
        case LogiLed::KeyName::NUM_PERIOD: return L"NUM.";
        case LogiLed::KeyName::G_1: return L"G1";
        case LogiLed::KeyName::G_2: return L"G2";
        case LogiLed::KeyName::G_3: return L"G3";
        case LogiLed::KeyName::G_4: return L"G4";
        case LogiLed::KeyName::G_5: return L"G5";
        case LogiLed::KeyName::G_6: return L"G6";
        case LogiLed::KeyName::G_7: return L"G7";
        case LogiLed::KeyName::G_8: return L"G8";
        case LogiLed::KeyName::G_9: return L"G9";
        case LogiLed::KeyName::G_LOGO: return L"G_LOGO";
        case LogiLed::KeyName::G_BADGE: return L"G_BADGE";
        default: return L"UNKNOWN";
    }
}

// Convert display name to LogiLed::KeyName (for config import/export)
LogiLed::KeyName DisplayNameToLogiLedKey(const std::wstring& displayName) {
    if (displayName == L"ESC") return LogiLed::KeyName::ESC;
    if (displayName == L"F1") return LogiLed::KeyName::F1;
    if (displayName == L"F2") return LogiLed::KeyName::F2;
    if (displayName == L"F3") return LogiLed::KeyName::F3;
    if (displayName == L"F4") return LogiLed::KeyName::F4;
    if (displayName == L"F5") return LogiLed::KeyName::F5;
    if (displayName == L"F6") return LogiLed::KeyName::F6;
    if (displayName == L"F7") return LogiLed::KeyName::F7;
    if (displayName == L"F8") return LogiLed::KeyName::F8;
    if (displayName == L"F9") return LogiLed::KeyName::F9;
    if (displayName == L"F10") return LogiLed::KeyName::F10;
    if (displayName == L"F11") return LogiLed::KeyName::F11;
    if (displayName == L"F12") return LogiLed::KeyName::F12;
    if (displayName == L"PRINT") return LogiLed::KeyName::PRINT_SCREEN;
    if (displayName == L"SCROLL") return LogiLed::KeyName::SCROLL_LOCK;
    if (displayName == L"PAUSE") return LogiLed::KeyName::PAUSE_BREAK;
    if (displayName == L"~") return LogiLed::KeyName::TILDE;
    if (displayName == L"1") return LogiLed::KeyName::ONE;
    if (displayName == L"2") return LogiLed::KeyName::TWO;
    if (displayName == L"3") return LogiLed::KeyName::THREE;
    if (displayName == L"4") return LogiLed::KeyName::FOUR;
    if (displayName == L"5") return LogiLed::KeyName::FIVE;
    if (displayName == L"6") return LogiLed::KeyName::SIX;
    if (displayName == L"7") return LogiLed::KeyName::SEVEN;
    if (displayName == L"8") return LogiLed::KeyName::EIGHT;
    if (displayName == L"9") return LogiLed::KeyName::NINE;
    if (displayName == L"0") return LogiLed::KeyName::ZERO;
    if (displayName == L"-") return LogiLed::KeyName::MINUS;
    if (displayName == L"=") return LogiLed::KeyName::EQUALS;
    if (displayName == L"BACKSPACE") return LogiLed::KeyName::BACKSPACE;
    if (displayName == L"INSERT") return LogiLed::KeyName::INSERT;
    if (displayName == L"HOME") return LogiLed::KeyName::HOME;
    if (displayName == L"PGUP") return LogiLed::KeyName::PAGE_UP;
    if (displayName == L"NUMLOCK") return LogiLed::KeyName::NUM_LOCK;
    if (displayName == L"NUM/") return LogiLed::KeyName::NUM_SLASH;
    if (displayName == L"NUM*") return LogiLed::KeyName::NUM_ASTERISK;
    if (displayName == L"NUM-") return LogiLed::KeyName::NUM_MINUS;
    if (displayName == L"TAB") return LogiLed::KeyName::TAB;
    if (displayName == L"Q") return LogiLed::KeyName::Q;
    if (displayName == L"W") return LogiLed::KeyName::W;
    if (displayName == L"E") return LogiLed::KeyName::E;
    if (displayName == L"R") return LogiLed::KeyName::R;
    if (displayName == L"T") return LogiLed::KeyName::T;
    if (displayName == L"Y") return LogiLed::KeyName::Y;
    if (displayName == L"U") return LogiLed::KeyName::U;
    if (displayName == L"I") return LogiLed::KeyName::I;
    if (displayName == L"O") return LogiLed::KeyName::O;
    if (displayName == L"P") return LogiLed::KeyName::P;
    if (displayName == L"[") return LogiLed::KeyName::OPEN_BRACKET;
    if (displayName == L"]") return LogiLed::KeyName::CLOSE_BRACKET;
    if (displayName == L"\\") return LogiLed::KeyName::BACKSLASH;
    if (displayName == L"DELETE") return LogiLed::KeyName::KEYBOARD_DELETE;
    if (displayName == L"END") return LogiLed::KeyName::END;
    if (displayName == L"PGDN") return LogiLed::KeyName::PAGE_DOWN;
    if (displayName == L"NUM7") return LogiLed::KeyName::NUM_SEVEN;
    if (displayName == L"NUM8") return LogiLed::KeyName::NUM_EIGHT;
    if (displayName == L"NUM9") return LogiLed::KeyName::NUM_NINE;
    if (displayName == L"NUM+") return LogiLed::KeyName::NUM_PLUS;
    if (displayName == L"CAPS") return LogiLed::KeyName::CAPS_LOCK;
    if (displayName == L"A") return LogiLed::KeyName::A;
    if (displayName == L"S") return LogiLed::KeyName::S;
    if (displayName == L"D") return LogiLed::KeyName::D;
    if (displayName == L"F") return LogiLed::KeyName::F;
    if (displayName == L"G") return LogiLed::KeyName::G;
    if (displayName == L"H") return LogiLed::KeyName::H;
    if (displayName == L"J") return LogiLed::KeyName::J;
    if (displayName == L"K") return LogiLed::KeyName::K;
    if (displayName == L"L") return LogiLed::KeyName::L;
    if (displayName == L";") return LogiLed::KeyName::SEMICOLON;
    if (displayName == L"'") return LogiLed::KeyName::APOSTROPHE;
    if (displayName == L"ENTER") return LogiLed::KeyName::ENTER;
    if (displayName == L"NUM4") return LogiLed::KeyName::NUM_FOUR;
    if (displayName == L"NUM5") return LogiLed::KeyName::NUM_FIVE;
    if (displayName == L"NUM6") return LogiLed::KeyName::NUM_SIX;
    if (displayName == L"LSHIFT") return LogiLed::KeyName::LEFT_SHIFT;
    if (displayName == L"Z") return LogiLed::KeyName::Z;
    if (displayName == L"X") return LogiLed::KeyName::X;
    if (displayName == L"C") return LogiLed::KeyName::C;
    if (displayName == L"V") return LogiLed::KeyName::V;
    if (displayName == L"B") return LogiLed::KeyName::B;
    if (displayName == L"N") return LogiLed::KeyName::N;
    if (displayName == L"M") return LogiLed::KeyName::M;
    if (displayName == L",") return LogiLed::KeyName::COMMA;
    if (displayName == L".") return LogiLed::KeyName::PERIOD;
    if (displayName == L"/") return LogiLed::KeyName::FORWARD_SLASH;
    if (displayName == L"RSHIFT") return LogiLed::KeyName::RIGHT_SHIFT;
    if (displayName == L"UP") return LogiLed::KeyName::ARROW_UP;
    if (displayName == L"NUM1") return LogiLed::KeyName::NUM_ONE;
    if (displayName == L"NUM2") return LogiLed::KeyName::NUM_TWO;
    if (displayName == L"NUM3") return LogiLed::KeyName::NUM_THREE;
    if (displayName == L"NUMENTER") return LogiLed::KeyName::NUM_ENTER;
    if (displayName == L"LCTRL") return LogiLed::KeyName::LEFT_CONTROL;
    if (displayName == L"LWIN") return LogiLed::KeyName::LEFT_WINDOWS;
    if (displayName == L"LALT") return LogiLed::KeyName::LEFT_ALT;
    if (displayName == L"SPACE") return LogiLed::KeyName::SPACE;
    if (displayName == L"RALT") return LogiLed::KeyName::RIGHT_ALT;
    if (displayName == L"RWIN") return LogiLed::KeyName::RIGHT_WINDOWS;
    if (displayName == L"MENU") return LogiLed::KeyName::APPLICATION_SELECT;
    if (displayName == L"RCTRL") return LogiLed::KeyName::RIGHT_CONTROL;
    if (displayName == L"LEFT") return LogiLed::KeyName::ARROW_LEFT;
    if (displayName == L"DOWN") return LogiLed::KeyName::ARROW_DOWN;
    if (displayName == L"RIGHT") return LogiLed::KeyName::ARROW_RIGHT;
    if (displayName == L"NUM0") return LogiLed::KeyName::NUM_ZERO;
    if (displayName == L"NUM.") return LogiLed::KeyName::NUM_PERIOD;
    if (displayName == L"G1") return LogiLed::KeyName::G_1;
    if (displayName == L"G2") return LogiLed::KeyName::G_2;
    if (displayName == L"G3") return LogiLed::KeyName::G_3;
    if (displayName == L"G4") return LogiLed::KeyName::G_4;
    if (displayName == L"G5") return LogiLed::KeyName::G_5;
    if (displayName == L"G6") return LogiLed::KeyName::G_6;
    if (displayName == L"G7") return LogiLed::KeyName::G_7;
    if (displayName == L"G8") return LogiLed::KeyName::G_8;
    if (displayName == L"G9") return LogiLed::KeyName::G_9;
    
    // Use a special invalid key value instead of ESC to indicate unknown keys
    // TODO: Define INVALID_KEY in LogiLed enum or use std::optional
    return LogiLed::KeyName::ESC; // Temporary fallback - consider using std::optional<LogiLed::KeyName> instead
}

// Format highlight keys for display in text field
std::wstring FormatHighlightKeysForDisplay(const std::vector<LogiLed::KeyName>& keys) {
    if (keys.empty()) {
        return L"";
    }
    
    // sort keys for consistent display
    std::vector<LogiLed::KeyName> sortedKeys = keys;
    std::sort(sortedKeys.begin(), sortedKeys.end(), [](LogiLed::KeyName a, LogiLed::KeyName b) {
        return static_cast<int>(a) < static_cast<int>(b);
    });

    std::wstring result;
    for (size_t i = 0; i < sortedKeys.size(); ++i) {
        if (i > 0) {
            result += L" - ";
        }
        result += LogiLedKeyToDisplayName(sortedKeys[i]);
    }
    return result;
}