#include "SettingsDialog.h"
#include "Settings.h"
#include "Utils.h"
#include "Localization.h"
#include "HotkeyManager.h"
#include "TimerMode.h"
#include "resource.h"
#include <commctrl.h>


namespace {

int GetDpiX(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        return 96;
    }
    const int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return (dpi > 0) ? dpi : 96;
}

void AdjustCheckboxWidthWithinGroup(HWND dialog, int groupId, int checkboxId) {
    HWND hGroup = GetDlgItem(dialog, groupId);
    HWND hCheck = GetDlgItem(dialog, checkboxId);
    if (!hGroup || !hCheck) {
        return;
    }

    RECT rcGroup{};
    RECT rcCheck{};
    if (!GetWindowRect(hGroup, &rcGroup) || !GetWindowRect(hCheck, &rcCheck)) {
        return;
    }

    // Convert to dialog client coordinates.
    MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&rcGroup), 2);
    MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&rcCheck), 2);

    const int dpi = GetDpiX(dialog);
    const int rightPad = MulDiv(12, dpi, 96);

    const int maxWidth = (rcGroup.right - rightPad) - rcCheck.left;
    if (maxWidth < 30) {
        return;
    }

    // Prefer the themed "ideal size" if available.
    SIZE ideal{};
    int desiredWidth = 0;
    if (SendMessageW(hCheck, BCM_GETIDEALSIZE, 0, reinterpret_cast<LPARAM>(&ideal)) != 0 && ideal.cx > 0) {
        desiredWidth = ideal.cx;
    } else {
        // Fallback: measure text and add checkbox glyph area.
        wchar_t text[256]{};
        GetWindowTextW(hCheck, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
        const int textLen = static_cast<int>(wcslen(text));

        HDC hdc = GetDC(dialog);
        if (!hdc) {
            return;
        }
        HFONT font = reinterpret_cast<HFONT>(SendMessageW(hCheck, WM_GETFONT, 0, 0));
        HGDIOBJ oldFont = nullptr;
        if (font) {
            oldFont = SelectObject(hdc, font);
        }

        SIZE sz{};
        GetTextExtentPoint32W(hdc, text, textLen, &sz);

        if (oldFont) {
            SelectObject(hdc, oldFont);
        }
        ReleaseDC(dialog, hdc);

        const int checkGlyph = GetSystemMetrics(SM_CXMENUCHECK);
        const int gap = MulDiv(10, dpi, 96);
        const int padding = MulDiv(10, dpi, 96);

        desiredWidth = checkGlyph + gap + sz.cx + padding;
    }

    const int currentWidth = rcCheck.right - rcCheck.left;
    const int newWidth = (desiredWidth > maxWidth) ? maxWidth : desiredWidth;
    if (newWidth > currentWidth) {
        SetWindowPos(hCheck, nullptr, rcCheck.left, rcCheck.top, newWidth,
                     rcCheck.bottom - rcCheck.top, SWP_NOZORDER);
    }
}

// Returns the rectangle of a child control in dialog client coordinates.
bool GetControlRectClient(HWND dialog, HWND hwnd, RECT* outRc) {
    if (!dialog || !hwnd || !outRc) {
        return false;
    }
    RECT rc{};
    if (!GetWindowRect(hwnd, &rc)) {
        return false;
    }
    MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&rc), 2);
    *outRc = rc;
    return true;
}

int GetGroupRightEdgeX(HWND dialog, int groupId, int rightPadPx) {
    HWND hGroup = GetDlgItem(dialog, groupId);
    if (!hGroup) {
        return 0;
    }
    RECT rcGroup{};
    if (!GetControlRectClient(dialog, hGroup, &rcGroup)) {
        return 0;
    }
    return rcGroup.right - rightPadPx;
}

