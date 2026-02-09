#pragma once

#include <windows.h>
#include <string>

#include "HotkeyManager.h"
#include "TimerMode.h"

namespace Everon {

// Forward declaration must match the underlying type used in Localization.h
enum class Language : unsigned char;

// Application settings stored in registry
class Settings {
public:
    static constexpr DWORD MIN_PERIOD_SEC = 1;
    static constexpr DWORD MAX_PERIOD_SEC = 86400; // 24 hours
    static constexpr DWORD DEFAULT_PERIOD_SEC = 59;

    Settings();

    // Getters
    DWORD GetPeriodSec() const noexcept { return m_periodSec; }
    WORD GetVirtualKey() const noexcept { return m_vkKey; }
    bool GetKeepDisplayOn() const noexcept { return m_keepDisplayOn; }
    bool GetShowToggleNotifications() const noexcept { return m_showToggleNotifications; }
    bool GetAutoStart() const noexcept { return m_autoStart; }
    bool IsEnabled() const noexcept { return m_enabled; }
    Language GetLanguage() const noexcept;
    HotkeyConfig GetHotkeyConfig() const noexcept;
    TimerConfig GetTimerConfig() const noexcept;

    // Setters
    void SetPeriodSec(DWORD value) noexcept;
    void SetVirtualKey(WORD value) noexcept;
    void SetKeepDisplayOn(bool value) noexcept;
    void SetShowToggleNotifications(bool value) noexcept;
    void SetAutoStart(bool value) noexcept { m_autoStart = value; }
    void SetEnabled(bool value) noexcept;
    void SetLanguage(Language value) noexcept;
    void SetHotkeyConfig(const HotkeyConfig& value) noexcept;
    void SetTimerConfig(const TimerConfig& value) noexcept;

    // Registry operations
    bool LoadFromRegistry();
    bool SaveToRegistry();
    bool IsDirty() const noexcept { return m_dirty; }
    void SetDirty(bool value) noexcept { m_dirty = value; }

    // Validation
    bool IsValidPeriod(DWORD value) const noexcept;
    bool IsValidVirtualKey(WORD vk) const noexcept;

    // Auto-start registry management
    static bool IsAutoStartEnabled();
    static bool SetAutoStartEnabled(bool enable);

private:
    DWORD m_periodSec = DEFAULT_PERIOD_SEC;
    WORD m_vkKey = 0;
    bool m_keepDisplayOn = false;
    bool m_showToggleNotifications = false;
    bool m_autoStart = false;
    bool m_enabled = true;
    HotkeyConfig m_hotkeyConfig = {};
    TimerConfig m_timerConfig = {};
    bool m_dirty = true;

    static constexpr const wchar_t* REG_KEY_PATH = L"Software\\Everon";
    static constexpr const wchar_t* RUN_KEY_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    static constexpr const wchar_t* APP_NAME = L"Everon";
};

} // namespace Everon
