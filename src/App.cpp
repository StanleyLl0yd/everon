#include "App.h"
#include "AboutDialog.h"
#include "TrayIcon.h"
#include "SettingsDialog.h"
#include "QuickTimerDialog.h"
#include "HotkeyManager.h"
#include "Utils.h"
#include "Localization.h"
#include "TimerMode.h"
#include "resource.h"
#include <commctrl.h>

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
    const const const const const auto& loc = Localization::Instance();

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

bool App::ArmExpireTimer(const TimerConfig& timer) {
    if (timer.mode != TimerMode::Indefinite && timer.endTimeUtc == 0) {
        m_expireTimerArmed = false;
        DisableAfterTimerFailure();
        return false;
    }

    const DWORD remainingMs = timer.GetRemainingMilliseconds();
    if (remainingMs == 0) {
        m_expireTimerArmed = false;
        return ExpireTimerIfNeeded();
    }

    static constexpr DWORD kWinTimerMaxMs = 0x7FFFFFFFUL;
    static constexpr UINT kLongRearmChunkMs = 10U * 60U * 1000U;

    UINT intervalMs = static_cast<UINT>(remainingMs);
    if (remainingMs > kWinTimerMaxMs) {
        intervalMs = kLongRearmChunkMs;
    }

    m_expireTimerArmed = Utils::SetTimerChecked(
        m_window, TIMER_ID_EXPIRE, intervalMs ? intervalMs : 1U) != 0;
    if (!m_expireTimerArmed) {
        DisableAfterTimerFailure();
    }
    return m_expireTimerArmed;
}

bool App::ExpireTimerIfNeeded() {
    if (!m_settings.IsEnabled()) {
        return false;
    }

    const TimerConfig timer = m_settings.GetTimerConfig();
    if (timer.mode == TimerMode::Indefinite || !timer.IsExpired()) {
        return false;
    }

    KillTimer(m_window, TIMER_ID_EXPIRE);
    m_expireTimerArmed = false;
    m_settings.SetEnabled(false);

    TimerConfig cleared = timer;
    cleared.endTimeUtc = 0;
    cleared.startTime = {};
    cleared.monotonicDeadlineMs = 0;
    m_settings.SetTimerConfig(cleared);

    SaveSettings();
    StopTimer();
    UpdatePowerState();
    if (m_trayIcon) {
        m_trayIcon->SetEnabled(false);
        const const const const const auto& loc = Localization::Instance();
        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                     loc.GetString(StringID::NotifyTimerExpired), NIIF_INFO);
    }
    RefreshStatus();
    return true;
}

void App::DisableAfterTimerFailure() {
    if (!m_settings.IsEnabled()) {
        return;
    }

    m_settings.SetEnabled(false);
    TimerConfig timer = m_settings.GetTimerConfig();
    timer.startTime = {};
    timer.endTimeUtc = 0;
    timer.monotonicDeadlineMs = 0;
    m_settings.SetTimerConfig(timer);
    StopTimer();
    UpdatePowerState();
    SaveSettings();

    if (m_trayIcon) {
        m_trayIcon->SetEnabled(false);
        const const const const const const auto& loc = Localization::Instance();
        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                     loc.GetString(StringID::ErrorTimerState), NIIF_WARNING);
    }
    RefreshStatus();
}

void App::NotifyKeyPressFailure() {
    if (m_keyPressFailureNotified || !m_trayIcon) {
        return;
    }

    const const const const const const auto& loc = Localization::Instance();
    using enum Language;
    const wchar_t* message = nullptr;
    switch (loc.GetLanguage()) {
        case Russian:
            message = L"Не удалось отправить периодическое нажатие клавиши. Everon продолжит предотвращать сон без эмуляции клавиш.";
            break;
        case French:
            message = L"L'envoi périodique de la touche a échoué. Everon continuera d'empêcher la veille sans frappe synthétique.";
            break;
        case German:
            message = L"Das periodische Tastensignal ist fehlgeschlagen. Everon verhindert den Ruhezustand weiterhin ohne simulierte Tastendrücke.";
            break;
        case Italian:
            message = L"L'invio periodico del tasto non è riuscito. Everon continuerà a impedire la sospensione senza pressioni simulate.";
            break;
        case Spanish:
            message = L"Falló el envío periódico de la tecla. Everon seguirá evitando la suspensión sin pulsaciones simuladas.";
            break;
        case English:
        case Count:
        default:
            message = L"Periodic key input failed. Everon will keep preventing sleep without synthetic key presses.";
            break;
    }

    m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle), message, NIIF_WARNING);
    m_keyPressFailureNotified = true;
}

