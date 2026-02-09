#pragma once

#include <windows.h>

namespace Everon {

enum class TimerMode {
    Indefinite,    // Бесконечно (по умолчанию)
    Duration,      // На определённое время (5 мин - 24 часа)
    UntilTime      // До указанного времени
};

struct TimerConfig {
    TimerMode mode = TimerMode::Indefinite;
    DWORD durationMinutes = 60;  // Для Duration режима (5-1440 минут)
    SYSTEMTIME untilTime = {};   // Для UntilTime режима (используются только часы/минуты)
    SYSTEMTIME startTime = {};   // Время запуска (legacy, для совместимости)
    ULONGLONG endTimeUtc = 0;    // FILETIME UTC (QWORD). 0 = не задано

    // Константы для Duration
    static constexpr DWORD MIN_DURATION_MIN = 5;
    static constexpr DWORD MAX_DURATION_MIN = 1440; // 24 часа

    bool IsValid() const noexcept;
    bool IsExpired() const noexcept;
    DWORD GetRemainingSeconds() const noexcept;
    DWORD GetRemainingMilliseconds() const noexcept;
    void ResetStartTime() noexcept;
};

} // namespace Everon
