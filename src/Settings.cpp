#include "Settings.h"
#include "Localization.h"
#include "HotkeyManager.h"
#include "TimerMode.h"
#include "Utils.h"
#include <strsafe.h>

namespace Everon {

namespace {

bool IsSameSystemTime(const SYSTEMTIME& a, const SYSTEMTIME& b) noexcept {
    return a.wYear == b.wYear &&
           a.wMonth == b.wMonth &&
           a.wDayOfWeek == b.wDayOfWeek &&
           a.wDay == b.wDay &&
           a.wHour == b.wHour &&
           a.wMinute == b.wMinute &&
           a.wSecond == b.wSecond &&
           a.wMilliseconds == b.wMilliseconds;
}

bool IsSameTimerConfig(const TimerConfig& a, const TimerConfig& b) noexcept {
    return a.mode == b.mode &&
           a.durationMinutes == b.durationMinutes &&
           IsSameSystemTime(a.untilTime, b.untilTime) &&
           IsSameSystemTime(a.startTime, b.startTime) &&
           a.endTimeUtc == b.endTimeUtc;
}

} // namespace

Settings::Settings() {
    m_autoStart = IsAutoStartEnabled();
    // Default UntilTime to current local time for a nicer UI default.
    GetLocalTime(&m_timerConfig.untilTime);
}

Language Settings::GetLanguage() const noexcept {
    return Localization::Instance().GetLanguage();
}

HotkeyConfig Settings::GetHotkeyConfig() const noexcept {
    return m_hotkeyConfig;
}

TimerConfig Settings::GetTimerConfig() const noexcept {
    return m_timerConfig;
}

void Settings::SetLanguage(Language value) noexcept {
    const Language old = GetLanguage();
    Localization::Instance().SetLanguage(value);
    if (GetLanguage() != old) {
        m_dirty = true;
    }
}

void Settings::SetHotkeyConfig(const HotkeyConfig& value) noexcept {
    if (m_hotkeyConfig != value) {
        m_hotkeyConfig = value;
        m_dirty = true;
    }
}

void Settings::SetTimerConfig(const TimerConfig& value) noexcept {
    if (!IsSameTimerConfig(m_timerConfig, value)) {
        m_timerConfig = value;
        m_dirty = true;
    }
}

void Settings::SetPeriodSec(DWORD value) noexcept {
    if (IsValidPeriod(value) && m_periodSec != value) {
        m_periodSec = value;
        m_dirty = true;
    }
}

void Settings::SetVirtualKey(WORD value) noexcept {
    if (IsValidVirtualKey(value) && m_vkKey != value) {
        m_vkKey = value;
        m_dirty = true;
    }
}

void Settings::SetKeepDisplayOn(bool value) noexcept {
    if (m_keepDisplayOn != value) {
        m_keepDisplayOn = value;
        m_dirty = true;
    }
}

void Settings::SetShowToggleNotifications(bool value) noexcept {
    if (m_showToggleNotifications != value) {
        m_showToggleNotifications = value;
        m_dirty = true;
    }
}

void Settings::SetEnabled(bool value) noexcept {
    if (m_enabled != value) {
        m_enabled = value;
        m_dirty = true;
    }
}

bool Settings::IsValidPeriod(DWORD value) const noexcept {
    return value >= MIN_PERIOD_SEC && value <= MAX_PERIOD_SEC;
}

bool Settings::IsValidVirtualKey(WORD vk) const noexcept {
    return vk == 0 || vk == VK_F15 || vk == VK_F16 || vk == VK_F17;
}

bool Settings::LoadFromRegistry() {
    HKEY hKey = nullptr;
    const LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey);
    if (openRes != ERROR_SUCCESS) {
        if (openRes != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(openRes, L"RegOpenKeyExW(HKCU\\\\Software\\\\Everon)");
        }
        // First run (or settings cleared). Keep defaults but ensure language/autostart are initialized.
        SetLanguage(Localization::DetectSystemLanguage());
        m_autoStart = IsAutoStartEnabled();
        m_dirty = false;
        return true;
    }

    auto ReadDword = [hKey](const wchar_t* name, DWORD& outValue) -> bool {
        DWORD type = 0;
        DWORD size = sizeof(DWORD);
        DWORD value = 0;
        const LONG res = RegQueryValueExW(hKey, name, nullptr, &type,
                                          reinterpret_cast<LPBYTE>(&value), &size);
        if (res == ERROR_SUCCESS && type == REG_DWORD) {
            outValue = value;
            return true;
        }
        if (res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(res, L"RegQueryValueExW(REG_DWORD)");
        }
        return false;
    };

