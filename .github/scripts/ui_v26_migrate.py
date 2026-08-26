from pathlib import Path
import re


def read(path):
    return Path(path).read_text(encoding="utf-8-sig")


def write(path, text):
    Path(path).write_text(text, encoding="utf-8", newline="\n")


def must_replace(text, old, new, label):
    if old not in text:
        raise RuntimeError(f"missing replacement target: {label}")
    return text.replace(old, new)


# Version 2.6.0
p = "src/Version.h"
s = read(p)
s = must_replace(s, "#define VER_MINOR 5\n#define VER_PATCH 1", "#define VER_MINOR 6\n#define VER_PATCH 0", "Version numbers")
s = must_replace(s, '#define VER_FILEVERSION_STR    "2.5.1.0"', '#define VER_FILEVERSION_STR    "2.6.0.0"', "file version")
s = must_replace(s, '#define VER_VERSION_STR        "2.5.1"', '#define VER_VERSION_STR        "2.6.0"', "version string")
s = must_replace(s, '#define VER_VERSION_STR_W      L"2.5.1"', '#define VER_VERSION_STR_W      L"2.6.0"', "wide version string")
s = must_replace(s, '#define VER_FILEVERSION_STR_W  L"2.5.1.0"', '#define VER_FILEVERSION_STR_W  L"2.6.0.0"', "wide file version")
write(p, s)

p = "CMakeLists.txt"
s = read(p)
s = must_replace(s, "project(Everon VERSION 2.5.1 LANGUAGES CXX RC)", "project(Everon VERSION 2.6.0 LANGUAGES CXX RC)", "CMake version")
write(p, s)

# Wider static dialog using Segoe UI; no runtime control measuring/repositioning.
p = "src/app.rc"
s = read(p)
new_dialog = r'''IDD_SETTINGS DIALOGEX 0, 0, 330, 300
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Everon Settings"
FONT 9, "Segoe UI", 400, 0, 0x1
BEGIN
    LTEXT           "Language:", IDC_LANGUAGE_LABEL, 12, 13, 58, 9
    COMBOBOX        IDC_LANGUAGE_COMBO, 82, 10, 236, 120, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

    GROUPBOX        "General", IDC_GENERAL_GROUP, 10, 34, 310, 98
    LTEXT           "Key press:", IDC_KEYPRESS_LABEL, 22, 51, 78, 9
    COMBOBOX        IDC_KEY_COMBO, 110, 48, 198, 90, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    LTEXT           "Period:", IDC_PERIOD_LABEL, 22, 72, 78, 9
    EDITTEXT        IDC_PERIOD_EDIT, 110, 69, 58, 15, ES_AUTOHSCROLL | ES_NUMBER
    LTEXT           "seconds", IDC_PERIOD_SECONDS_LABEL, 176, 72, 120, 9
    CONTROL         "Keep display on", IDC_KEEPDISPLAY_CHECK, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 22, 91, 286, 10
    CONTROL         "Start with Windows", IDC_AUTOSTART_CHECK, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 22, 106, 286, 10
    CONTROL         "Show notifications on Enable/Disable", IDC_NOTIFY_TOGGLE_CHECK, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 22, 121, 286, 10

    GROUPBOX        "Timer", IDC_TIMER_GROUP, 10, 139, 310, 70
    CONTROL         "Indefinitely", IDC_TIMER_INDEFINITE, "Button", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 22, 155, 120, 10
    CONTROL         "For duration:", IDC_TIMER_DURATION, "Button", BS_AUTORADIOBUTTON, 22, 172, 90, 10
    EDITTEXT        IDC_TIMER_DURATION_EDIT, 120, 169, 50, 15, ES_AUTOHSCROLL | ES_NUMBER
    LTEXT           "minutes (5-1440)", IDC_TIMER_DURATION_LABEL, 178, 172, 125, 9
    CONTROL         "Until time:", IDC_TIMER_UNTIL, "Button", BS_AUTORADIOBUTTON, 22, 190, 90, 10
    CONTROL         "", IDC_TIMER_UNTIL_TIME, "SysDateTimePick32", DTS_TIMEFORMAT | WS_TABSTOP, 120, 187, 105, 15

    GROUPBOX        "Hotkey", IDC_HOTKEYS_GROUP, 10, 216, 310, 43
    LTEXT           "Toggle hotkey:", IDC_HOTKEY_LABEL, 22, 233, 88, 9
    COMBOBOX        IDC_HOTKEY_COMBO, 120, 230, 188, 150, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

    DEFPUSHBUTTON   "OK", IDOK, 210, 275, 50, 16
    PUSHBUTTON      "Cancel", IDCANCEL, 268, 275, 50, 16
END'''
s, n = re.subn(r"IDD_SETTINGS DIALOGEX.*?\nEND\n\n//------------------------------------------------------------------------------", new_dialog + "\n\n//------------------------------------------------------------------------------", s, count=1, flags=re.S)
if n != 1:
    raise RuntimeError("failed to replace settings dialog resource")
