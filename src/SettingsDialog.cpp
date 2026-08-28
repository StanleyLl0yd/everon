#include "SettingsDialog.h"
#include "Settings.h"
#include "Utils.h"
#include "Localization.h"
#include "HotkeyManager.h"
#include "TimerMode.h"
#include "resource.h"
#include <commctrl.h>

#include <array>
#include <bit>

namespace Everon {

SettingsDialog::SettingsDialog(HINSTANCE instance)
    : m_instance(instance) {
}

bool SettingsDialog::Show(HWND parent, Settings& settings) {
    m_settings = &settings;
    const auto oldLang = m_settings->GetLanguage();
    const auto oldDirty = m_settings->IsDirty();

    const auto result = DialogBoxParamW(m_instance, MAKEINTRESOURCEW(IDD_SETTINGS),
                                        parent, DialogProc, std::bit_cast<LPARAM>(this));

    if (result != IDOK) {
        m_settings->SetLanguage(oldLang);
        m_settings->SetDirty(oldDirty);
    }

    m_settings = nullptr;
    return result == IDOK;
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND dialog, UINT message,
                                            WPARAM wParam, LPARAM lParam) {
    SettingsDialog* instance = nullptr;

    if (message == WM_INITDIALOG) {
        instance = std::bit_cast<SettingsDialog*>(lParam);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, lParam);
    } else {
        instance = std::bit_cast<SettingsDialog*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));
    }

    if (!instance) {
        return FALSE;
    }

    switch (message) {
        case WM_INITDIALOG:
            instance->OnInitDialog(dialog);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    if (instance->OnOkClicked(dialog)) {
                        EndDialog(dialog, IDOK);
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;

                case IDC_LANGUAGE_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        instance->OnLanguageChanged(dialog);
                    }
                    return TRUE;

                case IDC_KEY_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        instance->OnKeyPressChanged(dialog);
                    }
                    return TRUE;

                case IDC_KEEPDISPLAY_CHECK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        instance->UpdatePowerControlsState(dialog);
                    }
                    return TRUE;

                case IDC_TIMER_INDEFINITE:
                case IDC_TIMER_DURATION:
                case IDC_TIMER_UNTIL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        instance->OnTimerModeChanged(dialog);
                    }
                    return TRUE;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            if (const auto* header = std::bit_cast<const NMHDR*>(lParam);
                header && header->idFrom == IDC_TIMER_UNTIL_TIME &&
                header->code == DTN_DATETIMECHANGE) {
                instance->UpdateUntilHint(dialog);
                return TRUE;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

void SettingsDialog::OnInitDialog(HWND dialog) const {
    if (!m_settings) {
        return;
    }

    SetDlgItemInt(dialog, IDC_PERIOD_EDIT, m_settings->GetPeriodSec(), FALSE);
    CheckDlgButton(dialog, IDC_KEEPDISPLAY_CHECK,
                   m_settings->GetKeepDisplayOn() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_RESPECT_BATTERY_SAVER,
                   m_settings->GetRespectBatterySaver() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ALLOW_DISPLAY_BATTERY,
                   m_settings->GetAllowDisplayOnBattery() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_NOTIFY_TOGGLE_CHECK,
                   m_settings->GetShowToggleNotifications() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_AUTOSTART_CHECK,
                   m_settings->GetAutoStart() ? BST_CHECKED : BST_UNCHECKED);
    PopulateLanguageComboBox(dialog);
    PopulateKeyComboBox(GetDlgItem(dialog, IDC_KEY_COMBO), m_settings->GetVirtualKey());
    UpdateKeyPressControlsState(dialog);
    UpdatePowerControlsState(dialog);
    PopulateHotkeyComboBox(dialog);
    InitializeTimerControls(dialog);
    UpdateDialogText(dialog);
    Utils::CenterWindowOnMonitor(dialog, GetWindow(dialog, GW_OWNER));
}

void SettingsDialog::UpdateDialogText(HWND dialog) const {
    using enum StringID;

    const auto& loc = Localization::Instance();
    SetWindowTextW(dialog, loc.GetString(SettingsTitle));
    SetDlgItemTextW(dialog, IDC_LANGUAGE_LABEL, loc.GetString(SettingsLanguage));
    SetDlgItemTextW(dialog, IDC_GENERAL_GROUP, loc.GetString(SettingsGeneral));
    SetDlgItemTextW(dialog, IDC_PERIOD_LABEL, loc.GetString(SettingsPeriod));
    SetDlgItemTextW(dialog, IDC_PERIOD_SECONDS_LABEL, loc.GetString(SettingsPeriodSeconds));
    SetDlgItemTextW(dialog, IDC_KEYPRESS_LABEL, loc.GetString(SettingsKeyPress));
    SetDlgItemTextW(dialog, IDC_KEEPDISPLAY_CHECK, loc.GetString(SettingsKeepDisplay));
    SetDlgItemTextW(dialog, IDC_RESPECT_BATTERY_SAVER, loc.GetString(SettingsRespectBatterySaver));
    SetDlgItemTextW(dialog, IDC_ALLOW_DISPLAY_BATTERY, loc.GetString(SettingsAllowDisplayOnBattery));
    SetDlgItemTextW(dialog, IDC_NOTIFY_TOGGLE_CHECK, loc.GetString(SettingsNotifyOnToggle));
    SetDlgItemTextW(dialog, IDC_AUTOSTART_CHECK, loc.GetString(SettingsAutoStart));
    SetDlgItemTextW(dialog, IDC_TIMER_GROUP, loc.GetString(SettingsTimer));
    SetDlgItemTextW(dialog, IDC_TIMER_INDEFINITE, loc.GetString(SettingsTimerIndefinite));
    SetDlgItemTextW(dialog, IDC_TIMER_DURATION, loc.GetString(SettingsTimerDuration));
    SetDlgItemTextW(dialog, IDC_TIMER_DURATION_LABEL, loc.GetString(SettingsTimerMinutes));
    SetDlgItemTextW(dialog, IDC_TIMER_UNTIL, loc.GetString(SettingsTimerUntilTime));
    SetDlgItemTextW(dialog, IDC_HOTKEYS_GROUP, loc.GetString(SettingsHotkeys));
    SetDlgItemTextW(dialog, IDC_HOTKEY_LABEL, loc.GetString(SettingsHotkeyLabel));
    SetDlgItemTextW(dialog, IDOK, loc.GetString(ButtonOK));
    SetDlgItemTextW(dialog, IDCANCEL, loc.GetString(ButtonCancel));
    UpdateUntilHint(dialog);
}

void SettingsDialog::PopulateLanguageComboBox(HWND dialog) const {
    using enum Language;

    const auto combo = GetDlgItem(dialog, IDC_LANGUAGE_COMBO);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);

    const std::array languages{English, Russian, French, German, Italian, Spanish};

    int selectedIndex = 0;
    const auto currentLang = m_settings->GetLanguage();

    for (const auto language : languages) {
        const auto* name = Localization::GetLanguageName(language);
        const auto index = static_cast<int>(
            SendMessageW(combo, CB_ADDSTRING, 0, std::bit_cast<LPARAM>(name)));
        SendMessageW(combo, CB_SETITEMDATA, index, static_cast<LPARAM>(language));

        if (language == currentLang) {
            selectedIndex = index;
        }
    }

    SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::PopulateKeyComboBox(HWND comboBox, WORD selectedKey) const {
    using enum StringID;

    SendMessageW(comboBox, CB_RESETCONTENT, 0, 0);

    const auto& loc = Localization::Instance();

    struct KeyItem {
        const wchar_t* text;
        WORD virtualKey;
    };

    const std::array<KeyItem, 4> items{{
        {loc.GetString(SettingsKeyPressOff), 0},
        {L"F15", VK_F15},
        {L"F16", VK_F16},
        {L"F17", VK_F17},
    }};

    int selectedIndex = 0;

    for (const auto& item : items) {
        const auto index = static_cast<int>(
            SendMessageW(comboBox, CB_ADDSTRING, 0, std::bit_cast<LPARAM>(item.text)));
        SendMessageW(comboBox, CB_SETITEMDATA, index, item.virtualKey);
        if (item.virtualKey == selectedKey) {
            selectedIndex = index;
        }
    }

    SendMessageW(comboBox, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::PopulateHotkeyComboBox(HWND dialog) const {
    using enum StringID;

    const auto combo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);

    const auto& loc = Localization::Instance();

    struct HotkeyItem {
        const wchar_t* text;
        UINT modifiers;
        UINT vk;
    };

    const std::array<HotkeyItem, 6> items{{
        {loc.GetString(SettingsHotkeyNone), 0, 0},
        {L"Ctrl+Shift+E", MOD_CONTROL | MOD_SHIFT, 'E'},
        {L"Ctrl+Alt+E", MOD_CONTROL | MOD_ALT, 'E'},
        {L"Alt+F12", MOD_ALT, VK_F12},
        {L"Ctrl+Shift+F12", MOD_CONTROL | MOD_SHIFT, VK_F12},
        {L"Win+Pause", MOD_WIN, VK_PAUSE},
    }};

    const auto currentHotkey = m_settings->GetHotkeyConfig();
    int selectedIndex = 0;

    for (const auto& item : items) {
        const auto index = static_cast<int>(
            SendMessageW(combo, CB_ADDSTRING, 0, std::bit_cast<LPARAM>(item.text)));
        const auto data = MAKELPARAM(item.modifiers, item.vk);
        SendMessageW(combo, CB_SETITEMDATA, index, data);

        if (currentHotkey.enabled &&
            item.modifiers == currentHotkey.modifiers &&
            item.vk == currentHotkey.virtualKey) {
            selectedIndex = index;
        }
    }

    SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::OnLanguageChanged(HWND dialog) const {
    const auto languageCombo = GetDlgItem(dialog, IDC_LANGUAGE_COMBO);
    const auto languageIndex = static_cast<int>(SendMessageW(languageCombo, CB_GETCURSEL, 0, 0));
    if (languageIndex < 0) {
        return;
    }

    const auto language = static_cast<Language>(
        SendMessageW(languageCombo, CB_GETITEMDATA, languageIndex, 0));
    if (language == m_settings->GetLanguage()) {
        return;
    }

    const auto keyCombo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const auto keyIndex = static_cast<int>(SendMessageW(keyCombo, CB_GETCURSEL, 0, 0));
    auto selectedKey = m_settings->GetVirtualKey();
    if (keyIndex >= 0) {
        selectedKey = static_cast<WORD>(SendMessageW(keyCombo, CB_GETITEMDATA, keyIndex, 0));
    }

    const auto hotkeyCombo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    const auto hotkeyIndex = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCURSEL, 0, 0));
    LPARAM selectedHotkeyData = 0;
    if (hotkeyIndex >= 0) {
        selectedHotkeyData = SendMessageW(hotkeyCombo, CB_GETITEMDATA, hotkeyIndex, 0);
    }

    m_settings->SetLanguage(language);
    UpdateDialogText(dialog);
    PopulateKeyComboBox(keyCombo, selectedKey);
    UpdateKeyPressControlsState(dialog);
    PopulateHotkeyComboBox(dialog);

    const auto hotkeyCount = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < hotkeyCount; ++i) {
        if (SendMessageW(hotkeyCombo, CB_GETITEMDATA, i, 0) == selectedHotkeyData) {
            SendMessageW(hotkeyCombo, CB_SETCURSEL, i, 0);
            break;
        }
    }
}

