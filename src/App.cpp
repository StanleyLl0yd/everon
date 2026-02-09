#include "App.h"
#include "TrayIcon.h"
#include "SettingsDialog.h"
#include "HotkeyManager.h"
#include "Utils.h"
#include "Localization.h"
#include "TimerMode.h"
#include "resource.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

namespace Everon {

App::App(HINSTANCE instance)
    : m_instance(instance)
    , m_settingsDialog(std::make_unique<SettingsDialog>(instance)) {
}

App::~App() {
    if (m_window) {
        DestroyWindow(m_window);
    }
}

bool App::SaveSettings() {
    if (m_settings.SaveToRegistry()) {
        return true;
    }

    Utils::DebugLog(L"[Everon] Failed to save settings to registry\n");
    auto& loc = Localization::Instance();

    if (m_trayIcon) {
        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                     loc.GetString(StringID::ErrorSaveSettings), NIIF_WARNING);
    } else {
        MessageBoxW(m_window,
                   loc.GetString(StringID::ErrorSaveSettings),
                   loc.GetString(StringID::ErrorTitle),
                   MB_OK | MB_ICONWARNING);
    }

    return false;
}

void App::ArmExpireTimer(const TimerConfig& timer) {
    const DWORD remainingMs = timer.GetRemainingMilliseconds();
    if (remainingMs == 0) {
        OnTimer(TIMER_ID_EXPIRE);
        return;
    }

    static constexpr DWORD kWinTimerMaxMs = 0x7FFFFFFFUL;
    static constexpr UINT kLongRearmChunkMs = 10U * 60U * 1000U; // 10 minutes

    UINT intervalMs = static_cast<UINT>(remainingMs);
    if (remainingMs > kWinTimerMaxMs) {
        // Defensive path for corrupted/legacy settings far in the future.
        // Re-arm in chunks instead of passing an out-of-range value to SetTimer.
        intervalMs = kLongRearmChunkMs;
    }

    Utils::SetTimerChecked(m_window, TIMER_ID_EXPIRE, intervalMs ? intervalMs : 1U);
}