write(p, s)

# Simplified SettingsDialog declaration.
write("src/SettingsDialog.h", r'''#pragma once

#include <windows.h>

namespace Everon {

class Settings;

class SettingsDialog {
public:
    explicit SettingsDialog(HINSTANCE instance);
    bool Show(HWND parent, Settings& settings);

private:
    static INT_PTR CALLBACK DialogProc(HWND dialog, UINT message,
                                      WPARAM wParam, LPARAM lParam);
    void OnInitDialog(HWND dialog);
    bool OnOkClicked(HWND dialog);
    void OnLanguageChanged(HWND dialog);
    void OnKeyPressChanged(HWND dialog);
    void OnTimerModeChanged(HWND dialog);
    void PopulateLanguageComboBox(HWND dialog);
    void PopulateKeyComboBox(HWND comboBox, WORD selectedKey);
    void PopulateHotkeyComboBox(HWND dialog);
    void InitializeTimerControls(HWND dialog);
    void UpdateKeyPressControlsState(HWND dialog);
    void UpdateTimerControlsState(HWND dialog);
    void UpdateDialogText(HWND dialog);

    HINSTANCE m_instance = nullptr;
    Settings* m_settings = nullptr;
};

} // namespace Everon
''')

p = "src/SettingsDialog.cpp"
s = read(p)
s, n = re.subn(r"\nnamespace \{.*?\n\} // namespace\n\nnamespace Everon \{", "\nnamespace Everon {", s, count=1, flags=re.S)
if n != 1:
    raise RuntimeError("failed to remove dynamic layout helpers")
s = s.replace("    m_baseLayoutCaptured = false;\n    m_baseLayout = {};\n\n", "")
s = must_replace(
    s,
    """                case IDC_HOTKEY_ENABLE_CHECK:\n                    instance->OnHotkeyEnableChanged(dialog);\n                    return TRUE;\n""",
    """                case IDC_KEY_COMBO:\n                    if (HIWORD(wParam) == CBN_SELCHANGE) {\n                        instance->OnKeyPressChanged(dialog);\n                    }\n                    return TRUE;\n""",
    "keypress command handler",
)
s = must_replace(
    s,
    """    PopulateKeyComboBox(GetDlgItem(dialog, IDC_KEY_COMBO), m_settings->GetVirtualKey());\n    PopulateHotkeyComboBox(dialog);\n    HotkeyConfig hotkey = m_settings->GetHotkeyConfig();\n    CheckDlgButton(dialog, IDC_HOTKEY_ENABLE_CHECK,\n                  hotkey.enabled ? BST_CHECKED : BST_UNCHECKED);\n    EnableWindow(GetDlgItem(dialog, IDC_HOTKEY_COMBO), hotkey.enabled);\n    InitializeTimerControls(dialog);\n""",
    """    PopulateKeyComboBox(GetDlgItem(dialog, IDC_KEY_COMBO), m_settings->GetVirtualKey());\n    UpdateKeyPressControlsState(dialog);\n    PopulateHotkeyComboBox(dialog);\n    InitializeTimerControls(dialog);\n""",
    "dialog init",
)

clean_update = r'''void SettingsDialog::UpdateDialogText(HWND dialog) {
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
    SetDlgItemTextW(dialog, IDC_HOTKEY_LABEL, loc.GetString(StringID::SettingsHotkeyLabel));
    SetDlgItemTextW(dialog, IDOK, loc.GetString(StringID::ButtonOK));
    SetDlgItemTextW(dialog, IDCANCEL, loc.GetString(StringID::ButtonCancel));
}'''
s, n = re.subn(r"void SettingsDialog::UpdateDialogText\(HWND dialog\) \{.*?\n\}\n\nvoid SettingsDialog::PopulateLanguageComboBox", clean_update + "\n\nvoid SettingsDialog::PopulateLanguageComboBox", s, count=1, flags=re.S)
if n != 1:
    raise RuntimeError("failed to replace UpdateDialogText")

