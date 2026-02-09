#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace Everon {

// Hotkey configuration
struct HotkeyConfig {
    bool enabled = false;
    UINT modifiers = 0;  // MOD_CONTROL, MOD_SHIFT, MOD_ALT, MOD_WIN
    UINT virtualKey = 0; // VK_*

    bool IsValid() const {
        return virtualKey != 0;
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

// Hotkey manager
class HotkeyManager {
public:
    using HotkeyCallback = std::function<void()>;

    explicit HotkeyManager(HWND window);
    ~HotkeyManager();

    // Register/unregister hotkey
    bool RegisterHotkey(const HotkeyConfig& config, HotkeyCallback callback);
    void UnregisterHotkey();

    // Check if hotkey is registered
    bool IsRegistered() const { return m_isRegistered; }

    // Get current configuration
    const HotkeyConfig& GetConfig() const { return m_config; }

    // Handle WM_HOTKEY message
    bool HandleHotkey(WPARAM wParam);

    // Convert hotkey to string for display
    static std::wstring HotkeyToString(const HotkeyConfig& config);

    // Parse hotkey from string (for registry)
    static HotkeyConfig StringToHotkey(const wchar_t* str);
    static std::wstring HotkeyToRegistryString(const HotkeyConfig& config);

    // Hotkey ID
    static constexpr int HOTKEY_ID_TOGGLE = 1;

private:
    HWND m_window = nullptr;
    bool m_isRegistered = false;
    HotkeyConfig m_config;
    HotkeyCallback m_callback;
};

} // namespace Everon