void SettingsDialog::OnKeyPressChanged(HWND dialog) const {
    UpdateKeyPressControlsState(dialog);
}

bool SettingsDialog::OnOkClicked(HWND dialog) const {
    using enum StringID;
    using enum TimerMode;

    if (!m_settings) {
        return false;
    }

    const auto& loc = Localization::Instance();

    const auto keyCombo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const auto keySelection = static_cast<int>(SendMessageW(keyCombo, CB_GETCURSEL, 0, 0));
    WORD virtualKey = 0;
    if (keySelection >= 0) {
        virtualKey = static_cast<WORD>(SendMessageW(keyCombo, CB_GETITEMDATA, keySelection, 0));
    }

    BOOL translated = FALSE;
    auto period = GetDlgItemInt(dialog, IDC_PERIOD_EDIT, &translated, FALSE);
    if (virtualKey != 0 && (!translated || !m_settings->IsValidPeriod(period))) {
        MessageBoxW(dialog,
                    loc.GetString(ErrorInvalidPeriod),
                    loc.GetString(ErrorInvalidPeriodTitle),
                    MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(dialog, IDC_PERIOD_EDIT));
        return false;
    }
    if (virtualKey == 0 && (!translated || !m_settings->IsValidPeriod(period))) {
        period = m_settings->GetPeriodSec();
    }

    const auto keepDisplayOn = IsDlgButtonChecked(dialog, IDC_KEEPDISPLAY_CHECK) == BST_CHECKED;
    const auto respectBatterySaver =
        IsDlgButtonChecked(dialog, IDC_RESPECT_BATTERY_SAVER) == BST_CHECKED;
    const auto allowDisplayOnBattery =
        IsDlgButtonChecked(dialog, IDC_ALLOW_DISPLAY_BATTERY) == BST_CHECKED;
    const auto notifyOnToggle = IsDlgButtonChecked(dialog, IDC_NOTIFY_TOGGLE_CHECK) == BST_CHECKED;
    const auto autoStart = IsDlgButtonChecked(dialog, IDC_AUTOSTART_CHECK) == BST_CHECKED;

    HotkeyConfig hotkey{};
    const auto hotkeyCombo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    if (const auto hotkeySelection = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCURSEL, 0, 0));
        hotkeySelection >= 0) {
        const auto data = SendMessageW(hotkeyCombo, CB_GETITEMDATA, hotkeySelection, 0);
        hotkey.modifiers = LOWORD(data);
        hotkey.virtualKey = HIWORD(data);
    }
    hotkey.enabled = hotkey.IsValid();

    auto timer = m_settings->GetTimerConfig();
    const auto oldTimer = timer;

    if (IsDlgButtonChecked(dialog, IDC_TIMER_INDEFINITE) == BST_CHECKED) {
        timer.mode = Indefinite;
    } else if (IsDlgButtonChecked(dialog, IDC_TIMER_DURATION) == BST_CHECKED) {
        timer.mode = Duration;

        BOOL translatedDuration = FALSE;
        const auto duration = GetDlgItemInt(dialog, IDC_TIMER_DURATION_EDIT, &translatedDuration, FALSE);
        if (!translatedDuration ||
            duration < TimerConfig::MIN_DURATION_MIN ||
            duration > TimerConfig::MAX_DURATION_MIN) {
            MessageBoxW(dialog,
                        loc.GetString(ErrorInvalidTimerDuration),
                        loc.GetString(ErrorInvalidTimerTitle),
                        MB_OK | MB_ICONWARNING);
            return false;
        }
        timer.durationMinutes = duration;
    } else if (IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) == BST_CHECKED) {
        timer.mode = UntilTime;

        if (const auto timePicker = GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME);
            DateTime_GetSystemtime(timePicker, &timer.untilTime) != GDT_VALID) {
            GetLocalTime(&timer.untilTime);
        }

        SYSTEMTIME now{};
        GetLocalTime(&now);
        timer.untilTime.wYear = now.wYear;
        timer.untilTime.wMonth = now.wMonth;
        timer.untilTime.wDay = now.wDay;
        timer.untilTime.wSecond = 0;
        timer.untilTime.wMilliseconds = 0;
    }

    if (const auto timerChanged =
            timer.mode != oldTimer.mode ||
            (timer.mode == Duration && timer.durationMinutes != oldTimer.durationMinutes) ||
            (timer.mode == UntilTime &&
             (timer.untilTime.wHour != oldTimer.untilTime.wHour ||
              timer.untilTime.wMinute != oldTimer.untilTime.wMinute));
        timerChanged || timer.mode == Indefinite) {
        timer.startTime = {};
        timer.endTimeUtc = 0;
        timer.monotonicDeadlineMs = 0;
    }

    m_settings->SetPeriodSec(period);
    m_settings->SetVirtualKey(virtualKey);
    m_settings->SetKeepDisplayOn(keepDisplayOn);
    m_settings->SetRespectBatterySaver(respectBatterySaver);
    m_settings->SetAllowDisplayOnBattery(allowDisplayOnBattery);
    m_settings->SetShowToggleNotifications(notifyOnToggle);
    m_settings->SetAutoStart(autoStart);
    m_settings->SetHotkeyConfig(hotkey);
    m_settings->SetTimerConfig(timer);
    return true;
}

