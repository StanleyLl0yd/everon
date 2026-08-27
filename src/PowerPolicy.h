#pragma once

namespace Everon {

struct PowerContext {
    bool onBattery = false;
    bool batterySaver = false;
};

struct PowerDecision {
    bool preventSystemSleep = false;
    bool keepDisplayOn = false;
    bool pausedByBatterySaver = false;
};

inline PowerDecision EvaluatePowerPolicy(bool enabled,
                                         bool keepDisplayOn,
                                         bool respectBatterySaver,
                                         bool allowDisplayOnBattery,
                                         const PowerContext& context) noexcept {
    PowerDecision decision;
    if (!enabled) {
        return decision;
    }

    if (respectBatterySaver && context.batterySaver) {
        decision.pausedByBatterySaver = true;
        return decision;
    }

    decision.preventSystemSleep = true;
    decision.keepDisplayOn = keepDisplayOn && (!context.onBattery || allowDisplayOnBattery);
    return decision;
}

}
