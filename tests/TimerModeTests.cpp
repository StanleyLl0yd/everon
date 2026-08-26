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

    ULARGE_INTEGER value{};
    value.LowPart = nowFt.dwLowDateTime;
    value.HighPart = nowFt.dwHighDateTime;
    value.QuadPart += 2ULL * 60ULL * 10000000ULL;

    FILETIME futureFt{};
    futureFt.dwLowDateTime = value.LowPart;
    futureFt.dwHighDateTime = value.HighPart;

    SYSTEMTIME futureUtc{};
    SYSTEMTIME futureLocal{};
    FileTimeToSystemTime(&futureFt, &futureUtc);
    SystemTimeToTzSpecificLocalTime(nullptr, &futureUtc, &futureLocal);
    return futureLocal;
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
    Expect(duration.endTimeUtc != 0, "duration reset should pin an UTC end time");
    Expect(durationRemaining > 4U * 60U * 1000U,
           "fresh five-minute duration should have more than four minutes remaining");
    Expect(durationRemaining <= 5U * 60U * 1000U,
           "fresh five-minute duration should not exceed five minutes");

    TimerConfig until;
    until.mode = TimerMode::UntilTime;
    until.untilTime = TwoMinutesFromNowLocal();
    Expect(until.IsValid(), "valid until-time should pass validation");
    until.ResetStartTime();
    Expect(until.endTimeUtc != 0, "until-time reset should pin an UTC end time");
    Expect(until.GetRemainingMilliseconds() > 0,
           "future until-time should have time remaining");

    TimerConfig invalidUntil = until;
    invalidUntil.untilTime.wHour = 24;
    Expect(!invalidUntil.IsValid(), "hour 24 should be invalid");
    invalidUntil = until;
    invalidUntil.untilTime.wMinute = 60;
    Expect(!invalidUntil.IsValid(), "minute 60 should be invalid");

    if (g_failures == 0) {
        std::cout << "All TimerMode tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
