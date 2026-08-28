#pragma once

#include <windows.h>

namespace Everon {

class PowerManager {
public:
    PowerManager() = default;
    ~PowerManager();

    PowerManager(const PowerManager&) = delete;
    PowerManager& operator=(const PowerManager&) = delete;
    PowerManager(PowerManager&&) = delete;
    PowerManager& operator=(PowerManager&&) = delete;

    bool PreventSleep(bool keepDisplayOn);
    bool AllowSleep();
    void SendKeyPress(WORD virtualKey) const;

    bool IsPreventingSleep() const noexcept { return m_isActive; }
    bool IsKeepingDisplayOn() const noexcept { return m_isActive && m_keepDisplayOn; }

private:
    bool m_isActive = false;
    bool m_keepDisplayOn = false;
};

}
