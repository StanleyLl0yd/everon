#pragma once

#include <windows.h>

namespace Everon {

// Manages power state and prevents system sleep
class PowerManager {
public:
    PowerManager() = default;
    ~PowerManager();

    // Prevent system sleep
    void PreventSleep(bool keepDisplayOn);

    // Allow system sleep
    void AllowSleep();

    // Send virtual key press
    void SendKeyPress(WORD virtualKey);

    // Check if currently preventing sleep
    bool IsPreventingSleep() const noexcept { return m_isActive; }

private:
    bool m_isActive = false;
    bool m_keepDisplayOn = false;
};

} // namespace Everon
