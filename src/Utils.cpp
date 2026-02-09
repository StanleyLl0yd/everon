#include "Utils.h"
#include <strsafe.h>
#include <stdarg.h>

namespace Everon {
namespace Utils {

#ifdef _DEBUG
void DebugLog(const wchar_t* format, ...) {
    wchar_t buffer[512] = {};

    va_list args;
    va_start(args, format);
    StringCchVPrintfW(buffer, _countof(buffer), format, args);
    va_end(args);

    OutputDebugStringW(buffer);
}
#endif

bool CheckWinApiBool(BOOL result, const wchar_t* apiName) {
    if (result) {
        return true;
    }

    const wchar_t* name = (apiName && *apiName) ? apiName : L"(unknown)";
    DebugLog(L"[Everon][WinAPI] %s failed. GetLastError=%lu\n", name, GetLastError());
    return false;
}

bool CheckWinApiStatus(LONG status, const wchar_t* apiName) {
    if (status == ERROR_SUCCESS) {
        return true;
    }

    const wchar_t* name = (apiName && *apiName) ? apiName : L"(unknown)";
    DebugLog(L"[Everon][WinAPI] %s failed. Status=%ld\n", name, status);
    return false;
}

UINT_PTR SetTimerChecked(HWND window, UINT_PTR timerId, UINT intervalMs) {
    const UINT_PTR result = ::SetTimer(window, timerId, intervalMs, nullptr);
    if (result == 0) {
        CheckWinApiBool(FALSE, L"SetTimer");
    }
    return result;
}

bool ShellNotifyIconChecked(DWORD message, PNOTIFYICONDATAW data, const wchar_t* context) {
    const BOOL ok = Shell_NotifyIconW(message, data);
    if (!ok) {
        const wchar_t* msgName = L"Shell_NotifyIconW";
        switch (message) {
            case NIM_ADD: msgName = L"Shell_NotifyIconW(NIM_ADD)"; break;
            case NIM_MODIFY: msgName = L"Shell_NotifyIconW(NIM_MODIFY)"; break;
            case NIM_DELETE: msgName = L"Shell_NotifyIconW(NIM_DELETE)"; break;
            case NIM_SETVERSION: msgName = L"Shell_NotifyIconW(NIM_SETVERSION)"; break;
        }

        if (context && *context) {
            DebugLog(L"[Everon][WinAPI] %s failed (%s). GetLastError=%lu\n",
                    msgName, context, GetLastError());
        } else {
            DebugLog(L"[Everon][WinAPI] %s failed. GetLastError=%lu\n",
                    msgName, GetLastError());
        }
    }
    return ok != 0;
}

void CenterWindowOnMonitor(HWND window, HWND referenceWindow) {
    RECT rect = {};
    if (!GetWindowRect(window, &rect)) {
        return;
    }

    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    // Get monitor containing the reference window (or nearest to dialog)
    HMONITOR monitor = MonitorFromWindow(
        referenceWindow ? referenceWindow : window,
        MONITOR_DEFAULTTONEAREST
    );

    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);

    if (!GetMonitorInfoW(monitor, &monitorInfo)) {
        return;
    }

    // Use work area (excludes taskbar)
    const RECT& workArea = monitorInfo.rcWork;

    int x = workArea.left + ((workArea.right - workArea.left) - width) / 2;
    int y = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

    // Clamp to work area
    x = max(workArea.left, min(x, workArea.right - width));
    y = max(workArea.top, min(y, workArea.bottom - height));

    SetWindowPos(window, nullptr, x, y, 0, 0,
                SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

std::wstring GetKeyName(UINT virtualKey) {
    if (virtualKey == 0) {
        return L"Off";
    }

    // Special keys
    switch (virtualKey) {
        case VK_BACK:       return L"Backspace";
        case VK_TAB:        return L"Tab";
        case VK_RETURN:     return L"Enter";
        case VK_PAUSE:      return L"Pause";
        case VK_CAPITAL:    return L"CapsLock";
        case VK_ESCAPE:     return L"Esc";
        case VK_SPACE:      return L"Space";
        case VK_PRIOR:      return L"PageUp";
        case VK_NEXT:       return L"PageDown";
        case VK_END:        return L"End";
        case VK_HOME:       return L"Home";
        case VK_LEFT:       return L"Left";
        case VK_UP:         return L"Up";
        case VK_RIGHT:      return L"Right";
        case VK_DOWN:       return L"Down";
        case VK_SNAPSHOT:   return L"PrintScreen";
        case VK_INSERT:     return L"Insert";
        case VK_DELETE:     return L"Delete";
    }

    // F1-F24
    if (virtualKey >= VK_F1 && virtualKey <= VK_F24) {
        wchar_t buffer[8];
        swprintf_s(buffer, L"F%u", virtualKey - VK_F1 + 1);
        return buffer;
    }

    // 0-9, A-Z
    if ((virtualKey >= '0' && virtualKey <= '9') || (virtualKey >= 'A' && virtualKey <= 'Z')) {
        return std::wstring(1, static_cast<wchar_t>(virtualKey));
    }

    // Numpad
    if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_NUMPAD9) {
        wchar_t buffer[16];
        swprintf_s(buffer, L"Num%u", virtualKey - VK_NUMPAD0);
        return buffer;
    }

    // Try to get key name from system
    UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
    wchar_t buffer[64] = {};
    if (GetKeyNameTextW(scanCode << 16, buffer, 64) > 0) {
        return buffer;
    }

    // Fallback
    swprintf_s(buffer, L"Key%02X", virtualKey & 0xFFU);
    return buffer;
}

// SingleInstanceGuard implementation
SingleInstanceGuard::SingleInstanceGuard(const wchar_t* mutexName) {
    m_mutex = CreateMutexW(nullptr, TRUE, mutexName);

    if (!m_mutex) {
        // Mutex creation failed, allow app to run anyway
        m_isFirst = true;
        return;
    }

    m_isFirst = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!m_isFirst) {
        CloseHandle(m_mutex);
        m_mutex = nullptr;
    }
}

SingleInstanceGuard::~SingleInstanceGuard() {
    if (m_mutex) {
        CloseHandle(m_mutex);
    }
}

} // namespace Utils
} // namespace Everon
