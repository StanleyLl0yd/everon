#include "Settings.h"
#include "Localization.h"
#include "HotkeyManager.h"
#include "TimerMode.h"
#include "Utils.h"
#include <optional>
#include <string_view>
#include <vector>

namespace Everon {

namespace {

class RegistryKey {
public:
    explicit RegistryKey(HKEY key = nullptr) noexcept
        : m_key(key) {
    }

    ~RegistryKey() {
        if (m_key) {
            RegCloseKey(m_key);
        }
    }

    RegistryKey(const RegistryKey&) = delete;
    RegistryKey& operator=(const RegistryKey&) = delete;
    RegistryKey(RegistryKey&&) = delete;
    RegistryKey& operator=(RegistryKey&&) = delete;

    HKEY Get() const noexcept {
        return m_key;
    }

private:
    HKEY m_key = nullptr;
};

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

std::wstring GetExecutablePath() {
    DWORD capacity = 260;
    while (capacity <= 32768) {
        std::vector<wchar_t> buffer(capacity, L'\0');
        SetLastError(ERROR_SUCCESS);
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), capacity);
        if (length == 0) {
            Utils::DebugLog(L"[Everon] GetModuleFileNameW failed: %lu\n", GetLastError());
            return {};
        }
        if (length < capacity) {
            return std::wstring(buffer.data(), length);
        }
        capacity *= 2;
    }
    Utils::DebugLog(L"[Everon] Executable path is unexpectedly long\n");
    return {};
}

bool ReadDwordValue(HKEY key, const wchar_t* name, DWORD& outValue) {
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LONG result = RegGetValueW(key, nullptr, name, RRF_RT_REG_DWORD,
                                     nullptr, &value, &size);
    if (result == ERROR_SUCCESS && size == sizeof(value)) {
        outValue = value;
        return true;
    }
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        Utils::CheckWinApiStatus(result, L"RegGetValueW(REG_DWORD)");
    }
    return false;
}

bool ReadQwordValue(HKEY key, const wchar_t* name, ULONGLONG& outValue) {
    ULONGLONG value = 0;
    DWORD size = sizeof(value);
    const LONG result = RegGetValueW(key, nullptr, name, RRF_RT_REG_QWORD,
                                     nullptr, &value, &size);
    if (result == ERROR_SUCCESS && size == sizeof(value)) {
        outValue = value;
        return true;
    }
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        Utils::CheckWinApiStatus(result, L"RegGetValueW(REG_QWORD)");
    }
    return false;
}

std::optional<std::wstring> ReadStringValue(HKEY key, const wchar_t* name) {
    constexpr DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND;
    DWORD type = 0;
    DWORD size = 0;
    LONG result = RegGetValueW(key, nullptr, name, flags, &type, nullptr, &size);
    if (result == ERROR_FILE_NOT_FOUND) {
        return std::nullopt;
    }
    if (result != ERROR_SUCCESS) {
        Utils::CheckWinApiStatus(result, L"RegGetValueW(REG_SZ)");
        return std::nullopt;
    }

    std::vector<wchar_t> buffer((size / sizeof(wchar_t)) + 1, L'\0');
    DWORD readSize = size;
    result = RegGetValueW(key, nullptr, name, flags, &type, buffer.data(), &readSize);
    if (result != ERROR_SUCCESS) {
        Utils::CheckWinApiStatus(result, L"RegGetValueW(REG_SZ)");
        return std::nullopt;
    }
    buffer.back() = L'\0';
    return std::wstring(buffer.data());
}

bool ReadSystemTimeValue(HKEY key, const wchar_t* name, SYSTEMTIME& outValue) {
    SYSTEMTIME value{};
    DWORD size = sizeof(value);
    const LONG result = RegGetValueW(key, nullptr, name, RRF_RT_REG_BINARY,
                                     nullptr, &value, &size);
    if (result == ERROR_SUCCESS && size == sizeof(value)) {
        outValue = value;
        return true;
    }
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        Utils::CheckWinApiStatus(result, L"RegGetValueW(REG_BINARY)");
    }
    return false;
}

