#include <windows.h>
#include <iostream>

#include "Settings.h"

namespace {

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++g_failures;
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

    if (g_failures == 0) {
        std::cout << "All Settings tests passed.\n";
        return 0;
    }

    return 1;
}
