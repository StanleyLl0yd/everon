#pragma once

#include <windows.h>
#include <memory>
#include "Settings.h"
#include "PowerManager.h"

namespace Everon {

class TrayIcon;
class SettingsDialog;
class HotkeyManager;

class App {
public:
    explicit App(HINSTANCE instance);
    ~App();
    int Run();

    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"EveronMainWindow";
    static constexpr UINT WM_SHOW_SETTINGS = WM_APP + 2;

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message,
                                      WPARAM wParam, LPARAM lParam);
    void OnCreate();
    void OnDestroy();
    void OnTimer(UINT_PTR timerId);
    void OnTrayIcon(LPARAM lParam);
    void OnHotkey(WPARAM wParam);
    void OnTaskbarCreated();
    void ToggleEnabled();
    void ShowSettings();
    void ShowAbout();
    void Exit();
    void StartTimer();
    void StopTimer();
    void ArmExpireTimer(const TimerConfig& timer);
    void UpdatePowerState();
    void RegisterHotkey();
    bool SaveSettings();

    HINSTANCE m_instance = nullptr;
    HWND m_window = nullptr;
    Settings m_settings;
    PowerManager m_powerManager;
    std::unique_ptr<TrayIcon> m_trayIcon;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    std::unique_ptr<HotkeyManager> m_hotkeyManager;
    bool m_isSettingsDialogOpen = false;

    UINT m_taskbarCreatedMessage = 0;

    static constexpr UINT_PTR TIMER_ID_KEYPRESS = 1;
    static constexpr UINT_PTR TIMER_ID_EXPIRE = 2;
};

} // namespace Everon