int App::Run() {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_DATE_CLASSES;
    InitCommonControlsEx(&icc);

    // Handle Explorer restart (tray icon may disappear)
    m_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_instance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hIcon = LoadIconW(m_instance, MAKEINTRESOURCEW(IDI_EVERON));
    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    if (!RegisterClassW(&wc)) {
        Utils::DebugLog(L"[Everon] Failed to register window class\n");
        return 1;
    }

    m_window = CreateWindowExW(0, WINDOW_CLASS_NAME, L"Everon", WS_OVERLAPPEDWINDOW,
                               0, 0, 0, 0, nullptr, nullptr, m_instance, this);

    if (!m_window) {
        Utils::DebugLog(L"[Everon] Failed to create window\n");
        return 2;
    }

    ShowWindow(m_window, SW_HIDE);

    MSG message = {};
    int gm = 0;
    while ((gm = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (gm == -1) {
        Utils::DebugLog(L"[Everon] GetMessageW failed\n");
        return 3;
    }

    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK App::WindowProc(HWND window, UINT message,
                                WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;

    if (message == WM_CREATE) {
        auto createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->m_window = window;
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (!app) {
        return DefWindowProcW(window, message, wParam, lParam);
    }

    if (app->m_taskbarCreatedMessage != 0 && message == app->m_taskbarCreatedMessage) {
        app->OnTaskbarCreated();
        return 0;
    }

    switch (message) {
        case WM_CREATE:
            app->OnCreate();
            return 0;
        case WM_TIMER:
            app->OnTimer(static_cast<UINT_PTR>(wParam));
            return 0;
        case TrayIcon::WM_TRAYICON:
            app->OnTrayIcon(lParam);
            return 0;
        case WM_HOTKEY:
            app->OnHotkey(wParam);
            return 0;
        case WM_SHOW_SETTINGS:
            app->ShowSettings();
            return 0;
        case WM_DESTROY:
            app->OnDestroy();
            return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

void App::OnCreate() {
    m_settings.LoadFromRegistry();

    m_trayIcon = std::make_unique<TrayIcon>(m_window, m_instance);
    m_trayIcon->SetToggleCallback([this]() { ToggleEnabled(); });
    m_trayIcon->SetSettingsCallback([this]() { ShowSettings(); });
    m_trayIcon->SetAboutCallback([this]() { ShowAbout(); });
    m_trayIcon->SetExitCallback([this]() { Exit(); });

    if (!m_trayIcon->Add()) {
        auto& loc = Localization::Instance();
        MessageBoxW(nullptr,
                   loc.GetString(StringID::ErrorTrayIcon),
                   loc.GetString(StringID::ErrorTitle),
                   MB_OK | MB_ICONWARNING);

        // If we can't create the tray icon, the app becomes effectively invisible and
        // users lose the ability to disable/exit it. Exit to avoid a "headless" run.
        DestroyWindow(m_window);
        return;
    }

    m_trayIcon->UpdateTooltip(m_settings);
    m_trayIcon->SetEnabled(m_settings.IsEnabled());

    m_hotkeyManager = std::make_unique<HotkeyManager>(m_window);
    RegisterHotkey();

    if (m_settings.IsEnabled()) {
        UpdatePowerState();
        StartTimer();
    }
}

void App::OnDestroy() {
    StopTimer();
    m_powerManager.AllowSleep();
    m_hotkeyManager.reset();
    m_trayIcon.reset();
    m_window = nullptr;
    PostQuitMessage(0);
}

void App::OnTimer(UINT_PTR timerId) {
    if (!m_settings.IsEnabled()) {
        return;
    }

    if (timerId == TIMER_ID_KEYPRESS) {
        // Periodic key press (optional)
        const WORD vk = m_settings.GetVirtualKey();
        if (vk != 0) {
            m_powerManager.SendKeyPress(vk);
        }
        return;
    }

    if (timerId == TIMER_ID_EXPIRE) {
        // One-shot: stop this timer immediately
        KillTimer(m_window, TIMER_ID_EXPIRE);

        const TimerConfig timer = m_settings.GetTimerConfig();
        if (timer.IsExpired()) {
            // Timer expired - disable
            m_settings.SetEnabled(false);

            TimerConfig cleared = timer;
            cleared.endTimeUtc = 0;
            cleared.startTime = {};
            m_settings.SetTimerConfig(cleared);

            SaveSettings();
            StopTimer();
            m_powerManager.AllowSleep();
            m_trayIcon->UpdateTooltip(m_settings);
            m_trayIcon->SetEnabled(false);

            auto& loc = Localization::Instance();
            m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                        loc.GetString(StringID::NotifyTimerExpired), NIIF_INFO);
        } else {
            // Re-arm to handle clock adjustments and boundary races robustly.
            ArmExpireTimer(timer);
        }
        return;
    }
}


void App::OnTrayIcon(LPARAM lParam) {
    if (m_trayIcon) {
        m_trayIcon->HandleMessage(lParam);
    }
}

void App::OnHotkey(WPARAM wParam) {
    if (m_hotkeyManager && m_hotkeyManager->HandleHotkey(wParam)) {
        // Hotkey handled
    }
}

void App::ToggleEnabled() {
    const bool newEnabled = !m_settings.IsEnabled();
    m_settings.SetEnabled(newEnabled);

    if (newEnabled) {
        // Initialize timer runtime state on enable
        TimerConfig timer = m_settings.GetTimerConfig();
        if (timer.mode != TimerMode::Indefinite) {
            timer.ResetStartTime();
        } else {
            timer.startTime = {};
            timer.endTimeUtc = 0;
        }
        m_settings.SetTimerConfig(timer);

        UpdatePowerState();
        StartTimer();
    } else {
        StopTimer();
        m_powerManager.AllowSleep();

        // Clear runtime state to avoid stale expirations
        TimerConfig timer = m_settings.GetTimerConfig();
        timer.startTime = {};
        timer.endTimeUtc = 0;
        m_settings.SetTimerConfig(timer);
    }

    SaveSettings();
    if (m_trayIcon) {
        m_trayIcon->UpdateTooltip(m_settings);
        m_trayIcon->SetEnabled(m_settings.IsEnabled());

        if (m_settings.GetShowToggleNotifications()) {
            auto& loc = Localization::Instance();
            m_trayIcon->ShowNotification(
                loc.GetString(StringID::ErrorTitle),
                m_settings.IsEnabled() ? loc.GetString(StringID::NotifyEnabled)
                                       : loc.GetString(StringID::NotifyDisabled),
                NIIF_INFO);
        }
    }
}


void App::ShowSettings() {
    if (!m_settingsDialog || m_isSettingsDialogOpen) {
        return;
    }

    m_isSettingsDialogOpen = true;
    const bool accepted = m_settingsDialog->Show(m_window, m_settings);
    m_isSettingsDialogOpen = false;

    if (accepted) {
        // If timer configuration was changed (dialog clears runtime state), re-initialize it if enabled.
        if (m_settings.IsEnabled()) {
            TimerConfig timer = m_settings.GetTimerConfig();
            if (timer.mode != TimerMode::Indefinite && timer.endTimeUtc == 0) {
                timer.ResetStartTime();
                m_settings.SetTimerConfig(timer);
                SaveSettings();
            }

            UpdatePowerState();
            StartTimer();
        }

        if (m_trayIcon) {
            m_trayIcon->UpdateTooltip(m_settings);
            m_trayIcon->SetEnabled(m_settings.IsEnabled());
        }
        RegisterHotkey();
    }
}


void App::ShowAbout() {
    auto& loc = Localization::Instance();
    wchar_t message[1024];
    swprintf_s(message, _countof(message),
              L"%s\n%s\n\n%s\n- %s\n- %s\n- %s\n- %s\n\n%s\n%s",
              loc.GetString(StringID::AboutVersion),
              loc.GetString(StringID::AboutTagline),
              loc.GetString(StringID::AboutPerfectFor),
              loc.GetString(StringID::AboutDownloads),
              loc.GetString(StringID::AboutPresentations),
              loc.GetString(StringID::AboutMonitoring),
              loc.GetString(StringID::AboutMediaPlayback),
              loc.GetString(StringID::AboutInstructions),
              loc.GetString(StringID::AboutLicense));

    MessageBoxW(m_window, message,
               loc.GetString(StringID::MenuAbout),
               MB_OK | MB_ICONINFORMATION);
}

void App::OnTaskbarCreated() {
    if (!m_trayIcon) {
        return;
    }

    if (m_trayIcon->ReAdd()) {
        m_trayIcon->SetEnabled(m_settings.IsEnabled());
        m_trayIcon->UpdateTooltip(m_settings);
    }
}

void App::Exit() {
    if (m_window) {
        DestroyWindow(m_window);
    }
}

void App::StartTimer() {
    // Stop existing timers
    KillTimer(m_window, TIMER_ID_KEYPRESS);
    KillTimer(m_window, TIMER_ID_EXPIRE);

    if (!m_settings.IsEnabled()) {
        return;
    }

    // Keypress timer (only if a virtual key is configured)
    const WORD vk = m_settings.GetVirtualKey();
    const UINT periodSec = m_settings.GetPeriodSec();
    if (vk != 0 && periodSec > 0) {
        const UINT intervalMs = periodSec * 1000;
        Utils::SetTimerChecked(m_window, TIMER_ID_KEYPRESS, intervalMs);
    }

    // Expiration timer (one-shot)
    TimerConfig timer = m_settings.GetTimerConfig();
    if (timer.mode != TimerMode::Indefinite) {
        // For UntilTime we must pin a concrete end moment, otherwise it would roll to "tomorrow"
        // at the exact moment the timer fires.
        if (timer.mode == TimerMode::UntilTime && timer.endTimeUtc == 0) {
            timer.ResetStartTime();
            m_settings.SetTimerConfig(timer);
            SaveSettings();
        } else if (timer.mode == TimerMode::Duration && timer.endTimeUtc == 0 && timer.startTime.wYear == 0) {
            // Defensive: enabled duration without runtime state
            timer.ResetStartTime();
            m_settings.SetTimerConfig(timer);
            SaveSettings();
        }

        ArmExpireTimer(timer);
    }
}


void App::StopTimer() {
    KillTimer(m_window, TIMER_ID_KEYPRESS);
    KillTimer(m_window, TIMER_ID_EXPIRE);
}


void App::UpdatePowerState() {
    if (m_settings.IsEnabled()) {
        m_powerManager.PreventSleep(m_settings.GetKeepDisplayOn());
    } else {
        m_powerManager.AllowSleep();
    }
}


void App::RegisterHotkey() {
    if (!m_hotkeyManager) {
        return;
    }

    HotkeyConfig config = m_settings.GetHotkeyConfig();
    const bool ok = m_hotkeyManager->RegisterHotkey(config, [this]() { ToggleEnabled(); });

    if (!ok && config.enabled && config.IsValid() && m_trayIcon) {
        auto& loc = Localization::Instance();
        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                     loc.GetString(StringID::NotifyHotkeyFailed), NIIF_WARNING);
    }
}


} // namespace Everon