int MeasureControlTextWidth(HWND dialog, HWND hwnd) {
    if (!dialog || !hwnd) {
        return 0;
    }
    wchar_t text[256]{};
    GetWindowTextW(hwnd, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    const int textLen = static_cast<int>(wcslen(text));
    if (textLen <= 0) {
        return 0;
    }

    HDC hdc = GetDC(dialog);
    if (!hdc) {
        return 0;
    }

    HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = nullptr;
    if (font) {
        oldFont = SelectObject(hdc, font);
    }

    SIZE sz{};
    GetTextExtentPoint32W(hdc, text, textLen, &sz);

    if (oldFont) {
        SelectObject(hdc, oldFont);
    }
    ReleaseDC(dialog, hdc);

    return sz.cx;
}

int GetIdealButtonWidth(HWND dialog, HWND hBtn) {
    if (!dialog || !hBtn) {
        return 0;
    }

    // Preferred: themed ideal size.
    SIZE ideal{};
    if (SendMessageW(hBtn, BCM_GETIDEALSIZE, 0, reinterpret_cast<LPARAM>(&ideal)) != 0 && ideal.cx > 0) {
        return ideal.cx;
    }

    // Fallback: measure text and add a rough glyph + padding.
    const int textW = MeasureControlTextWidth(dialog, hBtn);
    if (textW <= 0) {
        return 0;
    }

    const int dpi = GetDpiX(dialog);
    const int glyph = GetSystemMetrics(SM_CXMENUCHECK);
    const int gap = MulDiv(10, dpi, 96);
    const int padding = MulDiv(10, dpi, 96);

    return glyph + gap + textW + padding;
}

// Fits a label+control row while keeping the control aligned to the given right edge.
void AdjustLabelAndControlRowToRightEdge(HWND dialog, int labelId, int ctrlId, int rightEdgeX) {
    HWND hLabel = GetDlgItem(dialog, labelId);
    HWND hCtrl = GetDlgItem(dialog, ctrlId);
    if (!hLabel || !hCtrl) {
        return;
    }

    RECT rcLabel{};
    RECT rcCtrl{};
    if (!GetControlRectClient(dialog, hLabel, &rcLabel) || !GetControlRectClient(dialog, hCtrl, &rcCtrl)) {
        return;
    }

    const int labelLeft = rcLabel.left;
    const int labelTop = rcLabel.top;
    const int labelHeight = rcLabel.bottom - rcLabel.top;

    const int ctrlTop = rcCtrl.top;
    const int ctrlHeight = rcCtrl.bottom - rcCtrl.top;

    const int paddingPx = 6;
    const int gapPx = 8;
    const int minCtrlPx = 90;

    const int desiredLabelW = MeasureControlTextWidth(dialog, hLabel) + paddingPx;

    int maxLabelW = rightEdgeX - labelLeft - gapPx - minCtrlPx;
    if (maxLabelW < 30) {
        return;
    }

    int newLabelW = desiredLabelW;
    if (newLabelW > maxLabelW) {
        newLabelW = maxLabelW;
    }

    int newCtrlLeft = labelLeft + newLabelW + gapPx;
    int newCtrlW = rightEdgeX - newCtrlLeft;

    if (newCtrlW < minCtrlPx) {
        newCtrlW = minCtrlPx;
        newCtrlLeft = rightEdgeX - newCtrlW;
        newLabelW = newCtrlLeft - gapPx - labelLeft;
        if (newLabelW < 30) {
            return;
        }
    }

    SetWindowPos(hLabel, nullptr, labelLeft, labelTop, newLabelW, labelHeight, SWP_NOZORDER);
    SetWindowPos(hCtrl, nullptr, newCtrlLeft, ctrlTop, newCtrlW, ctrlHeight, SWP_NOZORDER);
}

void AdjustLabelAndControlRowToGroupRight(HWND dialog, int groupId, int labelId, int ctrlId) {
    const int dpi = GetDpiX(dialog);
    const int rightPad = MulDiv(12, dpi, 96);
    const int rightEdge = GetGroupRightEdgeX(dialog, groupId, rightPad);
    if (rightEdge <= 0) {
        return;
    }
    AdjustLabelAndControlRowToRightEdge(dialog, labelId, ctrlId, rightEdge);
}

void AdjustCheckboxWidthToRightEdge(HWND dialog, int checkboxId, int rightEdgeX) {
    HWND hCheck = GetDlgItem(dialog, checkboxId);
    if (!hCheck) {
        return;
    }
    RECT rcCheck{};
    if (!GetControlRectClient(dialog, hCheck, &rcCheck)) {
        return;
    }

    const int maxWidth = rightEdgeX - rcCheck.left;
    if (maxWidth < 30) {
        return;
    }

    const int idealW = GetIdealButtonWidth(dialog, hCheck);
    const int desiredW = (idealW > 0) ? idealW : (rcCheck.right - rcCheck.left);
    const int newW = (desiredW > maxWidth) ? maxWidth : desiredW;

    const int currentW = rcCheck.right - rcCheck.left;
    if (newW > currentW) {
        SetWindowPos(hCheck, nullptr, rcCheck.left, rcCheck.top, newW, rcCheck.bottom - rcCheck.top, SWP_NOZORDER);
    }
}

// Expand two checkboxes on the same row without overlapping (left checkbox grows up to the right checkbox).
void AdjustCheckboxPairWithinGroupRow(HWND dialog, int groupId, int checkAId, int checkBId) {
    HWND hA = GetDlgItem(dialog, checkAId);
    HWND hB = GetDlgItem(dialog, checkBId);
    if (!hA || !hB) {
        return;
    }

    RECT rcA{};
    RECT rcB{};
    if (!GetControlRectClient(dialog, hA, &rcA) || !GetControlRectClient(dialog, hB, &rcB)) {
        return;
    }

    // Ensure A is the left one.
    int leftId = checkAId;
    int rightId = checkBId;
    RECT rcLeft = rcA;
    if (rcB.left < rcA.left) {
        leftId = checkBId;
        rightId = checkAId;
        rcLeft = rcB;
    }

    const int dpi = GetDpiX(dialog);
    const int rightPad = MulDiv(12, dpi, 96);
    const int gapPx = MulDiv(10, dpi, 96);

    const int groupRight = GetGroupRightEdgeX(dialog, groupId, rightPad);
    if (groupRight <= 0) {
        return;
    }

    // Right checkbox can expand to the group right edge.
    AdjustCheckboxWidthToRightEdge(dialog, rightId, groupRight);

    // Left checkbox can expand up to the start of the right checkbox minus a small gap.
    // Re-read right rect because it may have moved/changed width.
    HWND hRight = GetDlgItem(dialog, rightId);
    if (!hRight) {
        return;
    }
    RECT rcRightNow{};
    if (!GetControlRectClient(dialog, hRight, &rcRightNow)) {
        return;
    }
    const int leftMaxRight = rcRightNow.left - gapPx;
    if (leftMaxRight > rcLeft.left + 30) {
        AdjustCheckboxWidthToRightEdge(dialog, leftId, leftMaxRight);
    }
}

void AdjustStaticWidthToGroupRight(HWND dialog, int groupId, int staticId) {
    HWND hStatic = GetDlgItem(dialog, staticId);
    if (!hStatic) {
        return;
    }
    RECT rc{};
    if (!GetControlRectClient(dialog, hStatic, &rc)) {
        return;
    }

    const int dpi = GetDpiX(dialog);
    const int rightPad = MulDiv(12, dpi, 96);
    const int groupRight = GetGroupRightEdgeX(dialog, groupId, rightPad);
    if (groupRight <= 0) {
        return;
    }

    const int newW = groupRight - rc.left;
    const int currentW = rc.right - rc.left;
    if (newW > currentW) {
        SetWindowPos(hStatic, nullptr, rc.left, rc.top, newW, rc.bottom - rc.top, SWP_NOZORDER);
    }
}

int GetMaxRightX(HWND dialog, const int* ids, int count) {
    int maxRight = 0;
    for (int i = 0; i < count; ++i) {
        HWND h = GetDlgItem(dialog, ids[i]);
        if (!h) {
            continue;
        }
        RECT rc{};
        if (!GetControlRectClient(dialog, h, &rc)) {
            continue;
        }
        if (rc.right > maxRight) {
            maxRight = rc.right;
        }
    }
    return maxRight;
}

void ShiftControlsX(HWND dialog, const int* ids, int count, int deltaX) {
    if (deltaX == 0) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        HWND h = GetDlgItem(dialog, ids[i]);
        if (!h) {
            continue;
        }
        RECT rc{};
        if (!GetControlRectClient(dialog, h, &rc)) {
            continue;
        }
        SetWindowPos(h, nullptr, rc.left + deltaX, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }
}

void AdjustRadioToGroupRight(HWND dialog, int groupId, int radioId) {
    HWND hRadio = GetDlgItem(dialog, radioId);
    if (!hRadio) {
        return;
    }

    RECT rcRadio{};
    if (!GetControlRectClient(dialog, hRadio, &rcRadio)) {
        return;
    }

    const int dpi = GetDpiX(dialog);
    const int rightPad = MulDiv(12, dpi, 96);
    const int groupRight = GetGroupRightEdgeX(dialog, groupId, rightPad);
    if (groupRight <= 0) {
        return;
    }

    const int maxWidth = groupRight - rcRadio.left;
    if (maxWidth < 30) {
        return;
    }

    const int idealW = GetIdealButtonWidth(dialog, hRadio);
    const int desiredW = (idealW > 0) ? idealW : (rcRadio.right - rcRadio.left);
    const int newW = (desiredW > maxWidth) ? maxWidth : desiredW;

    const int currentW = rcRadio.right - rcRadio.left;
    if (newW > currentW) {
        SetWindowPos(hRadio, nullptr, rcRadio.left, rcRadio.top, newW, rcRadio.bottom - rcRadio.top, SWP_NOZORDER);
    }
}

// Expand a radio button label. If needed, shift the controls to the right on the same row to make room (within the group).
void AdjustRadioBeforeControls(HWND dialog, int groupId, int radioId, int firstRightControlId,
                              const int* moveIds, int moveCount) {
    HWND hRadio = GetDlgItem(dialog, radioId);
    HWND hNext = GetDlgItem(dialog, firstRightControlId);
    if (!hRadio || !hNext) {
        return;
    }

    RECT rcRadio{};
    RECT rcNext{};
    if (!GetControlRectClient(dialog, hRadio, &rcRadio) || !GetControlRectClient(dialog, hNext, &rcNext)) {
        return;
    }

    const int dpi = GetDpiX(dialog);
    const int rightPad = MulDiv(12, dpi, 96);
    const int gapPx = MulDiv(8, dpi, 96);

    const int groupRight = GetGroupRightEdgeX(dialog, groupId, rightPad);
    if (groupRight <= 0) {
        return;
    }

    const int maxPossibleW = (groupRight - rcRadio.left);
    if (maxPossibleW < 30) {
        return;
    }

    const int idealW = GetIdealButtonWidth(dialog, hRadio);
    int desiredW = (idealW > 0) ? idealW : (rcRadio.right - rcRadio.left);
    if (desiredW > maxPossibleW) {
        desiredW = maxPossibleW;
    }

    int availableW = (rcNext.left - gapPx) - rcRadio.left;
    if (availableW < 30) {
        availableW = 30;
    }

    if (desiredW > availableW && moveIds && moveCount > 0) {
        const int maxRightNow = GetMaxRightX(dialog, moveIds, moveCount);
        const int maxShift = (groupRight - maxRightNow);
        const int needed = desiredW - availableW;
        const int shift = (needed < maxShift) ? needed : maxShift;
        if (shift > 0) {
            ShiftControlsX(dialog, moveIds, moveCount, shift);
            // Next control moved, so we can re-read it.
            if (GetControlRectClient(dialog, hNext, &rcNext)) {
                availableW = (rcNext.left - gapPx) - rcRadio.left;
            }
        }
    }

    const int newW = (desiredW > availableW) ? availableW : desiredW;
    const int currentW = rcRadio.right - rcRadio.left;
    if (newW > currentW) {
        SetWindowPos(hRadio, nullptr, rcRadio.left, rcRadio.top, newW, rcRadio.bottom - rcRadio.top, SWP_NOZORDER);
    }
}


} // namespace