void SettingsDialog::InitializeTimerControls(HWND dialog) const {
    using enum TimerMode;

    auto timer = m_settings->GetTimerConfig();

    int radioButton = IDC_TIMER_INDEFINITE;
    switch (timer.mode) {
        case Indefinite:
            radioButton = IDC_TIMER_INDEFINITE;
            break;

        case Duration:
            radioButton = IDC_TIMER_DURATION;
            SetDlgItemInt(dialog, IDC_TIMER_DURATION_EDIT, timer.durationMinutes, FALSE);
            break;

        case UntilTime:
            radioButton = IDC_TIMER_UNTIL;
            DateTime_SetSystemtime(GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME), GDT_VALID, &timer.untilTime);
            break;

        default:
            break;
    }
    CheckRadioButton(dialog, IDC_TIMER_INDEFINITE, IDC_TIMER_UNTIL, radioButton);
    UpdateTimerControlsState(dialog);
}

void SettingsDialog::UpdateKeyPressControlsState(HWND dialog) const {
    const auto combo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const auto selection = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    WORD virtualKey = 0;
    if (selection >= 0) {
        virtualKey = static_cast<WORD>(SendMessageW(combo, CB_GETITEMDATA, selection, 0));
    }
    const BOOL enabled = virtualKey != 0 ? TRUE : FALSE;
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_EDIT), enabled);
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_LABEL), enabled);
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_SECONDS_LABEL), enabled);
}

