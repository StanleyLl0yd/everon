#pragma once

#include <windows.h>

namespace Everon {

enum class TimerMode {
    Indefinite,
    Duration,
    UntilTime
};

struct TimerConfig {
    TimerMode mode = TimerMode::Indefinite;
    DWORD durationMinutes = 60;
    SYSTEMTIME untilTime = {}; // Only hour and minute are used.
    SYSTEMTIME startTime = {}; // Legacy local start time kept for backward compatibility.
    ULONGLONG endTimeUtc = 0; // Persisted UTC deadline; 0 means unset.
    ULONGLONG monotonicDeadlineMs = 0;

    static constexpr DWORD MIN_DURATION_MIN = 5;
    static constexpr DWORD MAX_DURATION_MIN = 24 * 60;

    bool IsValid() const noexcept;
    bool IsExpired() const noexcept;
    bool IsUntilNextDay() const noexcept;
    bool IsUntilNextDay(const SYSTEMTIME& nowLocal) const noexcept;
    DWORD GetRemainingSeconds() const noexcept;
    DWORD GetRemainingMilliseconds() const noexcept;
    void ResetStartTime() noexcept;
    void ResumeMonotonicDuration() noexcept;
};

}
