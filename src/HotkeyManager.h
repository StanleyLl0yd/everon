#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace Everon {

struct HotkeyConfig {
    bool enabled = false;
    UINT modifiers = 0;
    UINT virtualKey = 0;

    bool IsValid() const {
        constexpr UINT kAllowedModifiers = MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN;
        return virtualKey > 0 && virtualKey <= 0xFFU &&
               (modifiers & ~kAllowedModifiers) == 0;
    }

    bool operator==(const HotkeyConfig& other) const {
        return enabled == other.enabled &&
               modifiers == other.modifiers &&
               virtualKey == other.virtualKey;
    }

    bool operator!=(const HotkeyConfig& other) const {
        return !(*this == other);
    }
};

class HotkeyManager {
public:
    using HotkeyCallback = std::function<void()>;

    explicit HotkeyManager(HWND window);
    ~HotkeyManager();

    bool RegisterHotkey(const HotkeyConfig& config, HotkeyCallback callback);
    void UnregisterHotkey();

    bool IsRegistered() const { return m_isRegistered; }

    const HotkeyConfig& GetConfig() const { return m_config; }

    bool HandleHotkey(WPARAM wParam);

    static std::wstring HotkeyToString(const HotkeyConfig& config);

    static HotkeyConfig StringToHotkey(const wchar_t* str);
    static std::wstring HotkeyToRegistryString(const HotkeyConfig& config);

    static constexpr int HOTKEY_ID_TOGGLE = 1;

private:
    HWND m_window = nullptr;
    bool m_isRegistered = false;
    HotkeyConfig m_config;
    HotkeyCallback m_callback;
};

}
