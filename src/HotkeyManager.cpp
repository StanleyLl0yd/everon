#include "HotkeyManager.h"
#include "Utils.h"
#include <cwchar>

namespace Everon {

HotkeyManager::~HotkeyManager() {
    Unregister();
}

bool HotkeyManager::Register(HWND hwnd, const HotkeyConfig& config) {
    Unregister();

    if (!config.enabled || config.virtualKey == 0) {
        return true;
    }

    if (!IsValidConfig(config)) {
        return false;
    }

    if (RegisterHotKey(hwnd, HOTKEY_ID, config.modifiers, config.virtualKey)) {
        m_hwnd = hwnd;
        m_isRegistered = true;
        return true;
    }

    return false;
}

void HotkeyManager::Unregister() {
    if (m_isRegistered && m_hwnd) {
        UnregisterHotKey(m_hwnd, HOTKEY_ID);
        m_isRegistered = false;
        m_hwnd = nullptr;
    }
}

bool HotkeyManager::IsValidConfig(const HotkeyConfig& config) {
    if (!config.enabled) {
        return true;
    }

    if (config.virtualKey == 0 || config.virtualKey > 0xFF) {
        return false;
    }

    constexpr UINT allowedModifiers = MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_NOREPEAT;
    if ((config.modifiers & ~allowedModifiers) != 0) {
        return false;
    }

    return true;
}

std::wstring HotkeyManager::HotkeyToString(const HotkeyConfig& config) {
    if (!config.enabled || config.virtualKey == 0) {
        return L"None";
    }

    std::wstring result;

    if (config.modifiers & MOD_CONTROL) {
        result += L"Ctrl+";
    }
    if (config.modifiers & MOD_ALT) {
        result += L"Alt+";
    }
    if (config.modifiers & MOD_SHIFT) {
        result += L"Shift+";
    }
    if (config.modifiers & MOD_WIN) {
        result += L"Win+";
    }

    result += Utils::GetKeyName(config.virtualKey);

    return result;
}

HotkeyConfig HotkeyManager::StringToHotkey(const wchar_t* str) {
    HotkeyConfig config;

    if (!str || *str == L'\0') {
        return config;
    }

    int enabled = 0;
    unsigned int modifiers = 0, vk = 0;
    if (swscanf_s(str, L"%d,%u,%u", &enabled, &modifiers, &vk) == 3) {
        config.enabled = (enabled != 0);
        config.modifiers = static_cast<UINT>(modifiers);
        config.virtualKey = static_cast<UINT>(vk);
    }

    return config;
}

std::wstring HotkeyManager::HotkeyToRegistryString(const HotkeyConfig& config) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"%d,%u,%u",
              config.enabled ? 1 : 0,
              config.modifiers,
              config.virtualKey);
    return buffer;
}

} 
