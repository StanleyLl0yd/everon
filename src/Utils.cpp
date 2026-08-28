#include "Utils.h"
#include <array>
#include <format>

namespace Everon::Utils {

bool CheckWinApiBool(BOOL result, const wchar_t* apiName) {
    if (result) {
        return true;
    }

    const wchar_t* name = (apiName && *apiName) ? apiName : L"(unknown)";
    DebugLog(L"[Everon][WinAPI] {} failed. GetLastError={}\n", name, GetLastError());
    return false;
}

bool CheckWinApiStatus(LONG status, const wchar_t* apiName) {
    if (status == ERROR_SUCCESS) {
        return true;
    }

    const wchar_t* name = (apiName && *apiName) ? apiName : L"(unknown)";
    DebugLog(L"[Everon][WinAPI] {} failed. Status={}\n", name, status);
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
            default: break;
        }

        if (context && *context) {
            DebugLog(L"[Everon][WinAPI] {} failed ({}). GetLastError={}\n",
                     msgName, context, GetLastError());
        } else {
            DebugLog(L"[Everon][WinAPI] {} failed. GetLastError={}\n",
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

    const auto monitor = MonitorFromWindow(
        referenceWindow ? referenceWindow : window,
        MONITOR_DEFAULTTONEAREST
    );

    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);

    if (!GetMonitorInfoW(monitor, &monitorInfo)) {
        return;
    }

    const RECT& workArea = monitorInfo.rcWork;

    int x = workArea.left + ((workArea.right - workArea.left) - width) / 2;
    int y = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

    x = max(workArea.left, min(x, workArea.right - width));
    y = max(workArea.top, min(y, workArea.bottom - height));

    SetWindowPos(window, nullptr, x, y, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

std::wstring GetKeyName(UINT virtualKey) {
    if (virtualKey == 0) {
        return L"Off";
    }

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
        default:            break;
    }

    if (virtualKey >= VK_F1 && virtualKey <= VK_F24) {
        return std::format(L"F{}", virtualKey - VK_F1 + 1);
    }

    if ((virtualKey >= '0' && virtualKey <= '9') || (virtualKey >= 'A' && virtualKey <= 'Z')) {
        return std::wstring(1, static_cast<wchar_t>(virtualKey));
    }

    if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_NUMPAD9) {
        return std::format(L"Num{}", virtualKey - VK_NUMPAD0);
    }

    const auto scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
    std::array<wchar_t, 64> buffer{};
    if (GetKeyNameTextW(scanCode << 16, buffer.data(), static_cast<int>(buffer.size())) > 0) {
        return buffer.data();
    }

    return std::format(L"Key{:02X}", virtualKey & 0xFFU);
}

SingleInstanceGuard::SingleInstanceGuard(const wchar_t* mutexName) {
    m_mutex = CreateMutexW(nullptr, TRUE, mutexName);

    if (!m_mutex) {
        // Fail open if the single-instance mutex cannot be created.
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

}