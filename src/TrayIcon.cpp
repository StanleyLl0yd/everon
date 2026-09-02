#include "TrayIcon.h"
#include "Settings.h"
#include "Utils.h"
#include "Localization.h"
#include "TimerMode.h"
#include "resource.h"
#include <strsafe.h>
#include <array>
#include <bit>
#include <format>
#include <string_view>

#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP 0x00000080
#endif
#ifndef NIF_GUID
#define NIF_GUID 0x00000020
#endif

namespace {

HICON SelectTrayIcon(bool enabled, HICON activeIcon, HICON disabledIcon) noexcept {
    if (enabled && activeIcon) {
        return activeIcon;
    }
    return disabledIcon ? disabledIcon : activeIcon;
}

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

    const auto screen = GetDC(nullptr);
    if (!screen) {
        return nullptr;
    }

    void* rawBits = nullptr;
    const auto color = CreateDIBSection(screen, std::bit_cast<BITMAPINFO*>(&header),
                                        DIB_RGB_COLORS, &rawBits, nullptr, 0);
    const auto memory = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (!color || !memory || !rawBits) {
        if (memory) DeleteDC(memory);
        if (color) DeleteObject(color);
        return nullptr;
    }

    const auto oldBitmap = SelectObject(memory, color);
    ZeroMemory(rawBits, static_cast<SIZE_T>(width) * height * 4);
    const BOOL drawn = DrawIconEx(memory, 0, 0, source, width, height, 0, nullptr, DI_NORMAL);
    SelectObject(memory, oldBitmap);
    DeleteDC(memory);
    if (!drawn) {
        DeleteObject(color);
        return nullptr;
    }

    auto* pixels = static_cast<BYTE*>(rawBits);
    const auto pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    for (size_t i = 0; i < pixelCount; ++i) {
        auto* pixel = pixels + (i * 4);
        const BYTE gray = static_cast<BYTE>((static_cast<unsigned>(pixel[2]) * 30U +
                                             static_cast<unsigned>(pixel[1]) * 59U +
                                             static_cast<unsigned>(pixel[0]) * 11U) / 100U);
        const BYTE muted = static_cast<BYTE>((static_cast<unsigned>(gray) + 180U) / 2U);
        pixel[0] = muted;
        pixel[1] = muted;
        pixel[2] = muted;
    }

    const auto mask = CreateBitmap(width, height, 1, 1, nullptr);
    if (!mask) {
        DeleteObject(color);
        return nullptr;
    }

    ICONINFO iconInfo{};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = color;
    iconInfo.hbmMask = mask;
    const auto result = CreateIconIndirect(&iconInfo);
    DeleteObject(mask);
    DeleteObject(color);
    return result;
}

std::wstring BuildKeyPressStatus(const Everon::Settings& settings, wchar_t secUnit) {
    const auto vk = settings.GetVirtualKey();
    const auto period = settings.GetPeriodSec();
    if (vk == 0 || period == 0) {
        return {};
    }

    return std::format(L"{}/{}{}", Everon::Utils::GetKeyName(vk), period, secUnit);
}

struct TimerStatus {
    DWORD activeDurationMinutes = 0;
    std::wstring text;
};

TimerStatus BuildTimerStatus(const Everon::Settings& settings,
                             const Everon::Localization& loc,
                             wchar_t minUnit,
                             wchar_t hourUnit) {
    using enum Everon::StringID;
    using enum Everon::TimerMode;

    const auto timer = settings.GetTimerConfig();
    if (timer.mode == Duration) {
        const auto remaining = timer.GetRemainingSeconds();
        if (remaining == INFINITE || remaining == 0) {
            return {timer.durationMinutes, {}};
        }

        auto minutes = (remaining + 59U) / 60U;
        const auto hours = minutes / 60U;
        minutes %= 60U;
        if (hours > 0) {
            return {
                timer.durationMinutes,
                std::format(L"{} {}{} {:02}{}", loc.GetString(SettingsTimer),
                            hours, hourUnit, minutes, minUnit)
            };
        }
        return {
            timer.durationMinutes,
            std::format(L"{} {}{}", loc.GetString(SettingsTimer), minutes, minUnit)
        };
    }

    if (timer.mode == UntilTime) {
        if (timer.IsUntilNextDay()) {
            return {
                0,
                std::format(L"{} {} {:02}:{:02}", loc.GetString(SettingsTimerUntil),
                            loc.GetString(SettingsTimerTomorrow),
                            timer.untilTime.wHour, timer.untilTime.wMinute)
            };
        }
        return {
            0,
            std::format(L"{} {:02}:{:02}", loc.GetString(SettingsTimerUntil),
                        timer.untilTime.wHour, timer.untilTime.wMinute)
        };
    }

    return {};
}

