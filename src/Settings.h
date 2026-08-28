#pragma once

#include <windows.h>
#include <string>

#include "HotkeyManager.h"
#include "TimerMode.h"

namespace Everon {

enum class Language : unsigned char;

class Settings {
public:
    static constexpr DWORD MIN_PERIOD_SEC = 1;
    static constexpr DWORD MAX_PERIOD_SEC = 24 * 60 * 60;
    static constexpr DWORD DEFAULT_PERIOD_SEC = 59;

    Settings();
#ifdef EVERON_TESTING
    explicit Settings(const wchar_t* registryKeyPath);
#endif

    DWORD GetPeriodSec() const noexcept { return m_periodSec; }
    WORD GetVirtualKey() const noexcept { return m_vkKey; }
    bool GetKeepDisplayOn() const noexcept { return m_keepDisplayOn; }
    bool GetRespectBatterySaver() const noexcept { return m_respectBatterySaver; }
    bool GetAllowDisplayOnBattery() const noexcept { return m_allowDisplayOnBattery; }
    bool GetShowToggleNotifications() const noexcept { return m_showToggleNotifications; }
    bool GetAutoStart() const noexcept { return m_autoStart; }
    bool IsEnabled() const noexcept { return m_enabled; }
    Language GetLanguage() const noexcept;
    HotkeyConfig GetHotkeyConfig() const noexcept;
    TimerConfig GetTimerConfig() const noexcept;

    void SetPeriodSec(DWORD value) noexcept;
    void SetVirtualKey(WORD value) noexcept;
    void SetKeepDisplayOn(bool value) noexcept;
    void SetRespectBatterySaver(bool value) noexcept;
    void SetAllowDisplayOnBattery(bool value) noexcept;
    void SetShowToggleNotifications(bool value) noexcept;
    void SetAutoStart(bool value) noexcept { m_autoStart = value; }
    void SetEnabled(bool value) noexcept;
    void SetLanguage(Language value) noexcept;
    void SetHotkeyConfig(const HotkeyConfig& value) noexcept;
    void SetTimerConfig(const TimerConfig& value) noexcept;
    void SetTimerRuntimeDeadline(ULONGLONG value) noexcept { m_timerConfig.monotonicDeadlineMs = value; }

    bool LoadFromRegistry();
    bool SaveToRegistry();
    bool IsDirty() const noexcept { return m_dirty; }
    void SetDirty(bool value) noexcept { m_dirty = value; }

    bool IsValidPeriod(DWORD value) const noexcept;
    bool IsValidVirtualKey(WORD vk) const noexcept;

    static bool IsAutoStartEnabled();
    static bool SetAutoStartEnabled(bool enable);

private:
    DWORD m_periodSec = DEFAULT_PERIOD_SEC;
    WORD m_vkKey = 0;
    bool m_keepDisplayOn = false;
    bool m_respectBatterySaver = false;
    bool m_allowDisplayOnBattery = true;
    bool m_showToggleNotifications = false;
    bool m_autoStart = false;
    bool m_enabled = true;
    HotkeyConfig m_hotkeyConfig = {};
    TimerConfig m_timerConfig = {};
    bool m_dirty = true;
    std::wstring m_registryKeyPath = REG_KEY_PATH;

    static constexpr const wchar_t* REG_KEY_PATH = L"Software\\Everon";
    static constexpr const wchar_t* RUN_KEY_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    static constexpr const wchar_t* APP_NAME = L"Everon";
};

}
