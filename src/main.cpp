#include <windows.h>
#include <Shobjidl.h>
#include "App.h"
#include "Utils.h"
#include "Localization.h"

using namespace Everon;

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE,
                    _In_ PWSTR, _In_ int) {
    // Ensure a stable AppUserModelID (Windows 7+) for consistent naming/grouping.
    // This also reduces chances of odd display names in Task Manager/notification UI.
    SetCurrentProcessExplicitAppUserModelID(L"Everon");

    Utils::SingleInstanceGuard guard(L"Local\\Everon_SingleInstance_Mutex");

    if (!guard.IsFirstInstance()) {
        // Ask the running instance to open settings.
        for (int i = 0; i < 20; ++i) {
            HWND runningWindow = FindWindowW(App::WINDOW_CLASS_NAME, nullptr);
            if (runningWindow) {
                PostMessageW(runningWindow, App::WM_SHOW_SETTINGS, 0, 0);
                break;
            }
            Sleep(100);
        }

#ifdef _DEBUG
        auto& loc = Localization::Instance();
        MessageBoxW(nullptr,
                   loc.GetString(StringID::ErrorAlreadyRunning),
                   loc.GetString(StringID::ErrorTitle),
                   MB_OK | MB_ICONINFORMATION);
#endif
        return 0;
    }

    App app(instance);
    return app.Run();
}