new_language = r'''void SettingsDialog::OnLanguageChanged(HWND dialog) {
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
}'''
s, n = re.subn(r"void SettingsDialog::OnLanguageChanged\(HWND dialog\) \{.*?\n\}\n\nvoid SettingsDialog::OnHotkeyEnableChanged\(HWND dialog\) \{.*?\n\}", new_language, s, count=1, flags=re.S)
if n != 1:
    raise RuntimeError("failed to simplify language/hotkey handlers")

old_validation = r'''    BOOL translated = FALSE;
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
'''
new_validation = r'''    HWND keyCombo = GetDlgItem(dialog, IDC_KEY_COMBO);
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

    const bool keepDisplayOn = (IsDlgButtonChecked(dialog, IDC_KEEPDISPLAY_CHECK) == BST_CHECKED);
    const bool notifyOnToggle = (IsDlgButtonChecked(dialog, IDC_NOTIFY_TOGGLE_CHECK) == BST_CHECKED);
    const bool autoStart = (IsDlgButtonChecked(dialog, IDC_AUTOSTART_CHECK) == BST_CHECKED);
'''
s = must_replace(s, old_validation, new_validation, "keypress validation")

old_hotkey = r'''    HotkeyConfig hotkey = {};
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
'''
new_hotkey = r'''    HotkeyConfig hotkey = {};
    HWND hotkeyCombo = GetDlgItem(dialog, IDC_HOTKEY_COMBO);
    const int hotkeySelection = static_cast<int>(SendMessageW(hotkeyCombo, CB_GETCURSEL, 0, 0));
    if (hotkeySelection >= 0) {
        const LPARAM data = SendMessageW(hotkeyCombo, CB_GETITEMDATA, hotkeySelection, 0);
        hotkey.modifiers = LOWORD(data);
        hotkey.virtualKey = HIWORD(data);
    }
    hotkey.enabled = hotkey.IsValid();
'''
s = must_replace(s, old_hotkey, new_hotkey, "hotkey selection")

s, n = re.subn(r"void SettingsDialog::CaptureBaseLayout\(HWND dialog\) \{.*?\n\}\n\nvoid SettingsDialog::RestoreBaseLayout\(HWND dialog\) \{.*?\n\}\n\n\nvoid SettingsDialog::InitializeTimerControls", "void SettingsDialog::InitializeTimerControls", s, count=1, flags=re.S)
if n != 1:
    raise RuntimeError("failed to remove layout state methods")

key_state = r'''void SettingsDialog::UpdateKeyPressControlsState(HWND dialog) {
    HWND combo = GetDlgItem(dialog, IDC_KEY_COMBO);
    const int selection = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    WORD virtualKey = 0;
    if (selection >= 0) {
        virtualKey = static_cast<WORD>(SendMessageW(combo, CB_GETITEMDATA, selection, 0));
    }
    const BOOL enabled = (virtualKey != 0) ? TRUE : FALSE;
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_EDIT), enabled);
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_LABEL), enabled);
    EnableWindow(GetDlgItem(dialog, IDC_PERIOD_SECONDS_LABEL), enabled);
}

'''
marker = "void SettingsDialog::UpdateTimerControlsState(HWND dialog) {"
if marker not in s:
    raise RuntimeError("missing timer state marker")
s = s.replace(marker, key_state + marker, 1)
write(p, s)

# Tray icon state and interaction.
write("src/TrayIcon.h", r'''#pragma once

#include <windows.h>
#include <shellapi.h>
#include <functional>

namespace Everon {

class Settings;

class TrayIcon {
public:
    using MenuCallback = std::function<void()>;

    explicit TrayIcon(HWND parentWindow, HINSTANCE instance);
    ~TrayIcon();

    bool Add();
    bool ReAdd();
    void Remove();

    void UpdateTooltip(const Settings& settings);
    void ShowNotification(const wchar_t* title, const wchar_t* message, DWORD flags);
    void SetEnabled(bool enabled);
    void HandleMessage(LPARAM lParam);

    void SetToggleCallback(MenuCallback callback) { m_onToggle = std::move(callback); }
    void SetSettingsCallback(MenuCallback callback) { m_onSettings = std::move(callback); }
    void SetAboutCallback(MenuCallback callback) { m_onAbout = std::move(callback); }
    void SetExitCallback(MenuCallback callback) { m_onExit = std::move(callback); }

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

    bool m_isEnabled = true;
};

} // namespace Everon
''')

