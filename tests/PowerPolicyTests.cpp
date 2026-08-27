#include <iostream>

#include "PowerPolicy.h"

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

    PowerContext ac{};
    PowerDecision decision = EvaluatePowerPolicy(true, true, false, true, ac);
    Expect(decision.preventSystemSleep, "enabled app should prevent system sleep on AC");
    Expect(decision.keepDisplayOn, "display keep-awake should remain active on AC");
    Expect(!decision.pausedByBatterySaver, "normal AC operation should not be paused");

    PowerContext battery{};
    battery.onBattery = true;
    decision = EvaluatePowerPolicy(true, true, false, false, battery);
    Expect(decision.preventSystemSleep, "system keep-awake should remain active on battery");
    Expect(!decision.keepDisplayOn, "display keep-awake should be suppressed on battery when disabled by policy");

    decision = EvaluatePowerPolicy(true, true, false, true, battery);
    Expect(decision.keepDisplayOn, "display keep-awake should remain active on battery when allowed");

    PowerContext saver{};
    saver.onBattery = true;
    saver.batterySaver = true;
    decision = EvaluatePowerPolicy(true, true, true, true, saver);
    Expect(!decision.preventSystemSleep, "Battery Saver policy should release system keep-awake");
    Expect(!decision.keepDisplayOn, "Battery Saver policy should release display keep-awake");
    Expect(decision.pausedByBatterySaver, "Battery Saver pause should be observable");

    decision = EvaluatePowerPolicy(true, true, false, true, saver);
    Expect(decision.preventSystemSleep, "Battery Saver should be ignored when policy is disabled");
    Expect(decision.keepDisplayOn, "display keep-awake should remain active when Battery Saver is ignored");

    decision = EvaluatePowerPolicy(false, true, true, true, saver);
    Expect(!decision.preventSystemSleep && !decision.keepDisplayOn,
           "disabled app should never request keep-awake state");

    if (g_failures == 0) {
        std::cout << "All PowerPolicy tests passed.\n";
        return 0;
    }

    return 1;
}
