#include "TrayIcon.h"
#include "Settings.h"
#include "Utils.h"
#include "Localization.h"
#include "TimerMode.h"
#include "resource.h"
#include <strsafe.h>

// Some SDKs may not define these flags, but Windows 7+ supports them.
#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP 0x00000080
#endif
#ifndef NIF_GUID
#define NIF_GUID 0x00000020
#endif


namespace Everon {


TrayIcon::TrayIcon(HWND parentWindow, HINSTANCE instance)
    : m_parentWindow(parentWindow)
    , m_instance(instance) {
}

TrayIcon::~TrayIcon() {
    Remove();
}

bool TrayIcon::Add() {
    m_notifyData = {};
    // Windows 7+ supports the modern NOTIFYICONDATA size.
    m_notifyData.cbSize = sizeof(NOTIFYICONDATAW);
    m_notifyData.hWnd = m_parentWindow;
    m_notifyData.uID = 1;
    // Opt-in to standard tooltip behavior for NOTIFYICON_VERSION_4.
    m_notifyData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    m_notifyData.uCallbackMessage = WM_TRAYICON;

    // Stable identity is recommended on Windows 7+.
    // This also helps the shell keep icon state consistent across restarts.
    static const GUID kTrayGuid =
        {0x8b5e6f7a, 0x6d8a, 0x4a0c, {0x9d, 0x2e, 0x4f, 0x7d, 0x7a, 0x51, 0x1c, 0x10}};
    m_notifyData.guidItem = kTrayGuid;

    m_notifyData.hIcon = static_cast<HICON>(
        LoadImageW(m_instance, MAKEINTRESOURCEW(IDI_EVERON),
                  IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR)
    );
    if (m_notifyData.hIcon) {
        m_iconOwned = true;
    } else {
        m_notifyData.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        m_iconOwned = false; // shared system icon
    }

    StringCchCopyW(m_notifyData.szTip, _countof(m_notifyData.szTip), L"Everon");

    if (!Utils::ShellNotifyIconChecked(NIM_ADD, &m_notifyData, L"add tray icon")) {
        Utils::DebugLog(L"[Everon] Failed to add tray icon: %lu\n", GetLastError());
        return false;
    }

    // Try modern behavior; if it fails, fall back to legacy.
    m_notifyData.uVersion = NOTIFYICON_VERSION_4;
    // For NIM_SETVERSION, uFlags are ignored but keeping it clean avoids edge cases.
    const UINT savedFlags = m_notifyData.uFlags;
    m_notifyData.uFlags = 0;
    if (!Utils::ShellNotifyIconChecked(NIM_SETVERSION, &m_notifyData, L"set tray icon v4")) {
        m_notifyData.uVersion = NOTIFYICON_VERSION;
        Utils::ShellNotifyIconChecked(NIM_SETVERSION, &m_notifyData, L"set tray icon legacy version");
    }
    m_notifyData.uFlags = savedFlags;
    // Re-apply tooltip after setting version (some shells ignore the tip set during NIM_ADD).
    // IMPORTANT: When using NIF_GUID identity, include NIF_GUID for NIM_MODIFY; otherwise the modify
    // may silently target nothing and the shell keeps the original (often non-informative) tooltip.
    m_notifyData.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"apply tray tooltip after add");
    m_notifyData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;


    return true;
}

bool TrayIcon::ReAdd() {
    Remove();
    return Add();
}

void TrayIcon::Remove() {
    if (m_notifyData.cbSize > 0) {
        Utils::ShellNotifyIconChecked(NIM_DELETE, &m_notifyData, L"remove tray icon");
        if (m_iconOwned && m_notifyData.hIcon) {
            DestroyIcon(m_notifyData.hIcon);
        }
        m_iconOwned = false;
        m_notifyData = {};
    }
}