bool WriteDwordValue(HKEY key, const wchar_t* name, DWORD value) {
    const LONG result = RegSetKeyValueW(key, nullptr, name, REG_DWORD,
                                        &value, sizeof(value));
    return Utils::CheckWinApiStatus(result, L"RegSetKeyValueW(REG_DWORD)");
}

bool WriteQwordValue(HKEY key, const wchar_t* name, ULONGLONG value) {
    const LONG result = RegSetKeyValueW(key, nullptr, name, REG_QWORD,
                                        &value, sizeof(value));
    return Utils::CheckWinApiStatus(result, L"RegSetKeyValueW(REG_QWORD)");
}

bool WriteStringValue(HKEY key, const wchar_t* name, std::wstring_view value) {
    const auto stored = std::wstring(value);
    const auto size = static_cast<DWORD>((stored.size() + 1) * sizeof(wchar_t));
    const LONG result = RegSetKeyValueW(key, nullptr, name, REG_SZ,
                                        stored.c_str(), size);
    return Utils::CheckWinApiStatus(result, L"RegSetKeyValueW(REG_SZ)");
}

bool WriteSystemTimeValue(HKEY key, const wchar_t* name, const SYSTEMTIME& value) {
    const LONG result = RegSetKeyValueW(key, nullptr, name, REG_BINARY,
                                        &value, sizeof(value));
    return Utils::CheckWinApiStatus(result, L"RegSetKeyValueW(REG_BINARY)");
}

void RecoverLegacyDurationEndTime(TimerConfig& timer) {
    if (timer.mode != TimerMode::Duration || timer.startTime.wYear == 0) {
        return;
    }

    SYSTEMTIME startUtc{};
    if (!TzSpecificLocalTimeToSystemTime(nullptr, &timer.startTime, &startUtc)) {
        return;
    }

    FILETIME startFt{};
    if (!SystemTimeToFileTime(&startUtc, &startFt)) {
        return;
    }

    ULARGE_INTEGER value{};
    value.LowPart = startFt.dwLowDateTime;
    value.HighPart = startFt.dwHighDateTime;
    timer.endTimeUtc = value.QuadPart +
        (static_cast<ULONGLONG>(timer.durationMinutes) * 60ULL * 10000000ULL);
}

TimerConfig ReadTimerConfig(HKEY key, TimerConfig timer) {
    DWORD mode = 0;
    if (ReadDwordValue(key, L"TimerMode", mode)) {
        timer.mode = static_cast<TimerMode>(mode);
    }

    DWORD duration = timer.durationMinutes;
    if (ReadDwordValue(key, L"TimerDuration", duration)) {
        timer.durationMinutes = duration;
    }

    ReadSystemTimeValue(key, L"TimerUntilTime", timer.untilTime);
    ReadSystemTimeValue(key, L"TimerStartTime", timer.startTime);

    ULONGLONG endUtc = 0;
    if (ReadQwordValue(key, L"TimerEndUtc", endUtc)) {
        timer.endTimeUtc = endUtc;
    } else {
        RecoverLegacyDurationEndTime(timer);
    }

    if (!timer.IsValid()) {
        timer = TimerConfig{};
        GetLocalTime(&timer.untilTime);
    }
    return timer;
}

std::wstring ExpandRegistryString(const std::wstring& value) {
    const DWORD needed = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
    if (needed == 0) {
        return value;
    }

    std::vector<wchar_t> expanded(needed, L'\0');
    if (ExpandEnvironmentStringsW(value.c_str(), expanded.data(), needed) == 0) {
        return value;
    }
    return std::wstring(expanded.data());
}

std::wstring ExtractExecutableToken(const std::wstring& command) {
    if (command.empty()) {
        return {};
    }
    if (command.front() == L'"') {
        const auto end = command.find(L'"', 1);
        return end == std::wstring::npos ? std::wstring{} : command.substr(1, end - 1);
    }
    const auto end = command.find_first_of(L" \t");
    return command.substr(0, end);
}

}