namespace Everon {

SettingsDialog::SettingsDialog(HINSTANCE instance)
    : m_instance(instance) {
}

bool SettingsDialog::Show(HWND parent, Settings& settings) {
    m_settings = &settings;
    m_baseLayoutCaptured = false;
    m_baseLayout = {};

    // Allow live language preview inside the dialog, but revert it if user clicks Cancel.
    const Language oldLang = m_settings->GetLanguage();
    const bool oldDirty = m_settings->IsDirty();

    INT_PTR result = DialogBoxParamW(m_instance, MAKEINTRESOURCEW(IDD_SETTINGS),
                                    parent, DialogProc, reinterpret_cast<LPARAM>(this));

    if (result != IDOK) {
        m_settings->SetLanguage(oldLang);
        m_settings->SetDirty(oldDirty);
    }

    m_settings = nullptr;
    return (result == IDOK);
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
                case IDC_HOTKEY_ENABLE_CHECK:
                    instance->OnHotkeyEnableChanged(dialog);
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
    }

    return FALSE;
}

void SettingsDialog::OnInitDialog(HWND dialog) {
    if (!m_settings) return;
    SetDlgItemInt(dialog, IDC_PERIOD_EDIT, m_settings->GetPeriodSec(), FALSE);
    CheckDlgButton(dialog, IDC_KEEPDISPLAY_CHECK,
                  m_settings->GetKeepDisplayOn() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_NOTIFY_TOGGLE_CHECK,
                  m_settings->GetShowToggleNotifications() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_AUTOSTART_CHECK,
                  m_settings->GetAutoStart() ? BST_CHECKED : BST_UNCHECKED);
    PopulateLanguageComboBox(dialog);
    PopulateKeyComboBox(GetDlgItem(dialog, IDC_KEY_COMBO), m_settings->GetVirtualKey());
    PopulateHotkeyComboBox(dialog);
    HotkeyConfig hotkey = m_settings->GetHotkeyConfig();
    CheckDlgButton(dialog, IDC_HOTKEY_ENABLE_CHECK,
                  hotkey.enabled ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(dialog, IDC_HOTKEY_COMBO), hotkey.enabled);
    InitializeTimerControls(dialog);
    UpdateDialogText(dialog);
    Utils::CenterWindowOnMonitor(dialog, GetWindow(dialog, GW_OWNER));
}