void SettingsDialog::UpdatePowerControlsState(HWND dialog) const {
    const BOOL keepDisplay = IsDlgButtonChecked(dialog, IDC_KEEPDISPLAY_CHECK) == BST_CHECKED;
    EnableWindow(GetDlgItem(dialog, IDC_ALLOW_DISPLAY_BATTERY), keepDisplay);
}

void SettingsDialog::UpdateTimerControlsState(HWND dialog) const {
    const BOOL duration = IsDlgButtonChecked(dialog, IDC_TIMER_DURATION) == BST_CHECKED;
    const BOOL until = IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) == BST_CHECKED;

    EnableWindow(GetDlgItem(dialog, IDC_TIMER_DURATION_EDIT), duration);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_DURATION_LABEL), duration);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME), until);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_UNTIL_HINT), until);
    UpdateUntilHint(dialog);
}

void SettingsDialog::UpdateUntilHint(HWND dialog) const {
    using enum TimerMode;

    if (IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) != BST_CHECKED) {
        SetDlgItemTextW(dialog, IDC_TIMER_UNTIL_HINT, L"");
        return;
    }

    SYSTEMTIME selected{};
    if (DateTime_GetSystemtime(GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME), &selected) != GDT_VALID) {
        SetDlgItemTextW(dialog, IDC_TIMER_UNTIL_HINT, L"");
        return;
    }

    TimerConfig timer;
    timer.mode = UntilTime;
    timer.untilTime = selected;
    SetDlgItemTextW(dialog, IDC_TIMER_UNTIL_HINT,
                    timer.IsUntilNextDay()
                        ? Localization::Instance().GetString(StringID::SettingsTimerTomorrow)
                        : L"");
}

void SettingsDialog::OnTimerModeChanged(HWND dialog) const {
    UpdateTimerControlsState(dialog);
}

}