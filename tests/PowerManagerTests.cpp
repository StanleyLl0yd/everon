#include <windows.h>
#include <iostream>

#include "PowerManager.h"

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

    PowerManager manager;
    Expect(!manager.IsPreventingSleep(), "power manager should start inactive");
    Expect(manager.AllowSleep(), "allowing sleep while inactive should succeed");

    Expect(manager.PreventSleep(false), "system keep-awake request should succeed");
    Expect(manager.IsPreventingSleep(), "successful keep-awake request should update state");
    Expect(manager.PreventSleep(false), "repeating the active keep-awake request should succeed");

    Expect(manager.PreventSleep(true), "display keep-awake request should succeed");
    Expect(manager.IsPreventingSleep(), "display keep-awake should keep the manager active");

    Expect(manager.AllowSleep(), "restoring normal sleep behavior should succeed");
    Expect(!manager.IsPreventingSleep(), "successful sleep restore should clear active state");
    Expect(manager.AllowSleep(), "repeating sleep restore should succeed");

    if (g_failures == 0) {
        std::cout << "All PowerManager tests passed.\n";
        return 0;
    }

    return 1;
}