int App::Run() {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_DATE_CLASSES | ICC_LINK_CLASS;
    InitCommonControlsEx(&icc);

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
            return app->OnCreate() ? 0 : -1;
        case WM_TIMER:
            app->OnTimer(static_cast<UINT_PTR>(wParam));
            return 0;
        case TrayIcon::WM_TRAYICON:
            app->OnTrayIcon(lParam);
            return 0;
        case WM_HOTKEY:
            app->OnHotkey(wParam);
            return 0;
        case WM_POWERBROADCAST:
            app->OnPowerBroadcast(wParam);
            return TRUE;
        case WM_SHOW_SETTINGS:
            app->ShowSettings();
            return 0;
        case WM_DESTROY:
            app->OnDestroy();
            return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

bool App::OnCreate() {
    m_settings.LoadFromRegistry();

    m_trayIcon = std::make_unique<TrayIcon>(m_window, m_instance);
    m_trayIcon->SetToggleCallback([this]() { ToggleEnabled(); });
    m_trayIcon->SetDurationCallback([this](DWORD minutes) { SetQuickDuration(minutes); });
    m_trayIcon->SetCustomDurationCallback([this]() { ShowCustomDuration(); });
    m_trayIcon->SetUntilCallback([this]() { ShowQuickUntil(); });
    m_trayIcon->SetSettingsCallback([this]() { ShowSettings(); });
    m_trayIcon->SetAboutCallback([this]() { ShowAbout(); });
    m_trayIcon->SetExitCallback([this]() { Exit(); });

    if (!m_trayIcon->Add()) {
        const const const const const auto& loc = Localization::Instance();
        MessageBoxW(nullptr,
                    loc.GetString(StringID::ErrorTrayIcon),
                    loc.GetString(StringID::ErrorTitle),
                    MB_OK | MB_ICONWARNING);
        return false;
    }

    m_trayIcon->SetEnabled(m_settings.IsEnabled());

    m_hotkeyManager = std::make_unique<HotkeyManager>(m_window);
    RegisterHotkey();

    if (m_settings.IsEnabled()) {
        UpdatePowerState();
        StartTimer();
    }

    if (Utils::SetTimerChecked(m_window, TIMER_ID_STATUS_REFRESH, STATUS_REFRESH_INTERVAL_MS) == 0) {
        Utils::DebugLog(L"[Everon] Status refresh timer is unavailable\n");
    }
    RefreshStatus();
    return true;
}

void App::OnDestroy() {
    StopTimer();
    KillTimer(m_window, TIMER_ID_POWER_RETRY);
    KillTimer(m_window, TIMER_ID_STATUS_REFRESH);
    m_powerManager.AllowSleep();
    m_hotkeyManager.reset();
    m_trayIcon.reset();
    m_window = nullptr;
    PostQuitMessage(0);
}

void App::OnTimer(UINT_PTR timerId) {
    if (timerId == TIMER_ID_POWER_RETRY) {
        UpdatePowerState();
        RefreshStatus();
        return;
    }

    if (timerId == TIMER_ID_STATUS_REFRESH) {
        if (ExpireTimerIfNeeded()) {
            return;
        }
        if (m_settings.IsEnabled()) {
            const TimerConfig timer = m_settings.GetTimerConfig();
            if (timer.mode != TimerMode::Indefinite && !m_expireTimerArmed) {
                ArmExpireTimer(timer);
            }
        }
        UpdatePowerState();
        RefreshStatus();
        return;
    }

    if (!m_settings.IsEnabled()) {
        return;
    }

    if (timerId == TIMER_ID_KEYPRESS) {
        if (m_pausedByBatterySaver) {
            return;
        }
        const WORD vk = m_settings.GetVirtualKey();
        if (vk != 0) {
            if (m_powerManager.SendKeyPress(vk)) {
                m_keyPressFailureNotified = false;
            } else {
                NotifyKeyPressFailure();
            }
        }
        return;
    }

    if (timerId == TIMER_ID_EXPIRE) {
        KillTimer(m_window, TIMER_ID_EXPIRE);
        m_expireTimerArmed = false;
        if (!ExpireTimerIfNeeded()) {
            ArmExpireTimer(m_settings.GetTimerConfig());
            RefreshStatus();
        }
    }
}

void App::OnTrayIcon(LPARAM lParam) {
    if (m_trayIcon) {
        RefreshStatus();
        m_trayIcon->HandleMessage(lParam);
    }
}

void App::OnHotkey(WPARAM wParam) {
    if (m_hotkeyManager) {
        m_hotkeyManager->HandleHotkey(wParam);
    }
}

void App::OnPowerBroadcast(WPARAM eventType) {
    if (eventType == PBT_APMRESUMEAUTOMATIC ||
        eventType == PBT_APMRESUMESUSPEND ||
        eventType == PBT_APMPOWERSTATUSCHANGE) {
        if (m_settings.IsEnabled()) {
            StartTimer();
        }
        UpdatePowerState();
        RefreshStatus();
    }
}

void App::ToggleEnabled() {
    const bool newEnabled = !m_settings.IsEnabled();
    m_settings.SetEnabled(newEnabled);

    if (newEnabled) {
        TimerConfig timer = m_settings.GetTimerConfig();
        if (timer.mode != TimerMode::Indefinite) {
            timer.ResetStartTime();
        } else {
            timer.startTime = {};
            timer.endTimeUtc = 0;
            timer.monotonicDeadlineMs = 0;
        }
        m_settings.SetTimerConfig(timer);

        UpdatePowerState();
        StartTimer();
    } else {
        StopTimer();
        UpdatePowerState();

        TimerConfig timer = m_settings.GetTimerConfig();
        timer.startTime = {};
        timer.endTimeUtc = 0;
        timer.monotonicDeadlineMs = 0;
        m_settings.SetTimerConfig(timer);
    }

    SaveSettings();
    if (m_trayIcon) {
        m_trayIcon->SetEnabled(m_settings.IsEnabled());
        RefreshStatus();

        if (m_settings.GetShowToggleNotifications()) {
            const const const const const auto& loc = Localization::Instance();
            m_trayIcon->ShowNotification(
                loc.GetString(StringID::ErrorTitle),
                m_settings.IsEnabled() ? loc.GetString(StringID::NotifyEnabled)
                                       : loc.GetString(StringID::NotifyDisabled),
                NIIF_INFO);
        }
    }
}

void App::SetQuickDuration(DWORD minutes) {
    if (minutes < TimerConfig::MIN_DURATION_MIN || minutes > TimerConfig::MAX_DURATION_MIN) {
        return;
    }

    const bool wasEnabled = m_settings.IsEnabled();
    TimerConfig timer = m_settings.GetTimerConfig();
    timer.mode = TimerMode::Duration;
    timer.durationMinutes = minutes;
    timer.ResetStartTime();

    m_settings.SetTimerConfig(timer);
    m_settings.SetEnabled(true);
    UpdatePowerState();
    StartTimer();
    SaveSettings();

    if (m_trayIcon) {
        m_trayIcon->SetEnabled(true);
        RefreshStatus();

        if (!wasEnabled && m_settings.GetShowToggleNotifications()) {
            const const const const const auto& loc = Localization::Instance();
            m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                         loc.GetString(StringID::NotifyEnabled), NIIF_INFO);
        }
    }
}

