#include <windows.h>
#include <shobjidl.h>
#include "App.h"
#include "Utils.h"
#include "Localization.h"

using namespace Everon;

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE,
                    _In_ PWSTR, _In_ int) {
    using enum StringID;

    SetCurrentProcessExplicitAppUserModelID(L"Everon");

    if (Utils::SingleInstanceGuard guard(L"Local\\Everon_SingleInstance_Mutex");
        !guard.IsFirstInstance()) {
        bool forwarded = false;
        for (int i = 0; i < 50; ++i) {
            if (HWND runningWindow = FindWindowW(App::WINDOW_CLASS_NAME, nullptr);
                runningWindow && PostMessageW(runningWindow, App::WM_SHOW_SETTINGS, 0, 0)) {
                forwarded = true;
                break;
            }
            Sleep(100);
        }

        if (!forwarded) {
            const auto& loc = Localization::Instance();
            MessageBoxW(nullptr,
                        loc.GetString(ErrorAlreadyRunning),
                        loc.GetString(ErrorTitle),
                        MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    App app(instance);
    return app.Run();
}
