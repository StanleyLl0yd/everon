#include "HotkeyManager.h"
#include "Utils.h"
#include <array>

namespace Everon {

HotkeyManager::HotkeyManager(HWND window)
    : m_window(window) {
}

HotkeyManager::~HotkeyManager() {
    UnregisterHotkey();
}

bool HotkeyManager::RegisterHotkey(const HotkeyConfig& config, HotkeyCallback callback) {
    UnregisterHotkey();

    if (!config.enabled || !config.IsValid()) {
        return true;
    }

    m_config = config;
    m_callback = std::move(callback);

    UINT mods = m_config.modifiers;
#ifdef MOD_NOREPEAT
    mods |= MOD_NOREPEAT;
#endif

    if (::RegisterHotKey(m_window, HOTKEY_ID_TOGGLE, mods, m_config.virtualKey)) {
        m_isRegistered = true;
        Utils::DebugLog(L"[Everon] Hotkey registered: %s\n",
                        HotkeyToString(m_config).c_str());
        return true;
    }

    Utils::DebugLog(L"[Everon] Failed to register hotkey: %lu\n", GetLastError());
    m_isRegistered = false;
    return false;
}

void HotkeyManager::UnregisterHotkey() {
    if (!m_isRegistered) {
        return;
    }

    if (!::UnregisterHotKey(m_window, HOTKEY_ID_TOGGLE)) {
        Utils::DebugLog(L"[Everon] Failed to unregister hotkey: %lu\n", GetLastError());
    }
    m_isRegistered = false;
    Utils::DebugLog(L"[Everon] Hotkey unregistered\n");
}

bool HotkeyManager::HandleHotkey(WPARAM wParam) const {
    if (wParam == HOTKEY_ID_TOGGLE && m_isRegistered && m_callback) {
        m_callback();
        return true;
    }
    return false;
}

std::wstring HotkeyManager::HotkeyToString(const HotkeyConfig& config) {
    if (!config.IsValid()) {
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
    unsigned int modifiers = 0;
    if (unsigned int vk = 0; swscanf_s(str, L"%d,%u,%u", &enabled, &modifiers, &vk) == 3) {
        config.enabled = (enabled != 0);
        config.modifiers = static_cast<UINT>(modifiers);
        config.virtualKey = static_cast<UINT>(vk);
    }

    return config;
}

std::wstring HotkeyManager::HotkeyToRegistryString(const HotkeyConfig& config) {
    std::array<wchar_t, 64> buffer{};
    swprintf_s(buffer.data(), buffer.size(), L"%d,%u,%u",
               config.enabled ? 1 : 0,
               config.modifiers,
               config.virtualKey);
    return buffer.data();
}

}
