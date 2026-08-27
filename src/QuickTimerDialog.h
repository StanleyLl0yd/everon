#pragma once

#include <windows.h>

namespace Everon {

bool ShowQuickDurationDialog(HINSTANCE instance, HWND parent, DWORD initialMinutes, DWORD& minutes);
bool ShowQuickUntilDialog(HINSTANCE instance, HWND parent, const SYSTEMTIME& initialTime, SYSTEMTIME& untilTime);

}
