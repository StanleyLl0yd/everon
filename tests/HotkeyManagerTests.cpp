#include <windows.h>
#include <iostream>
#include "HotkeyManager.h"

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
}

int main() {
    using namespace Everon;

    HotkeyConfig original;
    original.enabled = true;
    original.modifiers = MOD_CONTROL | MOD_SHIFT;
    original.virtualKey = 'E';

    const std::wstring serialized = HotkeyManager::HotkeyToRegistryString(original);
    const HotkeyConfig roundTrip = HotkeyManager::StringToHotkey(serialized.c_str());
    Expect(roundTrip == original, "hotkey registry round-trip should preserve values");
    Expect(roundTrip.IsValid(), "normal hotkey should be valid");

    const HotkeyConfig malformed = HotkeyManager::StringToHotkey(L"not-a-hotkey");
    Expect(!malformed.enabled && !malformed.IsValid(), "malformed hotkey should fail closed");

    HotkeyConfig badModifier = original;
    badModifier.modifiers |= 0x80000000U;
    Expect(!badModifier.IsValid(), "unknown modifier bits should be rejected");

    HotkeyConfig badKey = original;
    badKey.virtualKey = 0x100U;
    Expect(!badKey.IsValid(), "virtual keys outside one byte should be rejected");

    if (Failures() == 0) {
        std::cout << "All HotkeyManager tests passed.\n";
        return 0;
    }
    return 1;
}