    auto ReadQword = [hKey](const wchar_t* name, ULONGLONG& outValue) -> bool {
        DWORD type = 0;
        DWORD size = sizeof(ULONGLONG);
        ULONGLONG value = 0;
        const LONG res = RegQueryValueExW(hKey, name, nullptr, &type,
                                          reinterpret_cast<LPBYTE>(&value), &size);
        if (res == ERROR_SUCCESS && type == REG_QWORD) {
            outValue = value;
            return true;
        }
        if (res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(res, L"RegQueryValueExW(REG_QWORD)");
        }
        return false;
    };

    auto ReadString = [hKey](const wchar_t* name, wchar_t* buffer, DWORD bufferSize) -> bool {
        DWORD type = 0;
        DWORD size = bufferSize;
        const LONG res = RegQueryValueExW(hKey, name, nullptr, &type,
                                          reinterpret_cast<LPBYTE>(buffer), &size);
        if (res == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            return true;
        }
        if (res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(res, L"RegQueryValueExW(REG_SZ)");
        }
        return false;
    };

    DWORD tempDword = 0;
    if (ReadDword(L"PeriodSec", tempDword)) {
        SetPeriodSec(tempDword);
    }
    if (ReadDword(L"VkKey", tempDword)) {
        SetVirtualKey(static_cast<WORD>(tempDword));
    }
    if (ReadDword(L"KeepDisplayOn", tempDword)) {
        m_keepDisplayOn = (tempDword != 0);
    }
    if (ReadDword(L"ShowToggleNotifications", tempDword)) {
        m_showToggleNotifications = (tempDword != 0);
    }
    if (ReadDword(L"Enabled", tempDword)) {
        m_enabled = (tempDword != 0);
    }

    wchar_t langBuffer[16] = {};
    if (ReadString(L"Language", langBuffer, sizeof(langBuffer))) {
        SetLanguage(Localization::StringToLanguage(langBuffer));
    } else {
        SetLanguage(Localization::DetectSystemLanguage());
    }


    // Hotkey
    wchar_t hotkeyBuffer[128] = {};
    if (ReadString(L"Hotkey", hotkeyBuffer, sizeof(hotkeyBuffer))) {
        m_hotkeyConfig = HotkeyManager::StringToHotkey(hotkeyBuffer);
    }

    // Timer
    TimerConfig timer = m_timerConfig;

    DWORD tempMode = 0;
    if (ReadDword(L"TimerMode", tempMode)) {
        timer.mode = static_cast<TimerMode>(tempMode);
    }
    DWORD tempDuration = timer.durationMinutes;
    if (ReadDword(L"TimerDuration", tempDuration)) {
        timer.durationMinutes = tempDuration;
    }

    DWORD type = 0;
    DWORD size = sizeof(SYSTEMTIME);
    SYSTEMTIME st = {};

    LONG qRes = RegQueryValueExW(hKey, L"TimerUntilTime", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(&st), &size);
    if (qRes == ERROR_SUCCESS && type == REG_BINARY && size == sizeof(SYSTEMTIME)) {
        timer.untilTime = st;
    } else if (qRes != ERROR_SUCCESS && qRes != ERROR_FILE_NOT_FOUND) {
        Utils::CheckWinApiStatus(qRes, L"RegQueryValueExW(TimerUntilTime)");
    }

    size = sizeof(SYSTEMTIME);
    st = {};
    qRes = RegQueryValueExW(hKey, L"TimerStartTime", nullptr, &type,
                            reinterpret_cast<LPBYTE>(&st), &size);
    if (qRes == ERROR_SUCCESS && type == REG_BINARY && size == sizeof(SYSTEMTIME)) {
        timer.startTime = st;
    } else if (qRes != ERROR_SUCCESS && qRes != ERROR_FILE_NOT_FOUND) {
        Utils::CheckWinApiStatus(qRes, L"RegQueryValueExW(TimerStartTime)");
    }