void SettingsDialog::UpdateDialogText(HWND dialog) {
    if (!m_baseLayoutCaptured) {
        CaptureBaseLayout(dialog);
    } else {
        RestoreBaseLayout(dialog);
    }

    auto& loc = Localization::Instance();
    SetWindowTextW(dialog, loc.GetString(StringID::SettingsTitle));
    SetDlgItemTextW(dialog, IDC_LANGUAGE_LABEL, loc.GetString(StringID::SettingsLanguage));
    SetDlgItemTextW(dialog, IDC_GENERAL_GROUP, loc.GetString(StringID::SettingsGeneral));
    SetDlgItemTextW(dialog, IDC_PERIOD_LABEL, loc.GetString(StringID::SettingsPeriod));
    SetDlgItemTextW(dialog, IDC_PERIOD_SECONDS_LABEL, loc.GetString(StringID::SettingsPeriodSeconds));
    SetDlgItemTextW(dialog, IDC_KEYPRESS_LABEL, loc.GetString(StringID::SettingsKeyPress));
    SetDlgItemTextW(dialog, IDC_KEEPDISPLAY_CHECK, loc.GetString(StringID::SettingsKeepDisplay));
    SetDlgItemTextW(dialog, IDC_NOTIFY_TOGGLE_CHECK, loc.GetString(StringID::SettingsNotifyOnToggle));
    SetDlgItemTextW(dialog, IDC_AUTOSTART_CHECK, loc.GetString(StringID::SettingsAutoStart));
    SetDlgItemTextW(dialog, IDC_TIMER_GROUP, loc.GetString(StringID::SettingsTimer));
    SetDlgItemTextW(dialog, IDC_TIMER_INDEFINITE, loc.GetString(StringID::SettingsTimerIndefinite));
    SetDlgItemTextW(dialog, IDC_TIMER_DURATION, loc.GetString(StringID::SettingsTimerDuration));
    SetDlgItemTextW(dialog, IDC_TIMER_DURATION_LABEL, loc.GetString(StringID::SettingsTimerMinutes));
    SetDlgItemTextW(dialog, IDC_TIMER_UNTIL, loc.GetString(StringID::SettingsTimerUntilTime));
    SetDlgItemTextW(dialog, IDC_HOTKEYS_GROUP, loc.GetString(StringID::SettingsHotkeys));
    SetDlgItemTextW(dialog, IDC_HOTKEY_ENABLE_CHECK, loc.GetString(StringID::SettingsHotkeyEnable));
    SetDlgItemTextW(dialog, IDC_HOTKEY_LABEL, loc.GetString(StringID::SettingsHotkeyLabel));
    SetDlgItemTextW(dialog, IDOK, loc.GetString(StringID::ButtonOK));
    SetDlgItemTextW(dialog, IDCANCEL, loc.GetString(StringID::ButtonCancel));
    // Auto-fit potentially long localized labels/checkboxes inside fixed-size dialog resources.
    // 1) Key press row: Russian and some EU languages are longer than the original template.
    AdjustLabelAndControlRowToGroupRight(dialog, IDC_GENERAL_GROUP, IDC_KEYPRESS_LABEL, IDC_KEY_COMBO);

    // 2) General checkboxes share one row; expand each without overlapping.
    AdjustCheckboxPairWithinGroupRow(dialog, IDC_GENERAL_GROUP, IDC_KEEPDISPLAY_CHECK, IDC_AUTOSTART_CHECK);
    AdjustCheckboxWidthWithinGroup(dialog, IDC_GENERAL_GROUP, IDC_NOTIFY_TOGGLE_CHECK);

    // 3) Timer radio buttons: expand labels and, if needed, shift the right-side controls a bit (within the group).
    AdjustRadioToGroupRight(dialog, IDC_TIMER_GROUP, IDC_TIMER_INDEFINITE);
    const int durationMove[] = { IDC_TIMER_DURATION_EDIT, IDC_TIMER_DURATION_LABEL };
    AdjustRadioBeforeControls(dialog, IDC_TIMER_GROUP, IDC_TIMER_DURATION, IDC_TIMER_DURATION_EDIT,
                             durationMove, _countof(durationMove));
    const int untilMove[] = { IDC_TIMER_UNTIL_TIME };
    AdjustRadioBeforeControls(dialog, IDC_TIMER_GROUP, IDC_TIMER_UNTIL, IDC_TIMER_UNTIL_TIME,
                             untilMove, _countof(untilMove));
    AdjustStaticWidthToGroupRight(dialog, IDC_TIMER_GROUP, IDC_TIMER_DURATION_LABEL);

    // 4) Hotkey row and checkbox.
    AdjustLabelAndControlRowToGroupRight(dialog, IDC_HOTKEYS_GROUP, IDC_HOTKEY_LABEL, IDC_HOTKEY_COMBO);
    AdjustCheckboxWidthWithinGroup(dialog, IDC_HOTKEYS_GROUP, IDC_HOTKEY_ENABLE_CHECK);
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

    auto& loc = Localization::Instance();

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

    auto& loc = Localization::Instance();

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

        if (items[i].modifiers == currentHotkey.modifiers &&
            items[i].vk == currentHotkey.virtualKey) {
            selectedIndex = index;
        }
    }

    SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
}