p = "src/TrayIcon.cpp"
s = read(p)
helper = r'''
namespace {

HICON CreateMutedIcon(HICON source, int width, int height) {
    if (!source || width <= 0 || height <= 0) {
        return nullptr;
    }

    BITMAPV5HEADER header{};
    header.bV5Size = sizeof(header);
    header.bV5Width = width;
    header.bV5Height = -height;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;

    HDC screen = GetDC(nullptr);
    if (!screen) {
        return nullptr;
    }
    void* rawBits = nullptr;
    HBITMAP color = CreateDIBSection(screen, reinterpret_cast<BITMAPINFO*>(&header),
                                     DIB_RGB_COLORS, &rawBits, nullptr, 0);
    HDC memory = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (!color || !memory || !rawBits) {
        if (memory) DeleteDC(memory);
        if (color) DeleteObject(color);
        return nullptr;
    }

    HGDIOBJ oldBitmap = SelectObject(memory, color);
    ZeroMemory(rawBits, static_cast<SIZE_T>(width) * height * 4);
    const BOOL drawn = DrawIconEx(memory, 0, 0, source, width, height, 0, nullptr, DI_NORMAL);
    SelectObject(memory, oldBitmap);
    DeleteDC(memory);
    if (!drawn) {
        DeleteObject(color);
        return nullptr;
    }

    auto* pixels = static_cast<BYTE*>(rawBits);
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    for (size_t i = 0; i < pixelCount; ++i) {
        BYTE* pixel = pixels + (i * 4);
        const BYTE gray = static_cast<BYTE>((static_cast<unsigned>(pixel[2]) * 30U +
                                             static_cast<unsigned>(pixel[1]) * 59U +
                                             static_cast<unsigned>(pixel[0]) * 11U) / 100U);
        const BYTE muted = static_cast<BYTE>((static_cast<unsigned>(gray) + 180U) / 2U);
        pixel[0] = muted;
        pixel[1] = muted;
        pixel[2] = muted;
    }

    HBITMAP mask = CreateBitmap(width, height, 1, 1, nullptr);
    if (!mask) {
        DeleteObject(color);
        return nullptr;
    }

    ICONINFO iconInfo{};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = color;
    iconInfo.hbmMask = mask;
    HICON result = CreateIconIndirect(&iconInfo);
    DeleteObject(mask);
    DeleteObject(color);
    return result;
}

} // namespace

'''
marker = "\nnamespace Everon {\n"
if marker not in s:
    raise RuntimeError("TrayIcon namespace marker missing")
s = s.replace(marker, helper + "namespace Everon {\n", 1)

old_load = r'''    m_notifyData.hIcon = static_cast<HICON>(
        LoadImageW(m_instance, MAKEINTRESOURCEW(IDI_EVERON),
                  IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR)
    );
    if (m_notifyData.hIcon) {
        m_iconOwned = true;
    } else {
        m_notifyData.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        m_iconOwned = false; // shared system icon
    }
'''
new_load = r'''    const int iconWidth = GetSystemMetrics(SM_CXSMICON);
    const int iconHeight = GetSystemMetrics(SM_CYSMICON);
    m_activeIcon = static_cast<HICON>(
        LoadImageW(m_instance, MAKEINTRESOURCEW(IDI_EVERON),
                   IMAGE_ICON, iconWidth, iconHeight, LR_DEFAULTCOLOR));
    if (m_activeIcon) {
        m_activeIconOwned = true;
    } else {
        m_activeIcon = LoadIconW(nullptr, IDI_APPLICATION);
        m_activeIconOwned = false;
    }
    m_disabledIcon = CreateMutedIcon(m_activeIcon, iconWidth, iconHeight);
    m_notifyData.hIcon = m_isEnabled && m_activeIcon
        ? m_activeIcon
        : (m_disabledIcon ? m_disabledIcon : m_activeIcon);
'''
s = must_replace(s, old_load, new_load, "tray icon loading")

old_remove = r'''        if (m_iconOwned && m_notifyData.hIcon) {
            DestroyIcon(m_notifyData.hIcon);
        }
        m_iconOwned = false;
        m_notifyData = {};
'''
new_remove = r'''        if (m_disabledIcon) {
            DestroyIcon(m_disabledIcon);
            m_disabledIcon = nullptr;
        }
        if (m_activeIconOwned && m_activeIcon) {
            DestroyIcon(m_activeIcon);
        }
        m_activeIcon = nullptr;
        m_activeIconOwned = false;
        m_notifyData = {};
'''
s = must_replace(s, old_remove, new_remove, "tray icon cleanup")

