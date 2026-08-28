#include "SettingsDialog.h"
#include "Settings.h"
#include "Utils.h"
#include "Localization.h"
#include "HotkeyManager.h"
#include "TimerMode.h"
#include "resource.h"
#include <commctrl.h>

namespace Everon {

SettingsDialog::SettingsDialog(HINSTANCE instance)
    : m_instance(instance) {
}

bool SettingsDialog::Show(HWND parent, Settings& settings) {
    m_settings = &settings;
    const Language oldLang = m_settings->GetLanguage();
    const bool oldDirty = m_settings->IsDirty();

    INT_PTR result = DialogBoxParamW(m_instance, MAKEINTRESOURCEW(IDD_SETTINGS),
                                    parent, DialogProc, reinterpret_cast<LPARAM>(this));

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
        instance = reinterpret_cast<SettingsDialog*>(lParam);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, lParam);
    } else {
        instance = reinterpret_cast<SettingsDialog*>(
            GetWindowLongPtrW(dialog, GWLP_USERDATA));
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
            }
            break;
        case WM_NOTIFY: {
            const auto* header = reinterpret_cast<const NMHDR*>(lParam);
            if (header && header->idFrom == IDC_TIMER_UNTIL_TIME &&
                header->code == DTN_DATETIMECHANGE) {
                instance->UpdateUntilHint(dialog);
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}

void SettingsDialog::OnInitDialog(HWND dialog) {
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

void SettingsDialog::UpdateDialogText(HWND dialog) {
    const const const const auto& loc = Localization::Instance();
    SetWindowTextW(dialog, loc.GetString(StringID::SettingsTitle));
    SetDlgItemTextW(dialog, IDC_LANGUAGE_LABEL, loc.GetString(StringID::SettingsLanguage));
    SetDlgItemTextW(dialog, IDC_GENERAL_GROUP, loc.GetString(StringID::SettingsGeneral));
    SetDlgItemTextW(dialog, IDC_PERIOD_LABEL, loc.GetString(StringID::SettingsPeriod));
    SetDlgItemTextW(dialog, IDC_PERIOD_SECONDS_LABEL, loc.GetString(StringID::SettingsPeriodSeconds));
    SetDlgItemTextW(dialog, IDC_KEYPRESS_LABEL, loc.GetString(StringID::SettingsKeyPress));
    SetDlgItemTextW(dialog, IDC_KEEPDISPLAY_CHECK, loc.GetString(StringID::SettingsKeepDisplay));
    SetDlgItemTextW(dialog, IDC_RESPECT_BATTERY_SAVER,
                    loc.GetString(StringID::SettingsRespectBatterySaver));
    SetDlgItemTextW(dialog, IDC_ALLOW_DISPLAY_BATTERY,
                    loc.GetString(StringID::SettingsAllowDisplayOnBattery));
    SetDlgItemTextW(dialog, IDC_NOTIFY_TOGGLE_CHECK, loc.GetString(StringID::SettingsNotifyOnToggle));
    SetDlgItemTextW(dialog, IDC_AUTOSTART_CHECK, loc.GetString(StringID::SettingsAutoStart));
    SetDlgItemTextW(dialog, IDC_TIMER_GROUP, loc.GetString(StringID::SettingsTimer));
    SetDlgItemTextW(dialog, IDC_TIMER_INDEFINITE, loc.GetString(StringID::SettingsTimerIndefinite));
    SetDlgItemTextW(dialog, IDC_TIMER_DURATION, loc.GetString(StringID::SettingsTimerDuration));
    SetDlgItemTextW(dialog, IDC_TIMER_DURATION_LABEL, loc.GetString(StringID::SettingsTimerMinutes));
    SetDlgItemTextW(dialog, IDC_TIMER_UNTIL, loc.GetString(StringID::SettingsTimerUntilTime));
    SetDlgItemTextW(dialog, IDC_HOTKEYS_GROUP, loc.GetString(StringID::SettingsHotkeys));
    SetDlgItemTextW(dialog, IDC_HOTKEY_LABEL, loc.GetString(StringID::SettingsHotkeyLabel));
    SetDlgItemTextW(dialog, IDOK, loc.GetString(StringID::ButtonOK));
    SetDlgItemTextW(dialog, IDCANCEL, loc.GetString(StringID::ButtonCancel));
    UpdateUntilHint(dialog);
}

void SettingsDialog::PopulateLanguageComboBox(HWND dialog) {
    HWND combo = GetDlgItem(dialog, IDC_LANGUAGE_COMBO);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);

    const Language languages[] = {
        Language::English, Language::Russian, Language::French,
        Language::German, Language::Italian, Language::Spanish
    };

    int selectedIndex = 0;
    Language currentLang = m_settings->GetLanguage();

    for (size_t i = 0; i < _countof(languages); ++i) {
        const wchar_t* name = Localization::GetLanguageName(languages[i]);
        int index = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0,
                                                 reinterpret_cast<LPARAM>(name)));
        SendMessageW(combo, CB_SETITEMDATA, index, static_cast<LPARAM>(languages[i]));

        if (languages[i] == currentLang) {
            selectedIndex = index;
        }
    }

    SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::PopulateKeyComboBox(HWND comboBox, WORD selectedKey) {
    SendMessageW(comboBox, CB_RESETCONTENT, 0, 0);

    const const const const auto& loc = Localization::Instance();

    struct KeyItem {
        const wchar_t* text;
        WORD virtualKey;
    };

    const KeyItem items[] = {
        { loc.GetString(StringID::SettingsKeyPressOff), 0 },
        { L"F15", VK_F15 },
        { L"F16", VK_F16 },
        { L"F17", VK_F17 }
    };

    int selectedIndex = 0;

    for (size_t i = 0; i < _countof(items); ++i) {
        const int index = static_cast<int>(
            SendMessageW(comboBox, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(items[i].text))
        );
        SendMessageW(comboBox, CB_SETITEMDATA, index, items[i].virtualKey);
        if (items[i].virtualKey == selectedKey) {
            selectedIndex = index;
        }
    }

    SendMessageW(comboBox, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::PopulateHotkeyComboBox(HWND dialog) {
    HWND combo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);

    const const const const auto& loc = Localization::Instance();

    struct HotkeyItem {
        const wchar_t* text;
        UINT modifiers;
        UINT vk;
    };

    const HotkeyItem items[] = {
        { loc.GetString(StringID::SettingsHotkeyNone), 0, 0 },
        { L"Ctrl+Shift+E", MOD_CONTROL | MOD_SHIFT, 'E' },
        { L"Ctrl+Alt+E", MOD_CONTROL | MOD_ALT, 'E' },
        { L"Alt+F12", MOD_ALT, VK_F12 },
        { L"Ctrl+Shift+F12", MOD_CONTROL | MOD_SHIFT, VK_F12 },
        { L"Win+Pause", MOD_WIN, VK_PAUSE },
    };

    HotkeyConfig currentHotkey = m_settings->GetHotkeyConfig();
    int selectedIndex = 0;

    for (size_t i = 0; i < _countof(items); ++i) {
        int index = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0,
                                                 reinterpret_cast<LPARAM>(items[i].text)));
        LPARAM data = MAKELPARAM(items[i].modifiers, items[i].vk);
        SendMessageW(combo, CB_SETITEMDATA, index, data);

        if (currentHotkey.enabled &&
            items[i].modifiers == currentHotkey.modifiers &&
            items[i].vk == currentHotkey.virtualKey) {
            selectedIndex = index;
        }
    }

    SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::OnLanguageChanged(HWND dialog) {
    HWND languageCombo = GetDlgItem(dialog, IDC_LANGUAGE_COMBO);
    const int languageIndex = static_cast<int>(SendMessageW(languageCombo, CB_GETCURSEL, 0, 0));
    if (languageIndex < 0) {
        return;
    }

    const Language language = static_cast<Language>(
        SendMessageW(languageCombo, CB_GETITEMDATA, languageIndex, 0));
    if (language == m_settings->GetLanguage()) {
        return;
    }

    HWND keyCombo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const int keyIndex = static_cast<int>(SendMessageW(keyCombo, CB_GETCURSEL, 0, 0));
    WORD selectedKey = m_settings->GetVirtualKey();
    if (keyIndex >= 0) {
        selectedKey = static_cast<WORD>(SendMessageW(keyCombo, CB_GETITEMDATA, keyIndex, 0));
    }

    HWND hotkeyCombo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    const int hotkeyIndex = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCURSEL, 0, 0));
    LPARAM selectedHotkeyData = 0;
    if (hotkeyIndex >= 0) {
        selectedHotkeyData = SendMessageW(hotkeyCombo, CB_GETITEMDATA, hotkeyIndex, 0);
    }

    m_settings->SetLanguage(language);
    UpdateDialogText(dialog);
    PopulateKeyComboBox(keyCombo, selectedKey);
    UpdateKeyPressControlsState(dialog);
    PopulateHotkeyComboBox(dialog);

    const int hotkeyCount = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < hotkeyCount; ++i) {
        if (SendMessageW(hotkeyCombo, CB_GETITEMDATA, i, 0) == selectedHotkeyData) {
            SendMessageW(hotkeyCombo, CB_SETCURSEL, i, 0);
            break;
        }
    }
}

