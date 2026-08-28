#include "PowerManager.h"
#include "Utils.h"
#include <array>

namespace Everon {

PowerManager::~PowerManager() {
    AllowSleep();
}

bool PowerManager::PreventSleep(bool keepDisplayOn) {
    if (m_isActive && m_keepDisplayOn == keepDisplayOn) {
        return true;
    }

    EXECUTION_STATE flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
    if (keepDisplayOn) {
        flags |= ES_DISPLAY_REQUIRED;
    }

    if (SetThreadExecutionState(flags) == 0) {
        Utils::DebugLog(L"[Everon] Failed to prevent sleep: %lu\n", GetLastError());
        return false;
    }

    m_isActive = true;
    m_keepDisplayOn = keepDisplayOn;
    return true;
}

bool PowerManager::AllowSleep() {
    if (!m_isActive) {
        return true;
    }

    if (SetThreadExecutionState(ES_CONTINUOUS) == 0) {
        Utils::DebugLog(L"[Everon] Failed to restore normal sleep behavior: %lu\n", GetLastError());
        return false;
    }

    m_isActive = false;
    m_keepDisplayOn = false;
    return true;
}

bool PowerManager::SendKeyPress(WORD virtualKey) const {
    if (virtualKey == 0) {
        return true;
    }

    std::array<INPUT, 2> inputs{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = virtualKey;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = virtualKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    if (const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
        sent != static_cast<UINT>(inputs.size())) {
        Utils::DebugLog(L"[Everon] SendInput failed/sent %u: %lu\n", sent, GetLastError());
        return false;
    }
    return true;
}

}