void TrayIcon::UpdateTooltip(const Settings& settings) {
    if (m_notifyData.cbSize == 0) {
        return;
    }

    auto& loc = Localization::Instance();
    wchar_t tooltip[128] = {};
    StringCchCopyW(tooltip, _countof(tooltip),
                   settings.IsEnabled() ? loc.GetString(StringID::TooltipEnabled)
                                       : loc.GetString(StringID::TooltipDisabled));

    auto AppendBullet = [&](const wchar_t* text) {
        if (!text || !*text) {
            return;
        }
        wchar_t tmp[128] = {};
        StringCchPrintfW(tmp, _countof(tmp), L" \x2022 %s", text);
        StringCchCatW(tooltip, _countof(tooltip), tmp);
    };

    if (settings.IsEnabled()) {
        const bool isRu = (loc.GetLanguage() == Language::Russian);
        const wchar_t secUnit = isRu ? L'\x0441' : L's'; // 'с' or 's'
        const wchar_t minUnit = isRu ? L'\x043C' : L'm'; // 'м' or 'm'
        const wchar_t hourUnit = isRu ? L'\x0447' : L'h'; // 'ч' or 'h'

        // Optional keypress info (omit if no key is configured)
        const WORD vk = settings.GetVirtualKey();
        const DWORD period = settings.GetPeriodSec();
        if (vk != 0 && period > 0) {
            const std::wstring keyName = Utils::GetKeyName(vk);
            wchar_t part[64] = {};
            StringCchPrintfW(part, _countof(part), L"%s/%lu%c", keyName.c_str(),
                             static_cast<unsigned long>(period), secUnit);
            AppendBullet(part);
        }

        // Keep display on (optional)
        if (settings.GetKeepDisplayOn()) {
            AppendBullet(loc.GetString(StringID::SettingsKeepDisplay));
        }

        // Timer info (optional)
        TimerConfig timer = settings.GetTimerConfig();
        if (timer.mode == TimerMode::Duration) {
            DWORD remaining = timer.GetRemainingSeconds();
            if (remaining != INFINITE && remaining > 0) {
                DWORD minutes = remaining / 60;
                DWORD hours = minutes / 60;
                minutes = minutes % 60;

                wchar_t part[64] = {};
                if (hours > 0) {
                    StringCchPrintfW(part, _countof(part), L"%s %lu%c %02lu%c",
                                     loc.GetString(StringID::SettingsTimer),
                                     static_cast<unsigned long>(hours), hourUnit,
                                     static_cast<unsigned long>(minutes), minUnit);
                } else {
                    StringCchPrintfW(part, _countof(part), L"%s %lu%c",
                                     loc.GetString(StringID::SettingsTimer),
                                     static_cast<unsigned long>(minutes), minUnit);
                }
                AppendBullet(part);
            }
        } else if (timer.mode == TimerMode::UntilTime) {
            wchar_t part[64] = {};
            StringCchPrintfW(part, _countof(part), L"%s %02d:%02d",
                             loc.GetString(StringID::SettingsTimerUntil),
                             timer.untilTime.wHour, timer.untilTime.wMinute);
            AppendBullet(part);
        }
    }

    StringCchCopyW(m_notifyData.szTip, _countof(m_notifyData.szTip), tooltip);
    // Keep NIF_GUID for modify when icon was registered by GUID.
    m_notifyData.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"update tray tooltip");
}

void TrayIcon::HandleMessage(LPARAM lParam) {
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

void TrayIcon::ShowContextMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    auto& loc = Localization::Instance();
    const wchar_t* toggleText = m_isEnabled ?
        loc.GetString(StringID::MenuDisable) :
        loc.GetString(StringID::MenuEnable);

    AppendMenuW(menu, MF_STRING, IDM_TOGGLE, toggleText);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_SETTINGS, loc.GetString(StringID::MenuSettings));
    AppendMenuW(menu, MF_STRING, IDM_ABOUT, loc.GetString(StringID::MenuAbout));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, loc.GetString(StringID::MenuExit));

    POINT cursor = {};
    GetCursorPos(&cursor);
    SetForegroundWindow(m_parentWindow);

    const int command = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        cursor.x, cursor.y, 0, m_parentWindow, nullptr);

    PostMessageW(m_parentWindow, WM_NULL, 0, 0);
    DestroyMenu(menu);

    switch (command) {
        case IDM_TOGGLE:
            if (m_onToggle) m_onToggle();
            break;
        case IDM_SETTINGS:
            if (m_onSettings) m_onSettings();
            break;
        case IDM_ABOUT:
            if (m_onAbout) m_onAbout();
            break;
        case IDM_EXIT:
            if (m_onExit) m_onExit();
            break;
    }
}

void TrayIcon::ShowNotification(const wchar_t* title, const wchar_t* message, DWORD flags) {
    if (m_notifyData.cbSize == 0) {
        return;
    }

    // Keep NIF_GUID for modify when icon was registered by GUID.
    m_notifyData.uFlags = NIF_INFO | NIF_GUID;
    StringCchCopyW(m_notifyData.szInfoTitle, _countof(m_notifyData.szInfoTitle), title);
    StringCchCopyW(m_notifyData.szInfo, _countof(m_notifyData.szInfo), message);
    m_notifyData.dwInfoFlags = flags;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"show tray notification");
}

} // namespace Everon