Settings::Settings() {
    m_autoStart = IsAutoStartEnabled();
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
    const auto old = GetLanguage();
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

void Settings::SetRespectBatterySaver(bool value) noexcept {
    if (m_respectBatterySaver != value) {
        m_respectBatterySaver = value;
        m_dirty = true;
    }
}

void Settings::SetAllowDisplayOnBattery(bool value) noexcept {
    if (m_allowDisplayOnBattery != value) {
        m_allowDisplayOnBattery = value;
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
    HKEY rawKey = nullptr;
    const LONG openResult = RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &rawKey);
    if (openResult != ERROR_SUCCESS) {
        if (openResult != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(openResult, L"RegOpenKeyExW(HKCU\\Software\\Everon)");
        }
        SetLanguage(Localization::DetectSystemLanguage());
        m_autoStart = IsAutoStartEnabled();
        m_dirty = false;
        return true;
    }
    RegistryKey key(rawKey);

    DWORD value = 0;
    if (ReadDwordValue(key.Get(), L"PeriodSec", value)) {
        SetPeriodSec(value);
    }
    if (ReadDwordValue(key.Get(), L"VkKey", value)) {
        SetVirtualKey(static_cast<WORD>(value));
    }
    if (ReadDwordValue(key.Get(), L"KeepDisplayOn", value)) {
        m_keepDisplayOn = value != 0;
    }
    if (ReadDwordValue(key.Get(), L"RespectBatterySaver", value)) {
        m_respectBatterySaver = value != 0;
    }
    if (ReadDwordValue(key.Get(), L"AllowDisplayOnBattery", value)) {
        m_allowDisplayOnBattery = value != 0;
    }
    if (ReadDwordValue(key.Get(), L"ShowToggleNotifications", value)) {
        m_showToggleNotifications = value != 0;
    }
    if (ReadDwordValue(key.Get(), L"Enabled", value)) {
        m_enabled = value != 0;
    }

    if (const auto language = ReadStringValue(key.Get(), L"Language")) {
        SetLanguage(Localization::StringToLanguage(language->c_str()));
    } else {
        SetLanguage(Localization::DetectSystemLanguage());
    }

    if (const auto hotkey = ReadStringValue(key.Get(), L"Hotkey")) {
        m_hotkeyConfig = HotkeyManager::StringToHotkey(hotkey->c_str());
    }

    m_timerConfig = ReadTimerConfig(key.Get(), m_timerConfig);
    m_autoStart = IsAutoStartEnabled();
    m_dirty = false;
    return true;
}

bool Settings::SaveToRegistry() {
    if (!m_dirty) {
        return true;
    }

    HKEY rawKey = nullptr;
    const LONG createResult = RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, nullptr, 0,
                                               KEY_WRITE, nullptr, &rawKey, nullptr);
    if (!Utils::CheckWinApiStatus(createResult, L"RegCreateKeyExW(HKCU\\Software\\Everon)")) {
        return false;
    }
    RegistryKey key(rawKey);

    bool success = true;
    success &= WriteDwordValue(key.Get(), L"PeriodSec", m_periodSec);
    success &= WriteDwordValue(key.Get(), L"VkKey", static_cast<DWORD>(m_vkKey));
    success &= WriteDwordValue(key.Get(), L"KeepDisplayOn", m_keepDisplayOn ? 1U : 0U);
    success &= WriteDwordValue(key.Get(), L"RespectBatterySaver", m_respectBatterySaver ? 1U : 0U);
    success &= WriteDwordValue(key.Get(), L"AllowDisplayOnBattery", m_allowDisplayOnBattery ? 1U : 0U);
    success &= WriteDwordValue(key.Get(), L"ShowToggleNotifications", m_showToggleNotifications ? 1U : 0U);
    success &= WriteDwordValue(key.Get(), L"Enabled", m_enabled ? 1U : 0U);
    success &= WriteStringValue(key.Get(), L"Language", Localization::LanguageToString(GetLanguage()));
    success &= WriteStringValue(key.Get(), L"Hotkey", HotkeyManager::HotkeyToRegistryString(m_hotkeyConfig));

    const auto& timer = m_timerConfig;
    success &= WriteDwordValue(key.Get(), L"TimerMode", static_cast<DWORD>(timer.mode));
    success &= WriteDwordValue(key.Get(), L"TimerDuration", timer.durationMinutes);
    success &= WriteSystemTimeValue(key.Get(), L"TimerUntilTime", timer.untilTime);
    success &= WriteSystemTimeValue(key.Get(), L"TimerStartTime", timer.startTime);

    const ULONGLONG endUtc = m_enabled && timer.mode != TimerMode::Indefinite
        ? timer.endTimeUtc
        : 0;
    success &= WriteQwordValue(key.Get(), L"TimerEndUtc", endUtc);

    if (success) {
        m_dirty = false;
    }
    return success;
}