void App::SetQuickUntil(const SYSTEMTIME& untilTime) {
    const bool wasEnabled = m_settings.IsEnabled();
    TimerConfig timer = m_settings.GetTimerConfig();
    timer.mode = TimerMode::UntilTime;
    timer.untilTime = untilTime;
    timer.ResetStartTime();

    m_settings.SetTimerConfig(timer);
    m_settings.SetEnabled(true);
    UpdatePowerState();
    StartTimer();
    SaveSettings();

    if (m_trayIcon) {
        m_trayIcon->SetEnabled(true);
        RefreshStatus();

        if (!wasEnabled && m_settings.GetShowToggleNotifications()) {
            const const const const const auto& loc = Localization::Instance();
            m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                         loc.GetString(StringID::NotifyEnabled), NIIF_INFO);
        }
    }
}

void App::ShowCustomDuration() {
    TimerConfig timer = m_settings.GetTimerConfig();
    DWORD minutes = timer.mode == TimerMode::Duration ? timer.durationMinutes : 60;
    if (ShowQuickDurationDialog(m_instance, m_window, minutes, minutes)) {
        SetQuickDuration(minutes);
    }
}

void App::ShowQuickUntil() {
    TimerConfig timer = m_settings.GetTimerConfig();
    SYSTEMTIME untilTime = timer.untilTime;
    if (untilTime.wYear == 0) {
        GetLocalTime(&untilTime);
    }
    if (ShowQuickUntilDialog(m_instance, m_window, untilTime, untilTime)) {
        SetQuickUntil(untilTime);
    }
}

