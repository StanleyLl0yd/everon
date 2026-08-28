#include <windows.h>
#include <iostream>
#include <string>

#include "Utils.h"

int main() {
    using namespace Everon;

    const std::wstring mutexName = L"Local\\Everon_Test_Mutex_" +
                                   std::to_wstring(GetCurrentProcessId());

    Utils::SingleInstanceGuard first(mutexName.c_str());
    if (!first.IsFirstInstance()) {
        std::cerr << "FAILED: first guard should own a fresh mutex\n";
        return 1;
    }

    Utils::SingleInstanceGuard second(mutexName.c_str());
    if (second.IsFirstInstance()) {
        std::cerr << "FAILED: second guard should detect the existing mutex\n";
        return 1;
    }

    std::cout << "All SingleInstance tests passed.\n";
    return 0;
}
