#include "PowerManager.h"
#include "Utils.h"

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

void PowerManager::SendKeyPress(WORD virtualKey) {
    if (virtualKey == 0) {
        return;
    }

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = virtualKey;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = virtualKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    const UINT sent = SendInput(2, inputs, sizeof(INPUT));
    if (sent != 2) {
        Utils::DebugLog(L"[Everon] SendInput failed/sent %u: %lu\n", sent, GetLastError());
    }
}

}
