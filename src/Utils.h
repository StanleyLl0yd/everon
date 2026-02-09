#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

namespace Everon {
namespace Utils {

// Debug logging (outputs to debugger)
#ifdef _DEBUG
void DebugLog(const wchar_t* format, ...);
#else
inline void DebugLog(const wchar_t*, ...) {}
#endif

// Unified WinAPI result checks (with consistent debug logging)
bool CheckWinApiBool(BOOL result, const wchar_t* apiName);
bool CheckWinApiStatus(LONG status, const wchar_t* apiName);
UINT_PTR SetTimerChecked(HWND window, UINT_PTR timerId, UINT intervalMs);
bool ShellNotifyIconChecked(DWORD message, PNOTIFYICONDATAW data, const wchar_t* context = nullptr);

// Center window on monitor
void CenterWindowOnMonitor(HWND window, HWND referenceWindow = nullptr);

// Get virtual key name
std::wstring GetKeyName(UINT virtualKey);

// Single instance check using mutex
class SingleInstanceGuard {
public:
    explicit SingleInstanceGuard(const wchar_t* mutexName);
    ~SingleInstanceGuard();

    bool IsFirstInstance() const noexcept { return m_isFirst; }

private:
    HANDLE m_mutex = nullptr;
    bool m_isFirst = false;
};

} // namespace Utils
} // namespace Everon