void SettingsDialog::OnKeyPressChanged(HWND dialog) {
    UpdateKeyPressControlsState(dialog);
}

bool SettingsDialog::OnOkClicked(HWND dialog) {
    if (!m_settings) {
        return false;
    }

    const const const const auto& loc = Localization::Instance();

    HWND keyCombo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const int keySelection = static_cast<int>(SendMessageW(keyCombo, CB_GETCURSEL, 0, 0));
    WORD virtualKey = 0;
    if (keySelection >= 0) {
        virtualKey = static_cast<WORD>(SendMessageW(keyCombo, CB_GETITEMDATA, keySelection, 0));
    }

    BOOL translated = FALSE;
    UINT period = GetDlgItemInt(dialog, IDC_PERIOD_EDIT, &translated, FALSE);
    if (virtualKey != 0 && (!translated || !m_settings->IsValidPeriod(period))) {
        MessageBoxW(dialog,
                   loc.GetString(StringID::ErrorInvalidPeriod),
                   loc.GetString(StringID::ErrorInvalidPeriodTitle),
                   MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(dialog, IDC_PERIOD_EDIT));
        return false;
    }
    if (virtualKey == 0 && (!translated || !m_settings->IsValidPeriod(period))) {
        period = m_settings->GetPeriodSec();
    }

    const bool keepDisplayOn = IsDlgButtonChecked(dialog, IDC_KEEPDISPLAY_CHECK) == BST_CHECKED;
    const bool respectBatterySaver =
        IsDlgButtonChecked(dialog, IDC_RESPECT_BATTERY_SAVER) == BST_CHECKED;
    const bool allowDisplayOnBattery =
        IsDlgButtonChecked(dialog, IDC_ALLOW_DISPLAY_BATTERY) == BST_CHECKED;
    const bool notifyOnToggle = IsDlgButtonChecked(dialog, IDC_NOTIFY_TOGGLE_CHECK) == BST_CHECKED;
    const bool autoStart = IsDlgButtonChecked(dialog, IDC_AUTOSTART_CHECK) == BST_CHECKED;

    HotkeyConfig hotkey = {};
    HWND hotkeyCombo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    const int hotkeySelection = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCURSEL, 0, 0));
    if (hotkeySelection >= 0) {
        const LPARAM data = SendMessageW(hotkeyCombo, CB_GETITEMDATA, hotkeySelection, 0);
        hotkey.modifiers = LOWORD(data);
        hotkey.virtualKey = HIWORD(data);
    }
    hotkey.enabled = hotkey.IsValid();

    TimerConfig timer = m_settings->GetTimerConfig();
    const TimerConfig oldTimer = timer;

    if (IsDlgButtonChecked(dialog, IDC_TIMER_INDEFINITE) == BST_CHECKED) {
        timer.mode = TimerMode::Indefinite;
    } else if (IsDlgButtonChecked(dialog, IDC_TIMER_DURATION) == BST_CHECKED) {
        timer.mode = TimerMode::Duration;

        BOOL translatedDuration = FALSE;
        const UINT duration = GetDlgItemInt(dialog, IDC_TIMER_DURATION_EDIT, &translatedDuration, FALSE);
        if (!translatedDuration ||
            duration < TimerConfig::MIN_DURATION_MIN ||
            duration > TimerConfig::MAX_DURATION_MIN) {
            MessageBoxW(dialog,
                       loc.GetString(StringID::ErrorInvalidTimerDuration),
                       loc.GetString(StringID::ErrorInvalidTimerTitle),
                       MB_OK | MB_ICONWARNING);
            return false;
        }
        timer.durationMinutes = duration;
    } else if (IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) == BST_CHECKED) {
        timer.mode = TimerMode::UntilTime;

        HWND timePicker = GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME);
        if (DateTime_GetSystemtime(timePicker, &timer.untilTime) != GDT_VALID) {
            GetLocalTime(&timer.untilTime);
        }

        SYSTEMTIME now = {};
        GetLocalTime(&now);
        timer.untilTime.wYear = now.wYear;
        timer.untilTime.wMonth = now.wMonth;
        timer.untilTime.wDay = now.wDay;
        timer.untilTime.wSecond = 0;
        timer.untilTime.wMilliseconds = 0;
    }

    const bool timerChanged =
        timer.mode != oldTimer.mode ||
        (timer.mode == TimerMode::Duration && timer.durationMinutes != oldTimer.durationMinutes) ||
        (timer.mode == TimerMode::UntilTime &&
         (timer.untilTime.wHour != oldTimer.untilTime.wHour ||
          timer.untilTime.wMinute != oldTimer.untilTime.wMinute));

    if (timerChanged || timer.mode == TimerMode::Indefinite) {
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

void SettingsDialog::InitializeTimerControls(HWND dialog) {
    TimerConfig timer = m_settings->GetTimerConfig();

    int radioButton = IDC_TIMER_INDEFINITE;
    switch (timer.mode) {
        case TimerMode::Indefinite:
            radioButton = IDC_TIMER_INDEFINITE;
            break;
        case TimerMode::Duration:
            radioButton = IDC_TIMER_DURATION;
            SetDlgItemInt(dialog, IDC_TIMER_DURATION_EDIT, timer.durationMinutes, FALSE);
            break;
        case TimerMode::UntilTime: {
            radioButton = IDC_TIMER_UNTIL;
            HWND timePicker = GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME);
            DateTime_SetSystemtime(timePicker, GDT_VALID, &timer.untilTime);
            break;
        }
    }
    CheckRadioButton(dialog, IDC_TIMER_INDEFINITE, IDC_TIMER_UNTIL, radioButton);
    UpdateTimerControlsState(dialog);
}