old_handle = r'''void TrayIcon::HandleMessage(LPARAM lParam) {
    const UINT mouseMsg = LOWORD(lParam);

    switch (mouseMsg) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowContextMenu();
            break;
        case WM_LBUTTONDBLCLK:
            if (m_onSettings) {
                m_onSettings();
            }
            break;
    }
}
'''
new_handle = r'''void TrayIcon::SetEnabled(bool enabled) {
    m_isEnabled = enabled;
    UpdateIcon();
}

void TrayIcon::UpdateIcon() {
    if (m_notifyData.cbSize == 0) {
        return;
    }
    HICON icon = m_isEnabled ? m_activeIcon : (m_disabledIcon ? m_disabledIcon : m_activeIcon);
    if (!icon) {
        return;
    }
    m_notifyData.hIcon = icon;
    m_notifyData.uFlags = NIF_ICON | NIF_GUID;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"update tray state icon");
}

void TrayIcon::HandleMessage(LPARAM lParam) {
    const UINT mouseMsg = LOWORD(lParam);

    switch (mouseMsg) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowContextMenu();
            break;
        case WM_LBUTTONUP:
        case NIN_SELECT:
        case NIN_KEYSELECT:
            if (m_onSettings) {
                m_onSettings();
            }
            break;
    }
}
'''
s = must_replace(s, old_handle, new_handle, "tray activation behavior")
write(p, s)

# Concise factual About dialog with exact author and project URL.
p = "src/App.cpp"
s = read(p)
about = r'''void App::ShowAbout() {
    auto& loc = Localization::Instance();
    wchar_t message[768];
    swprintf_s(message, _countof(message),
              L"%s\n%s\n\nStanley Lloyd\n%s\n\nhttps://github.com/StanleyLl0yd/everon",
              loc.GetString(StringID::AboutVersion),
              loc.GetString(StringID::AboutTagline),
              loc.GetString(StringID::AboutLicense));

    MessageBoxW(m_window, message,
               loc.GetString(StringID::MenuAbout),
               MB_OK | MB_ICONINFORMATION);
}'''
s, n = re.subn(r"void App::ShowAbout\(\) \{.*?\n\}", about, s, count=1, flags=re.S)
if n != 1:
    raise RuntimeError("failed to replace About")
write(p, s)

# README interaction details.
p = "README.md"
s = read(p)
s = s.replace("- Double-click the tray icon to open Settings\n", "- Single-click the tray icon to open Settings\n- Uses a visually muted tray icon while Everon is disabled\n")
s = s.replace("   - Double-click to open **Settings**.\n", "   - Single-click to open **Settings**.\n")
write(p, s)

p = "README.ru.md"
s = read(p)
s = s.replace("- Двойной щелчок по иконке открывает настройки\n", "- Один щелчок по иконке открывает настройки\n- При отключённом Everon иконка в трее отображается визуально приглушённой\n")
s = s.replace("   - Двойной клик — открытие **Настроек**.\n", "   - Один клик — открытие **Настроек**.\n")
write(p, s)

# Bilingual changelogs.
p = "CHANGELOG.md"
s = read(p)
entry = '''## 2.6.0 — 2026-08-26\n\n### Changed\n\n- Refreshed the native Settings dialog with a wider Segoe UI layout and removed runtime control repositioning.\n- Simplified global hotkey configuration: selecting `None` disables it; any selected hotkey enables it.\n- Disabled the key-press interval controls when synthetic key presses are turned off.\n- A single tray-icon click now opens Settings; keyboard tray activation is also supported.\n- Added a visually muted tray icon for the disabled state.\n- Simplified About to show the version, purpose, author, license, and GitHub repository.\n\n'''
if "## 2.6.0" not in s:
    insert_at = s.find("## ")
    if insert_at < 0:
        raise RuntimeError("no changelog version heading found")
    s = s[:insert_at] + entry + s[insert_at:]
write(p, s)

p = "CHANGELOG.ru.md"
s = read(p)
entry = '''## 2.6.0 — 2026-08-26\n\n### Изменено\n\n- Обновлён нативный диалог настроек: увеличена ширина, используется Segoe UI, удалено динамическое перемещение элементов управления.\n- Упрощена настройка глобальной горячей клавиши: `Нет` отключает её, выбор сочетания включает.\n- Поля интервала становятся неактивными, когда синтетическое нажатие клавиши выключено.\n- Один клик по иконке в трее теперь открывает настройки; добавлена активация иконки с клавиатуры.\n- Для отключённого состояния добавлена визуально приглушённая иконка в трее.\n- Окно «О программе» сокращено до версии, назначения, автора, лицензии и ссылки на GitHub.\n\n'''
if "## 2.6.0" not in s:
    insert_at = s.find("## ")
    if insert_at < 0:
        raise RuntimeError("no Russian changelog version heading found")
    s = s[:insert_at] + entry + s[insert_at:]
write(p, s)