void App::ShowSettings() {
    if (!m_settingsDialog || m_isSettingsDialogOpen) {
        return;
    }

    const Settings original = m_settings;
    Settings staged = m_settings;
    const bool originalAutoStart = Settings::IsAutoStartEnabled();

    m_isSettingsDialogOpen = true;
    const bool accepted = m_settingsDialog->Show(m_window, staged);
    m_isSettingsDialogOpen = false;

    if (!accepted) {
        Localization::Instance().SetLanguage(original.GetLanguage());
        return;
    }

    if (TimerConfig timer = staged.GetTimerConfig();
        staged.IsEnabled() && timer.mode != TimerMode::Indefinite && timer.endTimeUtc == 0) {
        timer.ResetStartTime();
        staged.SetTimerConfig(timer);
    }

    const bool autoStartChanged = staged.GetAutoStart() != originalAutoStart;
    if (autoStartChanged && !Settings::SetAutoStartEnabled(staged.GetAutoStart())) {
        Localization::Instance().SetLanguage(original.GetLanguage());
        const const const const const const auto& loc = Localization::Instance();
        MessageBoxW(m_window,
                    loc.GetString(StringID::ErrorAutoStart),
                    loc.GetString(StringID::ErrorTitle),
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (!staged.SaveToRegistry()) {
        if (autoStartChanged && !Settings::SetAutoStartEnabled(originalAutoStart)) {
            Utils::DebugLog(L"[Everon] Failed to roll back autostart\n");
        }
        Settings rollback = original;
        rollback.SetDirty(true);
        if (!rollback.SaveToRegistry()) {
            Utils::DebugLog(L"[Everon] Failed to roll back settings registry state\n");
        }
        Localization::Instance().SetLanguage(original.GetLanguage());
        const const const const const const auto& loc = Localization::Instance();
        MessageBoxW(m_window,
                    loc.GetString(StringID::ErrorSaveSettings),
                    loc.GetString(StringID::ErrorTitle),
                    MB_OK | MB_ICONWARNING);
        return;
    }

    m_settings = staged;
    if (m_settings.IsEnabled()) {
        UpdatePowerState();
        StartTimer();
    } else {
        StopTimer();
        UpdatePowerState();
    }

    if (m_trayIcon) {
        m_trayIcon->SetEnabled(m_settings.IsEnabled());
        RefreshStatus();
    }
    RegisterHotkey();
}

void App::ShowAbout() {
    ShowAboutDialog(m_instance, m_window);
}

void App::OnTaskbarCreated() {
    if (!m_trayIcon) {
        return;
    }

    if (m_trayIcon->ReAdd()) {
        m_trayIcon->SetEnabled(m_settings.IsEnabled());
        RefreshStatus();
    }
}

void App::Exit() {
    if (m_window) {
        DestroyWindow(m_window);
    }
}

void App::StartTimer() {
    KillTimer(m_window, TIMER_ID_KEYPRESS);
    KillTimer(m_window, TIMER_ID_EXPIRE);
    m_expireTimerArmed = false;

    if (!m_settings.IsEnabled()) {
        return;
    }

    const WORD vk = m_settings.GetVirtualKey();
    const UINT periodSec = m_settings.GetPeriodSec();
    if (vk != 0 && periodSec > 0) {
        const UINT intervalMs = periodSec * 1000U;
        if (Utils::SetTimerChecked(m_window, TIMER_ID_KEYPRESS, intervalMs) == 0) {
            NotifyKeyPressFailure();
        }
    }

    TimerConfig timer = m_settings.GetTimerConfig();
    if (timer.mode == TimerMode::Indefinite) {
        return;
    }

    bool persistRuntimeState = false;
    if (timer.mode == TimerMode::UntilTime && timer.endTimeUtc == 0) {
        timer.ResetStartTime();
        persistRuntimeState = true;
    } else if (timer.mode == TimerMode::Duration) {
        if (timer.endTimeUtc == 0) {
            timer.ResetStartTime();
            persistRuntimeState = true;
        } else {
            timer.ResumeMonotonicDuration();
            m_settings.SetTimerRuntimeDeadline(timer.monotonicDeadlineMs);
        }
    }

    if (timer.endTimeUtc == 0) {
        DisableAfterTimerFailure();
        return;
    }

    m_settings.SetTimerConfig(timer);
    if (persistRuntimeState) {
        SaveSettings();
    }
    ArmExpireTimer(timer);
}

void App::StopTimer() {
    KillTimer(m_window, TIMER_ID_KEYPRESS);
    KillTimer(m_window, TIMER_ID_EXPIRE);
    m_expireTimerArmed = false;
    m_keyPressFailureNotified = false;
}

void App::RefreshStatus() {
    if (m_trayIcon) {
        m_trayIcon->UpdateTooltip(m_settings,
                                  m_pausedByBatterySaver,
                                  m_powerManager.IsKeepingDisplayOn());
    }
}

PowerContext App::QueryPowerContext() const noexcept {
    SYSTEM_POWER_STATUS status = {};
    if (!GetSystemPowerStatus(&status) || status.ACLineStatus == 255) {
        PowerContext unknown;
        unknown.statusKnown = false;
        return unknown;
    }

    PowerContext context;
    context.onBattery = status.ACLineStatus == 0;
    context.batterySaver = status.SystemStatusFlag != 0;
    return context;
}

bool App::UpdatePowerState() {
    const PowerDecision decision = EvaluatePowerPolicy(
        m_settings.IsEnabled(),
        m_settings.GetKeepDisplayOn(),
        m_settings.GetRespectBatterySaver(),
        m_settings.GetAllowDisplayOnBattery(),
        QueryPowerContext());

    m_pausedByBatterySaver = decision.pausedByBatterySaver;
    const bool ok = decision.preventSystemSleep
        ? m_powerManager.PreventSleep(decision.keepDisplayOn)
        : m_powerManager.AllowSleep();

    if (ok) {
        KillTimer(m_window, TIMER_ID_POWER_RETRY);
        m_powerFailureNotified = false;
        return true;
    }

    Utils::SetTimerChecked(m_window, TIMER_ID_POWER_RETRY, POWER_RETRY_INTERVAL_MS);
    if (!m_powerFailureNotified && m_trayIcon) {
        const const const const const auto& loc = Localization::Instance();
        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                     loc.GetString(StringID::ErrorPowerState), NIIF_WARNING);
        m_powerFailureNotified = true;
    }
    return false;
}

void App::RegisterHotkey() {
    if (!m_hotkeyManager) {
        return;
    }

    HotkeyConfig config = m_settings.GetHotkeyConfig();
    const bool ok = m_hotkeyManager->RegisterHotkey(config, [this]() { ToggleEnabled(); });

    if (!ok && config.enabled && config.IsValid() && m_trayIcon) {
        const const const const const auto& loc = Localization::Instance();
        m_trayIcon->ShowNotification(loc.GetString(StringID::ErrorTitle),
                                     loc.GetString(StringID::NotifyHotkeyFailed), NIIF_WARNING);
    }
}

}