void SettingsDialog::UpdateKeyPressControlsState(HWND dialog) {
    HWND combo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const int selection = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    WORD virtualKey = 0;
    if (selection >= 0) {
        virtualKey = static_cast<WORD>(SendMessageW(combo, CB_GETITEMDATA, selection, 0));
    }
    const BOOL enabled = virtualKey != 0 ? TRUE : FALSE;
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_EDIT), enabled);
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_LABEL), enabled);
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_SECONDS_LABEL), enabled);
}

void SettingsDialog::UpdatePowerControlsState(HWND dialog) {
    const BOOL keepDisplay = IsDlgButtonChecked(dialog, IDC_KEEPDISPLAY_CHECK) == BST_CHECKED;
    EnableWindow(GetDlgItem(dialog, IDC_ALLOW_DISPLAY_BATTERY), keepDisplay);
}

void SettingsDialog::UpdateTimerControlsState(HWND dialog) {
    const BOOL duration = IsDlgButtonChecked(dialog, IDC_TIMER_DURATION) == BST_CHECKED;
    const BOOL until = IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) == BST_CHECKED;

    EnableWindow(GetDlgItem(dialog, IDC_TIMER_DURATION_EDIT), duration);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_DURATION_LABEL), duration);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME), until);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_UNTIL_HINT), until);
    UpdateUntilHint(dialog);
}

void SettingsDialog::UpdateUntilHint(HWND dialog) {
    if (IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) != BST_CHECKED) {
        SetDlgItemTextW(dialog, IDC_TIMER_UNTIL_HINT, L"");
        return;
    }

    SYSTEMTIME selected = {};
    if (DateTime_GetSystemtime(GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME), &selected) != GDT_VALID) {
        SetDlgItemTextW(dialog, IDC_TIMER_UNTIL_HINT, L"");
        return;
    }

    TimerConfig timer;
    timer.mode = TimerMode::UntilTime;
    timer.untilTime = selected;
    SetDlgItemTextW(dialog, IDC_TIMER_UNTIL_HINT,
                    timer.IsUntilNextDay()
                        ? Localization::Instance().GetString(StringID::SettingsTimerTomorrow)
                        : L"");
}

void SettingsDialog::OnTimerModeChanged(HWND dialog) {
    UpdateTimerControlsState(dialog);
}

}