HMENU CreateTimerMenu(bool enabled, DWORD activeDurationMinutes,
                      const Everon::Localization& loc) {
    using enum Everon::StringID;

    const auto timerMenu = CreatePopupMenu();
    if (!timerMenu) {
        return nullptr;
    }

    const auto durationFlags = [enabled, activeDurationMinutes](DWORD minutes) -> UINT {
        const bool checked = enabled && activeDurationMinutes == minutes;
        return MF_STRING | (checked ? MF_CHECKED : 0U);
    };

    AppendMenuW(timerMenu, durationFlags(15), IDM_TIMER_15, loc.GetString(MenuTimer15));
    AppendMenuW(timerMenu, durationFlags(30), IDM_TIMER_30, loc.GetString(MenuTimer30));
    AppendMenuW(timerMenu, durationFlags(60), IDM_TIMER_60, loc.GetString(MenuTimer60));
    AppendMenuW(timerMenu, durationFlags(120), IDM_TIMER_120, loc.GetString(MenuTimer120));
    AppendMenuW(timerMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(timerMenu, MF_STRING, IDM_TIMER_CUSTOM, loc.GetString(MenuTimerCustom));
    AppendMenuW(timerMenu, MF_STRING, IDM_TIMER_UNTIL, loc.GetString(MenuTimerUntil));
    return timerMenu;
}

}

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

    m_notifyData.cbSize = sizeof(NOTIFYICONDATAW);
    m_notifyData.hWnd = m_parentWindow;
    m_notifyData.uID = 1;
    m_notifyData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    m_notifyData.uCallbackMessage = WM_TRAYICON;

    static const GUID kTrayGuid =
        {0x8b5e6f7a, 0x6d8a, 0x4a0c, {0x9d, 0x2e, 0x4f, 0x7d, 0x7a, 0x51, 0x1c, 0x10}};
    m_notifyData.guidItem = kTrayGuid;

    const auto iconWidth = GetSystemMetrics(SM_CXSMICON);
    const auto iconHeight = GetSystemMetrics(SM_CYSMICON);
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
    m_notifyData.hIcon = SelectTrayIcon(m_isEnabled, m_activeIcon, m_disabledIcon);

    StringCchCopyW(m_notifyData.szTip, _countof(m_notifyData.szTip), L"Everon");

    if (!Utils::ShellNotifyIconChecked(NIM_ADD, &m_notifyData, L"add tray icon")) {
        Utils::DebugLog(L"[Everon] Failed to add tray icon: {}\n", GetLastError());
        return false;
    }

    m_notifyData.uVersion = NOTIFYICON_VERSION_4;

    const auto savedFlags = m_notifyData.uFlags;
    m_notifyData.uFlags = 0;
    if (!Utils::ShellNotifyIconChecked(NIM_SETVERSION, &m_notifyData, L"set tray icon v4")) {
        m_notifyData.uVersion = NOTIFYICON_VERSION;
        Utils::ShellNotifyIconChecked(NIM_SETVERSION, &m_notifyData, L"set tray icon legacy version");
    }
    m_notifyData.uFlags = savedFlags;
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
    if (m_notifyData.cbSize == 0) {
        return;
    }

    Utils::ShellNotifyIconChecked(NIM_DELETE, &m_notifyData, L"remove tray icon");
    if (m_disabledIcon) {
        DestroyIcon(m_disabledIcon);
        m_disabledIcon = nullptr;
    }
    if (m_activeIconOwned && m_activeIcon) {
        DestroyIcon(m_activeIcon);
    }
    m_activeIcon = nullptr;
    m_activeIconOwned = false;
    m_notifyData = {};
}

