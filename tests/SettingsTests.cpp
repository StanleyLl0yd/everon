#include <windows.h>
#include <format>
#include <iostream>
#include <string>

#include "Localization.h"
#include "Settings.h"

namespace {

int& Failures() {
    static int failures = 0;
    return failures;
}

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++Failures();
    }
}

void DeleteTestKey(const std::wstring& path) {
    const LONG result = RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str());
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        std::cerr << "FAILED: unable to remove test registry key\n";
        ++Failures();
    }
}

}

int main() {
    using namespace Everon;

    Settings settings;
    settings.SetDirty(false);

    settings.SetPeriodSec(120);
    Expect(settings.GetPeriodSec() == 120, "valid key period should be accepted");
    Expect(settings.IsDirty(), "changing key period should mark settings dirty");

    settings.SetDirty(false);
    settings.SetPeriodSec(0);
    Expect(settings.GetPeriodSec() == 120, "period below minimum should be rejected");
    Expect(!settings.IsDirty(), "rejected period should not mark settings dirty");

    settings.SetPeriodSec(Settings::MAX_PERIOD_SEC + 1);
    Expect(settings.GetPeriodSec() == 120, "period above maximum should be rejected");

    settings.SetVirtualKey(VK_F16);
    Expect(settings.GetVirtualKey() == VK_F16, "F16 should be accepted for synthetic input");

    settings.SetDirty(false);
    settings.SetVirtualKey(VK_F14);
    Expect(settings.GetVirtualKey() == VK_F16, "unsupported synthetic key should be rejected");
    Expect(!settings.IsDirty(), "rejected synthetic key should not mark settings dirty");

    settings.SetKeepDisplayOn(true);
    Expect(settings.GetKeepDisplayOn(), "display keep-awake should be configurable");

    settings.SetRespectBatterySaver(true);
    Expect(settings.GetRespectBatterySaver(), "Battery Saver behavior should be configurable");

    settings.SetAllowDisplayOnBattery(false);
    Expect(!settings.GetAllowDisplayOnBattery(), "display behavior on battery should be configurable");

    settings.SetShowToggleNotifications(true);
    Expect(settings.GetShowToggleNotifications(), "toggle notifications should be configurable");

    settings.SetEnabled(false);
    Expect(!settings.IsEnabled(), "enabled state should be configurable");

    HotkeyConfig hotkey;
    hotkey.enabled = true;
    hotkey.modifiers = MOD_CONTROL | MOD_SHIFT;
    hotkey.virtualKey = 'E';
    settings.SetHotkeyConfig(hotkey);
    Expect(settings.GetHotkeyConfig() == hotkey, "hotkey configuration should round-trip in memory");

    TimerConfig timer;
    timer.mode = TimerMode::Duration;
    timer.durationMinutes = 30;
    timer.endTimeUtc = 123;
    timer.monotonicDeadlineMs = 456;
    settings.SetTimerConfig(timer);
    Expect(settings.GetTimerConfig().mode == TimerMode::Duration,
           "timer mode should be configurable");
    Expect(settings.GetTimerConfig().endTimeUtc == 123,
           "timer UTC deadline should be retained");
    Expect(settings.GetTimerConfig().monotonicDeadlineMs == 456,
           "timer runtime deadline should be retained in memory");

    settings.SetDirty(false);
    settings.SetTimerRuntimeDeadline(789);
    Expect(settings.GetTimerConfig().monotonicDeadlineMs == 789,
           "runtime deadline should be replaceable without changing persisted timer configuration");
    Expect(!settings.IsDirty(), "runtime deadline should not mark persistent settings dirty");

    Settings snapshot = settings;
    Settings staged = settings;
    staged.SetKeepDisplayOn(!snapshot.GetKeepDisplayOn());
    Expect(settings.GetKeepDisplayOn() == snapshot.GetKeepDisplayOn(),
           "editing a staged settings copy should not mutate the live snapshot");

    const std::wstring testPath = std::format(
        L"Software\\Everon\\Tests\\Settings-{}", GetCurrentProcessId());
    DeleteTestKey(testPath);

    Settings persisted(testPath.c_str());
    persisted.SetPeriodSec(321);
    persisted.SetVirtualKey(VK_F17);
    persisted.SetKeepDisplayOn(true);
    persisted.SetRespectBatterySaver(true);
    persisted.SetAllowDisplayOnBattery(false);
    persisted.SetShowToggleNotifications(true);
    persisted.SetEnabled(true);
    persisted.SetLanguage(Language::German);
    persisted.SetHotkeyConfig(hotkey);

    TimerConfig persistedTimer;
    persistedTimer.mode = TimerMode::Duration;
    persistedTimer.durationMinutes = 45;
    persistedTimer.ResetStartTime();
    persisted.SetTimerConfig(persistedTimer);

    Expect(persisted.SaveToRegistry(), "settings should save to an isolated registry key");
    Expect(!persisted.IsDirty(), "successful registry save should clear dirty state");

    Settings loaded(testPath.c_str());
    Expect(loaded.LoadFromRegistry(), "settings should load from an isolated registry key");
    Expect(loaded.GetPeriodSec() == 321, "registry round-trip should preserve period");
    Expect(loaded.GetVirtualKey() == VK_F17, "registry round-trip should preserve synthetic key");
    Expect(loaded.GetKeepDisplayOn(), "registry round-trip should preserve display policy");
    Expect(loaded.GetRespectBatterySaver(), "registry round-trip should preserve Battery Saver policy");
    Expect(!loaded.GetAllowDisplayOnBattery(), "registry round-trip should preserve battery display policy");
    Expect(loaded.GetShowToggleNotifications(), "registry round-trip should preserve notification policy");
    Expect(loaded.IsEnabled(), "registry round-trip should preserve enabled state");
    Expect(loaded.GetLanguage() == Language::German, "registry round-trip should preserve language");
    Expect(loaded.GetHotkeyConfig() == hotkey, "registry round-trip should preserve hotkey");
    Expect(loaded.GetTimerConfig().mode == TimerMode::Duration,
           "registry round-trip should preserve timer mode");
    Expect(loaded.GetTimerConfig().durationMinutes == 45,
           "registry round-trip should preserve timer duration");
    Expect(loaded.GetTimerConfig().endTimeUtc != 0,
           "registry round-trip should preserve active timer deadline");

    DeleteTestKey(testPath);
    HKEY corruptKey = nullptr;
    const LONG createResult = RegCreateKeyExW(HKEY_CURRENT_USER, testPath.c_str(), 0, nullptr, 0,
                                               KEY_WRITE, nullptr, &corruptKey, nullptr);
    Expect(createResult == ERROR_SUCCESS, "corrupt registry fixture should be created");
    if (createResult == ERROR_SUCCESS) {
        const wchar_t badPeriod[] = L"not-a-number";
        RegSetKeyValueW(corruptKey, nullptr, L"PeriodSec", REG_SZ, badPeriod, sizeof(badPeriod));

        const DWORD oversizedKey = 0x10000U | VK_F17;
        RegSetKeyValueW(corruptKey, nullptr, L"VkKey", REG_DWORD,
                        &oversizedKey, sizeof(oversizedKey));

        const DWORD invalidMode = 99;
        RegSetKeyValueW(corruptKey, nullptr, L"TimerMode", REG_DWORD,
                        &invalidMode, sizeof(invalidMode));

        const wchar_t invalidHotkey[] = L"1,2147483648,69";
        RegSetKeyValueW(corruptKey, nullptr, L"Hotkey", REG_SZ,
                        invalidHotkey, sizeof(invalidHotkey));
        RegCloseKey(corruptKey);
    }

    Settings corrupted(testPath.c_str());
    Expect(corrupted.LoadFromRegistry(), "corrupt registry values should fail closed without aborting load");
    Expect(corrupted.GetPeriodSec() == Settings::DEFAULT_PERIOD_SEC,
           "wrong registry type should preserve default period");
    Expect(corrupted.GetVirtualKey() == 0,
           "oversized registry virtual key should not be narrowed into a valid key");
    Expect(corrupted.GetTimerConfig().mode == TimerMode::Indefinite,
           "invalid persisted timer mode should reset timer configuration");
    Expect(!corrupted.GetHotkeyConfig().enabled,
           "invalid persisted hotkey should be disabled");

    DeleteTestKey(testPath);
    Localization::Instance().SetLanguage(Language::English);

    if (Failures() == 0) {
        std::cout << "All Settings tests passed.\n";
        return 0;
    }

    return 1;
}
