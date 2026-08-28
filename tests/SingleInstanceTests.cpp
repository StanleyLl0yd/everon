#include <windows.h>
#include <format>
#include <iostream>
#include <string>

#include "Utils.h"

int main() {
    using namespace Everon;

    const std::wstring mutexName = std::format(
        L"Local\\Everon_Test_Mutex_{}", GetCurrentProcessId());

    Utils::SingleInstanceGuard first(mutexName.c_str());
    if (const bool firstOwnsMutex = first.IsFirstInstance(); !firstOwnsMutex) {
        std::cerr << "FAILED: first guard should own a fresh mutex\n";
        return 1;
    }

    if (Utils::SingleInstanceGuard second(mutexName.c_str()); second.IsFirstInstance()) {
        std::cerr << "FAILED: second guard should detect the existing mutex\n";
        return 1;
    }

    std::cout << "All SingleInstance tests passed.\n";
    return 0;
}