bool Settings::IsAutoStartEnabled() {
    HKEY rawKey = nullptr;
    const LONG openResult = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY_PATH, 0, KEY_READ, &rawKey);
    if (openResult != ERROR_SUCCESS) {
        if (openResult != ERROR_FILE_NOT_FOUND) {
            Utils::CheckWinApiStatus(openResult, L"RegOpenKeyExW(HKCU\\Run)");
        }
        return false;
    }
    RegistryKey key(rawKey);

    DWORD type = 0;
    DWORD size = 0;
    LONG result = RegGetValueW(key.Get(), nullptr, APP_NAME,
                               RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
                               &type, nullptr, &size);
    if (result == ERROR_FILE_NOT_FOUND) {
        return false;
    }
    if (result != ERROR_SUCCESS || size < sizeof(wchar_t)) {
        if (result != ERROR_SUCCESS) {
            Utils::CheckWinApiStatus(result, L"RegGetValueW(HKCU\\Run\\Everon)");
        }
        return false;
    }

    std::vector<wchar_t> buffer((size / sizeof(wchar_t)) + 1, L'\0');
    DWORD readSize = size;
    result = RegGetValueW(key.Get(), nullptr, APP_NAME,
                          RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
                          &type, buffer.data(), &readSize);
    if (result != ERROR_SUCCESS) {
        Utils::CheckWinApiStatus(result, L"RegGetValueW(HKCU\\Run\\Everon)");
        return false;
    }
    buffer.back() = L'\0';

    auto stored = std::wstring(buffer.data());
    if (type == REG_EXPAND_SZ) {
        stored = ExpandRegistryString(stored);
    }

    const auto executable = GetExecutablePath();
    if (executable.empty()) {
        return false;
    }

    const auto configuredExecutable = ExtractExecutableToken(stored);
    return !configuredExecutable.empty() &&
           _wcsicmp(configuredExecutable.c_str(), executable.c_str()) == 0;
}

bool Settings::SetAutoStartEnabled(bool enable) {
    HKEY rawKey = nullptr;
    const LONG createResult = RegCreateKeyExW(HKEY_CURRENT_USER, RUN_KEY_PATH, 0, nullptr, 0,
                                               KEY_WRITE, nullptr, &rawKey, nullptr);
    if (!Utils::CheckWinApiStatus(createResult, L"RegCreateKeyExW(HKCU\\Run)")) {
        return false;
    }
    RegistryKey key(rawKey);

    if (!enable) {
        const LONG deleteResult = RegDeleteValueW(key.Get(), APP_NAME);
        if (deleteResult == ERROR_SUCCESS || deleteResult == ERROR_FILE_NOT_FOUND) {
            return true;
        }
        Utils::CheckWinApiStatus(deleteResult, L"RegDeleteValueW(HKCU\\Run\\Everon)");
        return false;
    }

    const auto executable = GetExecutablePath();
    if (executable.empty()) {
        return false;
    }

    const auto quotedPath = L"\"" + executable + L"\"";
    const auto size = static_cast<DWORD>((quotedPath.size() + 1) * sizeof(wchar_t));
    const LONG result = RegSetKeyValueW(key.Get(), nullptr, APP_NAME, REG_SZ,
                                        quotedPath.c_str(), size);
    return Utils::CheckWinApiStatus(result, L"RegSetKeyValueW(HKCU\\Run\\Everon)");
}

}