    // Timer runtime end moment (UTC) - preferred over legacy startTime for DST robustness
    ULONGLONG tempQword = 0;
    if (ReadQword(L"TimerEndUtc", tempQword)) {
        timer.endTimeUtc = tempQword;
    } else if (timer.mode == TimerMode::Duration && timer.startTime.wYear != 0) {
        // Backward compatibility: compute endTimeUtc from legacy startTime
        SYSTEMTIME startUtc = {};
        if (!TzSpecificLocalTimeToSystemTime(nullptr, &timer.startTime, &startUtc)) {
            startUtc = timer.startTime;
        }
        FILETIME startFt = {};
        if (SystemTimeToFileTime(&startUtc, &startFt)) {
            ULARGE_INTEGER u;
            u.LowPart = startFt.dwLowDateTime;
            u.HighPart = startFt.dwHighDateTime;
            timer.endTimeUtc = u.QuadPart + (static_cast<ULONGLONG>(timer.durationMinutes) * 60ULL * 10000000ULL);
        }
    }

    // Sanity check
    if (!timer.IsValid()) {
        timer = TimerConfig{};
        GetLocalTime(&timer.untilTime);
    }
    m_timerConfig = timer;

    const LONG closeRes = RegCloseKey(hKey);
    Utils::CheckWinApiStatus(closeRes, L"RegCloseKey(HKCU\\\\Software\\\\Everon)");
    m_autoStart = IsAutoStartEnabled();
    m_dirty = false;
    return true;
}

bool Settings::SaveToRegistry() {
    if (!m_dirty) {
        return true;
    }

    HKEY hKey = nullptr;
    const LONG createRes = RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, nullptr, 0,
                                           KEY_WRITE, nullptr, &hKey, nullptr);
    if (!Utils::CheckWinApiStatus(createRes, L"RegCreateKeyExW(HKCU\\\\Software\\\\Everon)")) {
        return false;
    }

    auto WriteDword = [hKey](const wchar_t* name, DWORD value) -> bool {
        const LONG res = RegSetValueExW(hKey, name, 0, REG_DWORD,
                                        reinterpret_cast<const BYTE*>(&value),
                                        sizeof(value));
        if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(REG_DWORD)")) {
            Utils::DebugLog(L"[Everon][Reg] Failed to write DWORD value '%s'\n", name);
            return false;
        }
        return true;
    };

    auto WriteString = [hKey](const wchar_t* name, const wchar_t* value) -> bool {
        DWORD size = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
        const LONG res = RegSetValueExW(hKey, name, 0, REG_SZ,
                                        reinterpret_cast<const BYTE*>(value),
                                        size);
        if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(REG_SZ)")) {
            Utils::DebugLog(L"[Everon][Reg] Failed to write string value '%s'\n", name);
            return false;
        }
        return true;
    };

    auto WriteQword = [hKey](const wchar_t* name, ULONGLONG value) -> bool {
        const LONG res = RegSetValueExW(hKey, name, 0, REG_QWORD,
                                        reinterpret_cast<const BYTE*>(&value),
                                        sizeof(value));
        if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(REG_QWORD)")) {
            Utils::DebugLog(L"[Everon][Reg] Failed to write QWORD value '%s'\n", name);
            return false;
        }
        return true;
    };

    bool success = true;
    success &= WriteDword(L"PeriodSec", m_periodSec);
    success &= WriteDword(L"VkKey", static_cast<DWORD>(m_vkKey));
    success &= WriteDword(L"KeepDisplayOn", m_keepDisplayOn ? 1 : 0);
    success &= WriteDword(L"ShowToggleNotifications", m_showToggleNotifications ? 1 : 0);
    success &= WriteDword(L"Enabled", m_enabled ? 1 : 0);
    success &= WriteString(L"Language", Localization::LanguageToString(GetLanguage()));

    success &= WriteString(L"Hotkey", HotkeyManager::HotkeyToRegistryString(m_hotkeyConfig).c_str());

    const TimerConfig& timer = m_timerConfig;
    success &= WriteDword(L"TimerMode", static_cast<DWORD>(timer.mode));
    success &= WriteDword(L"TimerDuration", timer.durationMinutes);
    LONG res = RegSetValueExW(hKey, L"TimerUntilTime", 0, REG_BINARY,
                              reinterpret_cast<const BYTE*>(&timer.untilTime),
                              sizeof(SYSTEMTIME));
    if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(TimerUntilTime)")) {
        success = false;
    }

    res = RegSetValueExW(hKey, L"TimerStartTime", 0, REG_BINARY,
                         reinterpret_cast<const BYTE*>(&timer.startTime),
                         sizeof(SYSTEMTIME));
    if (!Utils::CheckWinApiStatus(res, L"RegSetValueExW(TimerStartTime)")) {
        success = false;
    }

    // Store end moment only for the currently enabled run; avoid stale values when disabled.
    ULONGLONG endUtcToSave = 0;
    if (m_enabled && timer.mode != TimerMode::Indefinite) {
        endUtcToSave = timer.endTimeUtc;
    }
    success &= WriteQword(L"TimerEndUtc", endUtcToSave);

    const LONG closeRes = RegCloseKey(hKey);
    Utils::CheckWinApiStatus(closeRes, L"RegCloseKey(HKCU\\\\Software\\\\Everon)");

    if (success) {
        m_dirty = false;
    }
    return success;
}

