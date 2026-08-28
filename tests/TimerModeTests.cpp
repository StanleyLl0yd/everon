#include <windows.h>
#include <iostream>
#include "TimerMode.h"

namespace {

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++g_failures;
    }
}

SYSTEMTIME TwoMinutesFromNowLocal() {
    FILETIME nowFt{};
    GetSystemTimeAsFileTime(&nowFt);

    const ULONGLONG nowValue =
        (static_cast<ULONGLONG>(nowFt.dwHighDateTime) << 32U) |
        static_cast<ULONGLONG>(nowFt.dwLowDateTime);
    const ULONGLONG futureValue = nowValue + 2ULL * 60ULL * 10000000ULL;

    FILETIME futureFt{};
    futureFt.dwLowDateTime = static_cast<DWORD>(futureValue & 0xFFFFFFFFULL);
    futureFt.dwHighDateTime = static_cast<DWORD>(futureValue >> 32U);

    SYSTEMTIME futureUtc{};
    SYSTEMTIME futureLocal{};
    FileTimeToSystemTime(&futureFt, &futureUtc);
    SystemTimeToTzSpecificLocalTime(nullptr, &futureUtc, &futureLocal);
    return futureLocal;
}

bool IdentityLocalToUtc(const SYSTEMTIME& local, SYSTEMTIME& utc) noexcept {
    utc = local;
    return true;
}

bool SpringGapLocalToUtc(const SYSTEMTIME& local, SYSTEMTIME& utc) noexcept {
    if (local.wHour == 2) {
        return false;
    }
    utc = local;
    return true;
}

bool RejectLocalToUtc(const SYSTEMTIME&, SYSTEMTIME&) noexcept {
    return false;
}

SYSTEMTIME FileTimeUllToSystemTime(ULONGLONG value) {
    ULARGE_INTEGER integer{};
    integer.QuadPart = value;
    FILETIME fileTime{};
    fileTime.dwLowDateTime = integer.LowPart;
    fileTime.dwHighDateTime = integer.HighPart;
    SYSTEMTIME result{};
    FileTimeToSystemTime(&fileTime, &result);
    return result;
}

}

