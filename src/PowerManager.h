#pragma once

#include <windows.h>

namespace Everon {

class PowerManager {
public:
    PowerManager() = default;
    ~PowerManager();

    void PreventSleep(bool keepDisplayOn);

    void AllowSleep();

    void SendKeyPress(WORD virtualKey);

    bool IsPreventingSleep() const noexcept { return m_isActive; }

private:
    bool m_isActive = false;
    bool m_keepDisplayOn = false;
};

}
