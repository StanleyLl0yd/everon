#pragma once

#include <windows.h>
#include <shellapi.h>
#include <format>
#include <string>
#include <utility>

namespace Everon::Utils {

template <typename... Args>
void DebugLog(std::wformat_string<Args...> format, Args&&... args) {
#ifdef _DEBUG
    const auto message = std::format(format, std::forward<Args>(args)...);
    OutputDebugStringW(message.c_str());
#else
    (void)format;
    ((void)args, ...);
#endif
}

bool CheckWinApiBool(BOOL result, const wchar_t* apiName);
bool CheckWinApiStatus(LONG status, const wchar_t* apiName);
UINT_PTR SetTimerChecked(HWND window, UINT_PTR timerId, UINT intervalMs);
bool ShellNotifyIconChecked(DWORD message, PNOTIFYICONDATAW data, const wchar_t* context = nullptr);

void CenterWindowOnMonitor(HWND window, HWND referenceWindow = nullptr);

std::wstring GetKeyName(UINT virtualKey);

class SingleInstanceGuard {
public:
    explicit SingleInstanceGuard(const wchar_t* mutexName);
    ~SingleInstanceGuard();

    SingleInstanceGuard(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard(SingleInstanceGuard&&) = delete;
    SingleInstanceGuard& operator=(SingleInstanceGuard&&) = delete;

    bool IsFirstInstance() const noexcept { return m_isFirst; }

private:
    HANDLE m_mutex = nullptr;
    bool m_isFirst = false;
};

}