void TrayIcon::UpdateTooltip(const Settings& settings,
                             bool pausedByBatterySaver,
                             bool displayKeepAwakeActive) {
    using enum StringID;

    if (m_notifyData.cbSize == 0) {
        return;
    }

    const auto& loc = Localization::Instance();
    std::wstring status = settings.IsEnabled()
        ? loc.GetString(TooltipEnabled)
        : loc.GetString(TooltipDisabled);

    const auto append = [&status](std::wstring_view value) {
        if (!value.empty()) {
            status += L" • ";
            status.append(value);
        }
    };

    m_activeDurationMinutes = 0;
    if (settings.IsEnabled()) {
        if (pausedByBatterySaver) {
            append(loc.GetString(StatusBatterySaverPaused));
        }

        const bool isRu = loc.GetLanguage() == Language::Russian;
        const wchar_t secUnit = isRu ? L'\x0441' : L's';
        const wchar_t minUnit = isRu ? L'\x043C' : L'm';
        const wchar_t hourUnit = isRu ? L'\x0447' : L'h';

        append(BuildKeyPressStatus(settings, secUnit));
        if (displayKeepAwakeActive) {
            append(loc.GetString(SettingsKeepDisplay));
        }

        const auto timerStatus = BuildTimerStatus(settings, loc, minUnit, hourUnit);
        m_activeDurationMinutes = timerStatus.activeDurationMinutes;
        append(timerStatus.text);
    }

    m_statusText = status;
    StringCchCopyW(m_notifyData.szTip, _countof(m_notifyData.szTip), status.c_str());
    m_notifyData.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"update tray tooltip");
}

void TrayIcon::SetEnabled(bool enabled) {
    m_isEnabled = enabled;
    UpdateIcon();
}

void TrayIcon::UpdateIcon() {
    if (m_notifyData.cbSize == 0) {
        return;
    }

    const auto icon = SelectTrayIcon(m_isEnabled, m_activeIcon, m_disabledIcon);
    if (!icon) {
        return;
    }
    m_notifyData.hIcon = icon;
    m_notifyData.uFlags = NIF_ICON | NIF_GUID;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"update tray state icon");
}

void TrayIcon::HandleMessage(LPARAM lParam) {
    const auto mouseMsg = LOWORD(lParam);

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

        default:
            break;
    }
}

void TrayIcon::ShowContextMenu() {
    using enum StringID;

    const auto menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    const auto& loc = Localization::Instance();
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, m_statusText.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    const auto* toggleText = m_isEnabled
        ? loc.GetString(MenuDisable)
        : loc.GetString(MenuEnable);
    AppendMenuW(menu, MF_STRING, IDM_TOGGLE, toggleText);

    if (const auto timerMenu = CreateTimerMenu(m_isEnabled, m_activeDurationMinutes, loc)) {
        AppendMenuW(menu, MF_POPUP, std::bit_cast<UINT_PTR>(timerMenu), loc.GetString(MenuQuickTimer));
    }

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_SETTINGS, loc.GetString(MenuSettings));
    AppendMenuW(menu, MF_STRING, IDM_ABOUT, loc.GetString(MenuAbout));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, loc.GetString(MenuExit));

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(m_parentWindow);

    const auto command = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        cursor.x, cursor.y, 0, m_parentWindow, nullptr);

    PostMessageW(m_parentWindow, WM_NULL, 0, 0);
    DestroyMenu(menu);
    DispatchCommand(command);
}

void TrayIcon::DispatchCommand(int command) {
    switch (command) {
        case IDM_TOGGLE:
            if (m_onToggle) m_onToggle();
            break;
        case IDM_TIMER_15:
            if (m_onDuration) m_onDuration(15);
            break;
        case IDM_TIMER_30:
            if (m_onDuration) m_onDuration(30);
            break;
        case IDM_TIMER_60:
            if (m_onDuration) m_onDuration(60);
            break;
        case IDM_TIMER_120:
            if (m_onDuration) m_onDuration(120);
            break;
        case IDM_TIMER_CUSTOM:
            if (m_onCustomDuration) m_onCustomDuration();
            break;
        case IDM_TIMER_UNTIL:
            if (m_onUntil) m_onUntil();
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
        default:
            break;
    }
}

void TrayIcon::ShowNotification(const wchar_t* title, const wchar_t* message, DWORD flags) {
    if (m_notifyData.cbSize == 0) {
        return;
    }

    m_notifyData.uFlags = NIF_INFO | NIF_GUID;
    StringCchCopyW(m_notifyData.szInfoTitle, _countof(m_notifyData.szInfoTitle), title);
    StringCchCopyW(m_notifyData.szInfo, _countof(m_notifyData.szInfo), message);
    m_notifyData.dwInfoFlags = flags;
    Utils::ShellNotifyIconChecked(NIM_MODIFY, &m_notifyData, L"show tray notification");
}

}