bool Settings::IsAutoStartEnabled() {
    HKEY hKey = nullptr;
    const LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY_PATH, 0, KEY_READ, &hKey);
    if (openRes != ERROR_SUCCESS) {
        if (openRes != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(openRes, L"RegOpenKeyExW(HKCU\\\\Run)");
        }
        return false;
    }

    wchar_t value[2048] = {};
    DWORD type = 0;
    DWORD size = sizeof(value);
    LONG result = RegQueryValueExW(hKey, APP_NAME, nullptr, &type,
                                   reinterpret_cast<LPBYTE>(value), &size);
    const LONG closeRes = RegCloseKey(hKey);
    Utils::CheckWinApiStatus(closeRes, L"RegCloseKey(HKCU\\\\Run)");

    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        if (result != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(result, L"RegQueryValueExW(HKCU\\\\Run\\\\Everon)");
        }
        return false;
    }

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Expand environment variables if needed, then parse the first token.
    std::wstring stored;
    if (type == REG_EXPAND_SZ) {
        wchar_t expanded[4096] = {};
        DWORD expandedLen = ExpandEnvironmentStringsW(value, expanded, _countof(expanded));
        if (expandedLen != 0 && expandedLen <= _countof(expanded)) {
            stored = expanded;
        } else {
            stored = value;
        }
    } else {
        stored = value;
    }

    std::wstring first;
    if (!stored.empty() && stored[0] == L'"') {
        size_t end = stored.find(L'"', 1);
        if (end != std::wstring::npos) {
            first = stored.substr(1, end - 1);
        }
    } else {
        size_t end = stored.find_first_of(L" \t");
        first = stored.substr(0, end);
    }

    if (first.empty()) {
        return false;
    }

    return _wcsicmp(first.c_str(), exePath) == 0;
}

bool Settings::SetAutoStartEnabled(bool enable) {
    HKEY hKey = nullptr;
    const LONG createRes = RegCreateKeyExW(HKEY_CURRENT_USER, RUN_KEY_PATH, 0, nullptr, 0,
                                           KEY_WRITE, nullptr, &hKey, nullptr);
    if (!Utils::CheckWinApiStatus(createRes, L"RegCreateKeyExW(HKCU\\\\Run)")) {
        return false;
    }

    bool success = false;
    if (enable) {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        wchar_t quotedPath[2048] = {};
        StringCchPrintfW(quotedPath, _countof(quotedPath), L"\"%s\"", exePath);
        DWORD size = static_cast<DWORD>((wcslen(quotedPath) + 1) * sizeof(wchar_t));
        LONG setRes = RegSetValueExW(hKey, APP_NAME, 0, REG_SZ,
                                     reinterpret_cast<const BYTE*>(quotedPath),
                                     size);
        success = Utils::CheckWinApiStatus(setRes, L"RegSetValueExW(HKCU\\\\Run\\\\Everon)");
    } else {
        LONG delRes = RegDeleteValueW(hKey, APP_NAME);
        if (delRes == ERROR_SUCCESS || delRes == ERROR_FILE_NOT_FOUND) {
            success = true;
        } else {
            Utils::CheckWinApiStatus(delRes, L"RegDeleteValueW(HKCU\\\\Run\\\\Everon)");
            success = false;
        }
    }

    const LONG closeRes = RegCloseKey(hKey);
    Utils::CheckWinApiStatus(closeRes, L"RegCloseKey(HKCU\\\\Run)");
    return success;
}

} // namespace Everon