int main() {
    using namespace Everon;

    TimerConfig indefinite;
    Expect(indefinite.IsValid(), "indefinite mode should be valid");
    Expect(!indefinite.IsExpired(), "indefinite mode should never expire");
    Expect(indefinite.GetRemainingSeconds() == INFINITE,
           "indefinite remaining seconds should be INFINITE");

    TimerConfig duration;
    duration.mode = TimerMode::Duration;
    duration.durationMinutes = TimerConfig::MIN_DURATION_MIN;
    Expect(duration.IsValid(), "minimum duration should be valid");

    duration.durationMinutes = TimerConfig::MIN_DURATION_MIN - 1;
    Expect(!duration.IsValid(), "duration below minimum should be invalid");

    duration.durationMinutes = TimerConfig::MAX_DURATION_MIN + 1;
    Expect(!duration.IsValid(), "duration above maximum should be invalid");

    duration.durationMinutes = 5;
    duration.ResetStartTime();
    const DWORD durationRemaining = duration.GetRemainingMilliseconds();
    Expect(duration.endTimeUtc != 0, "duration reset should pin a UTC end time");
    Expect(duration.monotonicDeadlineMs != 0, "duration reset should pin a monotonic deadline");
    Expect(durationRemaining > 4U * 60U * 1000U,
           "fresh five-minute duration should have more than four minutes remaining");
    Expect(durationRemaining <= 5U * 60U * 1000U,
           "fresh five-minute duration should not exceed five minutes");

    TimerConfig monotonic = duration;
    monotonic.endTimeUtc = 1;
    Expect(monotonic.GetRemainingMilliseconds() > 0,
           "active duration should prefer the monotonic deadline over wall-clock UTC");

    TimerConfig resumed = duration;
    resumed.monotonicDeadlineMs = 0;
    resumed.ResumeMonotonicDuration();
    Expect(resumed.monotonicDeadlineMs != 0,
           "persisted duration should resume with a monotonic deadline");
    Expect(resumed.GetRemainingMilliseconds() > 0,
           "resumed duration should retain time remaining");

    TimerConfig expired;
    expired.mode = TimerMode::Duration;
    expired.durationMinutes = 5;
    expired.endTimeUtc = 1;
    Expect(expired.IsExpired(), "past persisted duration should be expired");
    expired.ResumeMonotonicDuration();
    Expect(expired.monotonicDeadlineMs == 0,
           "expired persisted duration should not create a monotonic deadline");

    TimerConfig until;
    until.mode = TimerMode::UntilTime;
    until.untilTime = TwoMinutesFromNowLocal();
    Expect(until.IsValid(), "valid until-time should pass validation");
    until.ResetStartTime();
    Expect(until.endTimeUtc != 0, "until-time reset should pin a UTC end time");
    Expect(until.GetRemainingMilliseconds() > 0,
           "future until-time should have time remaining");

    SYSTEMTIME fixedNow{};
    fixedNow.wYear = 2026;
    fixedNow.wMonth = 8;
    fixedNow.wDay = 26;
    fixedNow.wHour = 20;
    fixedNow.wMinute = 30;
    fixedNow.wSecond = 15;

    TimerConfig dayCheck;
    dayCheck.mode = TimerMode::UntilTime;
    dayCheck.untilTime.wHour = 20;
    dayCheck.untilTime.wMinute = 29;
    Expect(dayCheck.IsUntilNextDay(fixedNow), "earlier time-of-day should resolve to tomorrow");

    dayCheck.untilTime.wMinute = 31;
    Expect(!dayCheck.IsUntilNextDay(fixedNow), "later time-of-day should resolve to today");

    dayCheck.untilTime.wMinute = 30;
    Expect(dayCheck.IsUntilNextDay(fixedNow), "elapsed current minute should resolve to tomorrow");

    fixedNow.wSecond = 0;
    fixedNow.wMilliseconds = 0;
    Expect(!dayCheck.IsUntilNextDay(fixedNow), "exact current minute should resolve to today");

    fixedNow.wHour = 23;
    fixedNow.wMinute = 59;
    dayCheck.untilTime.wHour = 0;
    dayCheck.untilTime.wMinute = 0;
    Expect(dayCheck.IsUntilNextDay(fixedNow), "midnight after 23:59 should resolve to tomorrow");

    fixedNow.wHour = 0;
    fixedNow.wMinute = 0;
    dayCheck.untilTime.wHour = 23;
    dayCheck.untilTime.wMinute = 59;
    Expect(!dayCheck.IsUntilNextDay(fixedNow), "23:59 after midnight should resolve to today");

    TimerConfig invalidUntil = until;
    invalidUntil.untilTime.wHour = 24;
    Expect(!invalidUntil.IsValid(), "hour 24 should be invalid");
    invalidUntil = until;
    invalidUntil.untilTime.wMinute = 60;
    Expect(!invalidUntil.IsValid(), "minute 60 should be invalid");

    SYSTEMTIME springNow{};
    springNow.wYear = 2026;
    springNow.wMonth = 3;
    springNow.wDay = 29;
    springNow.wHour = 1;
    springNow.wMinute = 30;
    SYSTEMTIME springUntil{};
    springUntil.wHour = 2;
    springUntil.wMinute = 30;
    const ULONGLONG springUtc = ComputeNextUntilUtcForTesting(
        springNow, springUntil, SpringGapLocalToUtc);
    const SYSTEMTIME springResolved = FileTimeUllToSystemTime(springUtc);
    Expect(springUtc != 0, "DST gap should resolve to the next valid minute");
    Expect(springResolved.wHour == 3 && springResolved.wMinute == 0,
           "DST gap should advance 02:30 to 03:00 when the skipped hour is invalid");

    SYSTEMTIME fallNow{};
    fallNow.wYear = 2026;
    fallNow.wMonth = 10;
    fallNow.wDay = 25;
    fallNow.wHour = 0;
    fallNow.wMinute = 30;
    SYSTEMTIME fallUntil{};
    fallUntil.wHour = 1;
    fallUntil.wMinute = 30;
    const SYSTEMTIME fallResolved = FileTimeUllToSystemTime(
        ComputeNextUntilUtcForTesting(fallNow, fallUntil, IdentityLocalToUtc));
    Expect(fallResolved.wHour == 1 && fallResolved.wMinute == 30,
           "valid repeated-hour time should not be shifted by DST gap recovery");

    SYSTEMTIME yearEnd{};
    yearEnd.wYear = 2026;
    yearEnd.wMonth = 12;
    yearEnd.wDay = 31;
    yearEnd.wHour = 23;
    yearEnd.wMinute = 59;
    SYSTEMTIME midnight{};
    midnight.wHour = 0;
    midnight.wMinute = 0;
    const SYSTEMTIME nextYear = FileTimeUllToSystemTime(
        ComputeNextUntilUtcForTesting(yearEnd, midnight, IdentityLocalToUtc));
    Expect(nextYear.wYear == 2027 && nextYear.wMonth == 1 && nextYear.wDay == 1,
           "until-time date arithmetic should cross year boundaries");

    Expect(ComputeNextUntilUtcForTesting(springNow, springUntil, RejectLocalToUtc) == 0,
           "timer conversion should fail closed when no valid local time can be resolved");

    if (g_failures == 0) {
        std::cout << "All TimerMode tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
