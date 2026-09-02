#include "QuickTimerDialog.h"

#include "Localization.h"
#include "TimerMode.h"
#include "Utils.h"
#include "resource.h"
#include <commctrl.h>

#include <bit>

namespace Everon {

namespace {

struct DurationDialogState {
    DWORD minutes = 60;
};

struct UntilDialogState {
    SYSTEMTIME time = {};
};

void UpdateUntilHint(HWND dialog) {
    SYSTEMTIME selected = {};
    if (DateTime_GetSystemtime(GetDlgItem(dialog, IDC_QUICK_UNTIL_TIME), &selected) != GDT_VALID) {
        SetDlgItemTextW(dialog, IDC_QUICK_UNTIL_HINT, L"");
        return;
    }

    TimerConfig timer;
    timer.mode = TimerMode::UntilTime;
    timer.untilTime = selected;
    SetDlgItemTextW(dialog, IDC_QUICK_UNTIL_HINT,
                    timer.IsUntilNextDay()
                        ? Localization::Instance().GetString(StringID::SettingsTimerTomorrow)
                        : L"");
}

INT_PTR CALLBACK DurationDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    using enum StringID;

    auto* state = std::bit_cast<DurationDialogState*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));

    if (message == WM_INITDIALOG) {
        state = std::bit_cast<DurationDialogState*>(lParam);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, lParam);
        const auto& loc = Localization::Instance();
        SetWindowTextW(dialog, loc.GetString(QuickDurationTitle));
        SetDlgItemTextW(dialog, IDC_QUICK_DURATION_LABEL, loc.GetString(QuickDurationLabel));
        SetDlgItemTextW(dialog, IDOK, loc.GetString(ButtonOK));
        SetDlgItemTextW(dialog, IDCANCEL, loc.GetString(ButtonCancel));
        SetDlgItemInt(dialog, IDC_QUICK_DURATION_EDIT, state->minutes, FALSE);
        Utils::CenterWindowOnMonitor(dialog, GetWindow(dialog, GW_OWNER));
        return TRUE;
    }

    if (!state) {
        return FALSE;
    }

    if (message == WM_COMMAND) {
        if (LOWORD(wParam) == IDOK) {
            BOOL translated = FALSE;
            const UINT minutes = GetDlgItemInt(dialog, IDC_QUICK_DURATION_EDIT, &translated, FALSE);
            if (!translated || minutes < TimerConfig::MIN_DURATION_MIN ||
                minutes > TimerConfig::MAX_DURATION_MIN) {
                const auto& loc = Localization::Instance();
                MessageBoxW(dialog,
                            loc.GetString(ErrorInvalidTimerDuration),
                            loc.GetString(ErrorInvalidTimerTitle),
                            MB_OK | MB_ICONWARNING);
                SetFocus(GetDlgItem(dialog, IDC_QUICK_DURATION_EDIT));
                return TRUE;
            }
            state->minutes = minutes;
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
    }

    return FALSE;
}

INT_PTR CALLBACK UntilDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    using enum StringID;

    auto* state = std::bit_cast<UntilDialogState*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));

    if (message == WM_INITDIALOG) {
        state = std::bit_cast<UntilDialogState*>(lParam);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, lParam);
        const auto& loc = Localization::Instance();
        SetWindowTextW(dialog, loc.GetString(QuickUntilTitle));
        SetDlgItemTextW(dialog, IDC_QUICK_UNTIL_LABEL, loc.GetString(QuickUntilLabel));
        SetDlgItemTextW(dialog, IDOK, loc.GetString(ButtonOK));
        SetDlgItemTextW(dialog, IDCANCEL, loc.GetString(ButtonCancel));
        DateTime_SetSystemtime(GetDlgItem(dialog, IDC_QUICK_UNTIL_TIME), GDT_VALID, &state->time);
        UpdateUntilHint(dialog);
        Utils::CenterWindowOnMonitor(dialog, GetWindow(dialog, GW_OWNER));
        return TRUE;
    }

    if (!state) {
        return FALSE;
    }

    if (message == WM_NOTIFY) {
        if (const auto* header = std::bit_cast<const NMHDR*>(lParam);
            header && header->idFrom == IDC_QUICK_UNTIL_TIME && header->code == DTN_DATETIMECHANGE) {
            UpdateUntilHint(dialog);
            return TRUE;
        }
    }

    if (message == WM_COMMAND) {
        if (LOWORD(wParam) == IDOK) {
            SYSTEMTIME selected = {};
            if (DateTime_GetSystemtime(GetDlgItem(dialog, IDC_QUICK_UNTIL_TIME), &selected) != GDT_VALID) {
                return TRUE;
            }
            SYSTEMTIME now = {};
            GetLocalTime(&now);
            selected.wYear = now.wYear;
            selected.wMonth = now.wMonth;
            selected.wDay = now.wDay;
            selected.wSecond = 0;
            selected.wMilliseconds = 0;
            state->time = selected;
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
    }

    return FALSE;
}

}

bool ShowQuickDurationDialog(HINSTANCE instance, HWND parent, DWORD initialMinutes, DWORD& minutes) {
    DurationDialogState state;
    state.minutes = initialMinutes;
    if (const auto result = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_QUICK_DURATION),
                                             parent, DurationDialogProc,
                                             std::bit_cast<LPARAM>(&state));
        result != IDOK) {
        return false;
    }
    minutes = state.minutes;
    return true;
}

bool ShowQuickUntilDialog(HINSTANCE instance, HWND parent, const SYSTEMTIME& initialTime, SYSTEMTIME& untilTime) {
    UntilDialogState state;
    state.time = initialTime;
    if (state.time.wYear == 0) {
        GetLocalTime(&state.time);
    }
    if (const auto result = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_QUICK_UNTIL),
                                             parent, UntilDialogProc,
                                             std::bit_cast<LPARAM>(&state));
        result != IDOK) {
        return false;
    }
    untilTime = state.time;
    return true;
}

}