void SettingsDialog::OnLanguageChanged(HWND dialog) {
    HWND combo = GetDlgItem(dialog, IDC_LANGUAGE_COMBO);
    int index = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));

    if (index >= 0) {
        Language lang = static_cast<Language>(
            SendMessageW(combo, CB_GETITEMDATA, index, 0)
        );

        if (lang == m_settings->GetLanguage()) {
            return;
        }

        m_settings->SetLanguage(lang);
        UpdateDialogText(dialog);
        PopulateKeyComboBox(GetDlgItem(dialog, IDC_KEY_COMBO), m_settings->GetVirtualKey());
        PopulateHotkeyComboBox(dialog);

        HotkeyConfig hotkey = m_settings->GetHotkeyConfig();
        CheckDlgButton(dialog, IDC_HOTKEY_ENABLE_CHECK,
                      hotkey.enabled ? BST_CHECKED : BST_UNCHECKED);
    }
}

void SettingsDialog::OnHotkeyEnableChanged(HWND dialog) {
    BOOL enabled = (IsDlgButtonChecked(dialog, IDC_HOTKEY_ENABLE_CHECK) == BST_CHECKED);
    EnableWindow(GetDlgItem(dialog, IDC_HOTKEY_COMBO), enabled);
}

bool SettingsDialog::OnOkClicked(HWND dialog) {
    if (!m_settings) return false;

    auto& loc = Localization::Instance();

    BOOL translated = FALSE;
    UINT period = GetDlgItemInt(dialog, IDC_PERIOD_EDIT, &translated, FALSE);

    if (!translated || !m_settings->IsValidPeriod(period)) {
        MessageBoxW(dialog,
                   loc.GetString(StringID::ErrorInvalidPeriod),
                   loc.GetString(StringID::ErrorInvalidPeriodTitle),
                   MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(dialog, IDC_PERIOD_EDIT));
        return false;
    }

    const bool keepDisplayOn = (IsDlgButtonChecked(dialog, IDC_KEEPDISPLAY_CHECK) == BST_CHECKED);
    const bool notifyOnToggle = (IsDlgButtonChecked(dialog, IDC_NOTIFY_TOGGLE_CHECK) == BST_CHECKED);
    const bool autoStart = (IsDlgButtonChecked(dialog, IDC_AUTOSTART_CHECK) == BST_CHECKED);

    HWND keyCombo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const int keySelection = static_cast<int>(SendMessageW(keyCombo, CB_GETCURSEL, 0, 0));
    WORD virtualKey = 0;
    if (keySelection >= 0) {
        virtualKey = static_cast<WORD>(SendMessageW(keyCombo, CB_GETITEMDATA, keySelection, 0));
    }

    HotkeyConfig hotkey = {};
    hotkey.enabled = (IsDlgButtonChecked(dialog, IDC_HOTKEY_ENABLE_CHECK) == BST_CHECKED);

    HWND hotkeyCombo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    const int hotkeySelection = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCURSEL, 0, 0));
    if (hotkeySelection >= 0) {
        const LPARAM data = SendMessageW(hotkeyCombo, CB_GETITEMDATA, hotkeySelection, 0);
        hotkey.modifiers = LOWORD(data);
        hotkey.virtualKey = HIWORD(data);
    }

    // If enabled but "None" is selected, silently disable to avoid confusing state.
    if (hotkey.enabled && !hotkey.IsValid()) {
        hotkey = {};
    }

    // Timer configuration
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

        // Keep date fields sane for UI/registry, but only time-of-day is used by the timer logic.
        SYSTEMTIME now = {};
        GetLocalTime(&now);
        timer.untilTime.wYear = now.wYear;
        timer.untilTime.wMonth = now.wMonth;
        timer.untilTime.wDay = now.wDay;
        timer.untilTime.wSecond = 0;
        timer.untilTime.wMilliseconds = 0;
    }
    // If timer configuration changed, drop runtime state so it can be re-initialized when needed.
    const bool timerChanged =
        (timer.mode != oldTimer.mode) ||
        (timer.mode == TimerMode::Duration && timer.durationMinutes != oldTimer.durationMinutes) ||
        (timer.mode == TimerMode::UntilTime &&
            (timer.untilTime.wHour != oldTimer.untilTime.wHour || timer.untilTime.wMinute != oldTimer.untilTime.wMinute));

    if (timerChanged || timer.mode == TimerMode::Indefinite) {
        timer.startTime = {};
        timer.endTimeUtc = 0;
    }


    m_settings->SetPeriodSec(period);
    m_settings->SetVirtualKey(virtualKey);
    m_settings->SetKeepDisplayOn(keepDisplayOn);
    m_settings->SetShowToggleNotifications(notifyOnToggle);
    m_settings->SetAutoStart(autoStart);
    m_settings->SetHotkeyConfig(hotkey);
    m_settings->SetTimerConfig(timer);

    if (!Settings::SetAutoStartEnabled(autoStart)) {
        MessageBoxW(dialog,
                   loc.GetString(StringID::ErrorAutoStart),
                   loc.GetString(StringID::ErrorTitle),
                   MB_OK | MB_ICONWARNING);
        // Reflect actual state
        const bool actual = Settings::IsAutoStartEnabled();
        m_settings->SetAutoStart(actual);
    }

    if (!m_settings->SaveToRegistry()) {
        MessageBoxW(dialog,
                   loc.GetString(StringID::ErrorSaveSettings),
                   loc.GetString(StringID::ErrorTitle),
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    return true;
}

void SettingsDialog::CaptureBaseLayout(HWND dialog) {
    const int trackedIds[TRACKED_LAYOUT_SLOTS] = {
        IDC_KEYPRESS_LABEL,
        IDC_KEY_COMBO,
        IDC_KEEPDISPLAY_CHECK,
        IDC_NOTIFY_TOGGLE_CHECK,
        IDC_AUTOSTART_CHECK,
        IDC_TIMER_INDEFINITE,
        IDC_TIMER_DURATION,
        IDC_TIMER_DURATION_EDIT,
        IDC_TIMER_DURATION_LABEL,
        IDC_TIMER_UNTIL,
        IDC_TIMER_UNTIL_TIME,
        IDC_HOTKEY_LABEL,
        IDC_HOTKEY_COMBO,
        IDC_HOTKEY_ENABLE_CHECK
    };

    for (size_t i = 0; i < TRACKED_LAYOUT_SLOTS; ++i) {
        m_baseLayout[i].controlId = trackedIds[i];
        m_baseLayout[i].rect = {};
        m_baseLayout[i].valid = false;

        HWND hwnd = GetDlgItem(dialog, trackedIds[i]);
        if (!hwnd) {
            continue;
        }

        RECT rc{};
        if (GetControlRectClient(dialog, hwnd, &rc)) {
            m_baseLayout[i].rect = rc;
            m_baseLayout[i].valid = true;
        }
    }

    m_baseLayoutCaptured = true;
}

void SettingsDialog::RestoreBaseLayout(HWND dialog) {
    if (!m_baseLayoutCaptured) {
        return;
    }

    for (size_t i = 0; i < TRACKED_LAYOUT_SLOTS; ++i) {
        const LayoutSlot& slot = m_baseLayout[i];
        if (!slot.valid || slot.controlId == 0) {
            continue;
        }

        HWND hwnd = GetDlgItem(dialog, slot.controlId);
        if (!hwnd) {
            continue;
        }

        const int width = slot.rect.right - slot.rect.left;
        const int height = slot.rect.bottom - slot.rect.top;
        if (width <= 0 || height <= 0) {
            continue;
        }

        SetWindowPos(hwnd, nullptr, slot.rect.left, slot.rect.top, width, height, SWP_NOZORDER);
    }
}


void SettingsDialog::InitializeTimerControls(HWND dialog) {
    TimerConfig timer = m_settings->GetTimerConfig();

    // Set radio button
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

void SettingsDialog::UpdateTimerControlsState(HWND dialog) {
    BOOL duration = (IsDlgButtonChecked(dialog, IDC_TIMER_DURATION) == BST_CHECKED);
    BOOL until = (IsDlgButtonChecked(dialog, IDC_TIMER_UNTIL) == BST_CHECKED);

    EnableWindow(GetDlgItem(dialog, IDC_TIMER_DURATION_EDIT), duration);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_DURATION_LABEL), duration);
    EnableWindow(GetDlgItem(dialog, IDC_TIMER_UNTIL_TIME), until);
}

void SettingsDialog::OnTimerModeChanged(HWND dialog) {
    UpdateTimerControlsState(dialog);
}

} // namespace Everon
