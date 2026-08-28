#include "TimerMode.h"

namespace Everon {

static inline ULONGLONG FileTimeToUll(const FILETIME& ft) noexcept {
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}

static inline ULONGLONG NowUtcFileTimeUll() noexcept {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return FileTimeToUll(ft);
}

static inline bool LocalToUtcSystemTime(const SYSTEMTIME& local, SYSTEMTIME& utc) noexcept {
    // nullptr uses the current system time zone.
    return TzSpecificLocalTimeToSystemTime(nullptr, &local, &utc) != 0;
}

static inline ULONGLONG UtcSystemTimeToFileTimeUll(const SYSTEMTIME& utc) noexcept {
    FILETIME ft;
    if (!SystemTimeToFileTime(&utc, &ft)) {
        return 0;
    }
    return FileTimeToUll(ft);
}

static inline bool IsLeapYear(WORD year) noexcept {
    if ((year % 400) == 0) return true;
    if ((year % 100) == 0) return false;
    return (year % 4) == 0;
}

static inline WORD DaysInMonth(WORD year, WORD month) noexcept {
    switch (month) {
        case 1:  return 31;
        case 2:  return static_cast<WORD>(IsLeapYear(year) ? 29 : 28);
        case 3:  return 31;
        case 4:  return 30;
        case 5:  return 31;
        case 6:  return 30;
        case 7:  return 31;
        case 8:  return 31;
        case 9:  return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 31;
    }
}

static inline void AddDaysLocal(SYSTEMTIME& st, int days) noexcept {
    while (days-- > 0) {
        const WORD dim = DaysInMonth(st.wYear, st.wMonth);
        if (st.wDay < dim) {
            st.wDay = static_cast<WORD>(st.wDay + 1);
        } else {
            st.wDay = 1;
            if (st.wMonth < 12) {
                st.wMonth = static_cast<WORD>(st.wMonth + 1);
            } else {
                st.wMonth = 1;
                st.wYear = static_cast<WORD>(st.wYear + 1);
            }
        }
    }
}

static inline void AddMinutesLocal(SYSTEMTIME& st, int minutes) noexcept {
    int total = static_cast<int>(st.wHour) * 60 + static_cast<int>(st.wMinute) + minutes;
    while (total >= 24 * 60) {
        total -= 24 * 60;
        AddDaysLocal(st, 1);
    }
    st.wHour = static_cast<WORD>(total / 60);
    st.wMinute = static_cast<WORD>(total % 60);
}

static bool IsUntilNextDayAt(const SYSTEMTIME& nowLocal, const SYSTEMTIME& untilTime) noexcept {
    if (untilTime.wHour != nowLocal.wHour) {
        return untilTime.wHour < nowLocal.wHour;
    }
    if (untilTime.wMinute != nowLocal.wMinute) {
        return untilTime.wMinute < nowLocal.wMinute;
    }
    return nowLocal.wSecond > 0 || nowLocal.wMilliseconds > 0;
}

static ULONGLONG ComputeNextUntilUtc(const SYSTEMTIME& untilTime) noexcept {
    SYSTEMTIME nowLocal = {};
    GetLocalTime(&nowLocal);

    SYSTEMTIME targetLocal = nowLocal;
    targetLocal.wHour = untilTime.wHour;
    targetLocal.wMinute = untilTime.wMinute;
    targetLocal.wSecond = 0;
    targetLocal.wMilliseconds = 0;

    if (IsUntilNextDayAt(nowLocal, untilTime)) {
        AddDaysLocal(targetLocal, 1);
    }

    // DST transitions can make local times invalid; probe forward to the next valid minute.
    SYSTEMTIME targetUtc = {};
    SYSTEMTIME probeLocal = targetLocal;

    bool ok = LocalToUtcSystemTime(probeLocal, targetUtc);
    if (!ok) {
        for (int i = 0; i < 180 && !ok; ++i) {
            AddMinutesLocal(probeLocal, 1);
            ok = LocalToUtcSystemTime(probeLocal, targetUtc);
        }
    }

    if (!ok) {
        return 0;
    }

    return UtcSystemTimeToFileTimeUll(targetUtc);
}

static ULONGLONG ResolveTargetUtc(const TimerConfig& timer) noexcept {
    if (timer.endTimeUtc != 0) {
        return timer.endTimeUtc;
    }

    if (timer.mode == TimerMode::Duration) {
        if (timer.startTime.wYear == 0) {
            const ULONGLONG nowUtc = NowUtcFileTimeUll();
            return nowUtc + (static_cast<ULONGLONG>(timer.durationMinutes) * 60ULL * 10000000ULL);
        }

        SYSTEMTIME startUtc = {};
        if (!LocalToUtcSystemTime(timer.startTime, startUtc)) {
            const ULONGLONG nowUtc = NowUtcFileTimeUll();
            return nowUtc + (static_cast<ULONGLONG>(timer.durationMinutes) * 60ULL * 10000000ULL);
        }

        const ULONGLONG startUtcU = UtcSystemTimeToFileTimeUll(startUtc);
        if (startUtcU == 0) {
            const ULONGLONG nowUtc = NowUtcFileTimeUll();
            return nowUtc + (static_cast<ULONGLONG>(timer.durationMinutes) * 60ULL * 10000000ULL);
        }

        return startUtcU + (static_cast<ULONGLONG>(timer.durationMinutes) * 60ULL * 10000000ULL);
    }

    if (timer.mode == TimerMode::UntilTime) {
        return ComputeNextUntilUtc(timer.untilTime);
    }

    return 0;
}

static DWORD RemainingFromUtc(const TimerConfig& timer) noexcept {
    const ULONGLONG nowUtc = NowUtcFileTimeUll();
    const ULONGLONG targetUtc = ResolveTargetUtc(timer);

    if (targetUtc == 0 || targetUtc <= nowUtc) {
        return 0;
    }

    const ULONGLONG diff100ns = targetUtc - nowUtc;
    const ULONGLONG milliseconds = (diff100ns + 9999ULL) / 10000ULL;
    if (milliseconds > 0xFFFFFFFFULL) {
        return 0xFFFFFFFFUL;
    }
    return static_cast<DWORD>(milliseconds);
}

bool TimerConfig::IsValid() const noexcept {
    switch (mode) {
        case TimerMode::Indefinite:
            return true;
        case TimerMode::Duration:
            return durationMinutes >= MIN_DURATION_MIN && durationMinutes <= MAX_DURATION_MIN;
        case TimerMode::UntilTime:
            return untilTime.wHour < 24 && untilTime.wMinute < 60;
        default:
            return false;
    }
}

void TimerConfig::ResetStartTime() noexcept {
    if (mode == TimerMode::Duration) {
        GetLocalTime(&startTime);
        const ULONGLONG durationMs = static_cast<ULONGLONG>(durationMinutes) * 60ULL * 1000ULL;
        const ULONGLONG nowUtc = NowUtcFileTimeUll();
        endTimeUtc = nowUtc + (durationMs * 10000ULL);
        monotonicDeadlineMs = GetTickCount64() + durationMs;
    } else if (mode == TimerMode::UntilTime) {
        GetLocalTime(&startTime);
        endTimeUtc = ComputeNextUntilUtc(untilTime);
        monotonicDeadlineMs = 0;
    } else {
        startTime = {};
        endTimeUtc = 0;
        monotonicDeadlineMs = 0;
    }
}

void TimerConfig::ResumeMonotonicDuration() noexcept {
    if (mode != TimerMode::Duration || monotonicDeadlineMs != 0) {
        return;
    }

    const DWORD remainingMs = RemainingFromUtc(*this);
    if (remainingMs != 0) {
        monotonicDeadlineMs = GetTickCount64() + static_cast<ULONGLONG>(remainingMs);
    }
}

bool TimerConfig::IsExpired() const noexcept {
    if (mode == TimerMode::Indefinite) {
        return false;
    }
    return GetRemainingMilliseconds() == 0;
}

bool TimerConfig::IsUntilNextDay() const noexcept {
    SYSTEMTIME nowLocal = {};
    GetLocalTime(&nowLocal);
    return IsUntilNextDay(nowLocal);
}

bool TimerConfig::IsUntilNextDay(const SYSTEMTIME& nowLocal) const noexcept {
    return mode == TimerMode::UntilTime && IsUntilNextDayAt(nowLocal, untilTime);
}

DWORD TimerConfig::GetRemainingSeconds() const noexcept {
    if (mode == TimerMode::Indefinite) {
        return INFINITE;
    }

    const DWORD remainingMs = GetRemainingMilliseconds();
    if (remainingMs == 0 || remainingMs == INFINITE) {
        return remainingMs;
    }

    const ULONGLONG seconds = (static_cast<ULONGLONG>(remainingMs) + 999ULL) / 1000ULL;
    if (seconds > 0xFFFFFFFFULL) {
        return 0xFFFFFFFFUL;
    }
    return static_cast<DWORD>(seconds);
}

DWORD TimerConfig::GetRemainingMilliseconds() const noexcept {
    if (mode == TimerMode::Indefinite) {
        return INFINITE;
    }

    if (mode == TimerMode::Duration && monotonicDeadlineMs != 0) {
        const ULONGLONG nowMs = GetTickCount64();
        if (monotonicDeadlineMs <= nowMs) {
            return 0;
        }
        const ULONGLONG remainingMs = monotonicDeadlineMs - nowMs;
        return remainingMs > 0xFFFFFFFFULL
            ? 0xFFFFFFFFUL
            : static_cast<DWORD>(remainingMs);
    }

    return RemainingFromUtc(*this);
}

}
