#pragma once

#include <windows.h>
#include <shellapi.h>
#include <functional>

namespace Everon {

class Settings;

// Manages system tray icon and notifications
class TrayIcon {
public:
    using MenuCallback = std::function<void()>;

    explicit TrayIcon(HWND parentWindow, HINSTANCE instance);
    ~TrayIcon();

    // Add/remove tray icon
    bool Add();
    bool ReAdd();
    void Remove();

    // Update tooltip text
    void UpdateTooltip(const Settings& settings);

    // Show notification
    void ShowNotification(const wchar_t* title, const wchar_t* message, DWORD flags);

    // Update enabled state for menu
    void SetEnabled(bool enabled) { m_isEnabled = enabled; }

    // Handle tray messages
    void HandleMessage(LPARAM lParam);

    // Set menu callbacks
    void SetToggleCallback(MenuCallback callback) { m_onToggle = std::move(callback); }
    void SetSettingsCallback(MenuCallback callback) { m_onSettings = std::move(callback); }
    void SetAboutCallback(MenuCallback callback) { m_onAbout = std::move(callback); }
    void SetExitCallback(MenuCallback callback) { m_onExit = std::move(callback); }

    // Tray icon message ID
    static constexpr UINT WM_TRAYICON = WM_APP + 1;

private:
    void ShowContextMenu();

    HWND m_parentWindow = nullptr;
    HINSTANCE m_instance = nullptr;
    NOTIFYICONDATAW m_notifyData = {};
    bool m_iconOwned = false;

    MenuCallback m_onToggle;
    MenuCallback m_onSettings;
    MenuCallback m_onAbout;
    MenuCallback m_onExit;

    bool m_isEnabled = true;
};

} // namespace Everon
