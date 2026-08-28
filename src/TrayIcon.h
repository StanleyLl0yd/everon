#pragma once

#include <windows.h>
#include <shellapi.h>
#include <functional>
#include <string>

namespace Everon {

class Settings;

class TrayIcon {
public:
    using MenuCallback = std::function<void()>;
    using DurationCallback = std::function<void(DWORD)>;

    explicit TrayIcon(HWND parentWindow, HINSTANCE instance);
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;
    TrayIcon(TrayIcon&&) = delete;
    TrayIcon& operator=(TrayIcon&&) = delete;

    bool Add();
    bool ReAdd();
    void Remove();

    void UpdateTooltip(const Settings& settings,
                       bool pausedByBatterySaver = false,
                       bool displayKeepAwakeActive = false);
    void ShowNotification(const wchar_t* title, const wchar_t* message, DWORD flags);
    void SetEnabled(bool enabled);
    void HandleMessage(LPARAM lParam);

    void SetToggleCallback(MenuCallback callback) { m_onToggle = std::move(callback); }
    void SetSettingsCallback(MenuCallback callback) { m_onSettings = std::move(callback); }
    void SetAboutCallback(MenuCallback callback) { m_onAbout = std::move(callback); }
    void SetExitCallback(MenuCallback callback) { m_onExit = std::move(callback); }
    void SetDurationCallback(DurationCallback callback) { m_onDuration = std::move(callback); }
    void SetCustomDurationCallback(MenuCallback callback) { m_onCustomDuration = std::move(callback); }
    void SetUntilCallback(MenuCallback callback) { m_onUntil = std::move(callback); }

    static constexpr UINT WM_TRAYICON = WM_APP + 1;

private:
    void ShowContextMenu();
    void UpdateIcon();

    HWND m_parentWindow = nullptr;
    HINSTANCE m_instance = nullptr;
    NOTIFYICONDATAW m_notifyData = {};
    HICON m_activeIcon = nullptr;
    HICON m_disabledIcon = nullptr;
    bool m_activeIconOwned = false;

    MenuCallback m_onToggle;
    MenuCallback m_onSettings;
    MenuCallback m_onAbout;
    MenuCallback m_onExit;
    MenuCallback m_onCustomDuration;
    MenuCallback m_onUntil;
    DurationCallback m_onDuration;

    std::wstring m_statusText = L"Everon";
    DWORD m_activeDurationMinutes = 0;
    bool m_isEnabled = true;
};

}
