#include "PowerManager.h"
#include "Utils.h"

namespace Everon {

PowerManager::~PowerManager() {
    AllowSleep();
}

void PowerManager::PreventSleep(bool keepDisplayOn) {
    if (m_isActive && m_keepDisplayOn == keepDisplayOn) {
        return;
    }

    EXECUTION_STATE flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
    if (keepDisplayOn) {
        flags |= ES_DISPLAY_REQUIRED;
    }

    EXECUTION_STATE result = SetThreadExecutionState(flags);
    if (result == 0) {
        Utils::DebugLog(L"[Everon] Failed to prevent sleep: %lu\n", GetLastError());
        return;
    }

    m_isActive = true;
    m_keepDisplayOn = keepDisplayOn;
}


void PowerManager::AllowSleep() {
    if (!m_isActive) {
        return;
    }
    const EXECUTION_STATE prev = SetThreadExecutionState(ES_CONTINUOUS);
    if (prev == 0) {
		Utils::DebugLog(L"[Everon] SetThreadExecutionState(ES_CONTINUOUS) failed: %lu\n", GetLastError());
    }
    m_isActive = false;
    m_keepDisplayOn = false;
}

void PowerManager::SendKeyPress(WORD virtualKey) {
    if (virtualKey == 0) {
        return;
    }

    INPUT inputs[2] = {};

    // Key down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = virtualKey;
    inputs[0].ki.dwFlags = 0;

    // Key up
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = virtualKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    const UINT sent = SendInput(2, inputs, sizeof(INPUT));
    if (sent != 2) {
		Utils::DebugLog(L"[Everon] SendInput failed/sent %u: %lu\n", sent, GetLastError());
    }
}

} // namespace Everon
