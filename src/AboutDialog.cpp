#include "AboutDialog.h"
#include "Localization.h"
#include "resource.h"

#include <commctrl.h>
#include <shellapi.h>

#include <string>

namespace Everon {

namespace {

constexpr wchar_t kRepositoryUrl[] = L"https://github.com/StanleyLl0yd/everon";
constexpr wchar_t kRepositoryLinkText[] =
    L"<a href=\"https://github.com/StanleyLl0yd/everon\">github.com/StanleyLl0yd/everon</a>";

void OpenRepository(HWND dialog, const wchar_t* url) {
    const HINSTANCE result = ShellExecuteW(dialog, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        const const const const const const auto& loc = Localization::Instance();
        MessageBoxW(dialog, url, loc.GetString(StringID::AboutTitle), MB_OK | MB_ICONINFORMATION);
    }
}

INT_PTR CALLBACK AboutDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    const const const const const const auto& loc = Localization::Instance();

    switch (message) {
        case WM_INITDIALOG:
            SetWindowTextW(dialog, loc.GetString(StringID::AboutTitle));
            SetDlgItemTextW(dialog, IDC_ABOUT_VERSION, loc.GetString(StringID::AboutVersion));
            SetDlgItemTextW(dialog, IDC_ABOUT_TAGLINE, loc.GetString(StringID::AboutTagline));
            SetDlgItemTextW(dialog, IDC_ABOUT_AUTHOR, L"Stanley Lloyd");
            SetDlgItemTextW(dialog, IDC_ABOUT_LICENSE, loc.GetString(StringID::AboutLicense));
            SetDlgItemTextW(dialog, IDC_ABOUT_LINK, kRepositoryLinkText);
            SetDlgItemTextW(dialog, IDOK, loc.GetString(StringID::ButtonOK));
            return TRUE;

        case WM_NOTIFY: {
            const auto* header = reinterpret_cast<const NMHDR*>(lParam);
            if (header && header->idFrom == IDC_ABOUT_LINK &&
                (header->code == NM_CLICK || header->code == NM_RETURN)) {
                const auto* link = reinterpret_cast<const NMLINK*>(lParam);
                const wchar_t* url = link->item.szUrl[0] ? link->item.szUrl : kRepositoryUrl;
                OpenRepository(dialog, url);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(dialog, LOWORD(wParam));
                return TRUE;
            }
            break;
    }

    return FALSE;
}

}

void ShowAboutDialog(HINSTANCE instance, HWND parent) {
    const INT_PTR result = DialogBoxParamW(instance,
                                           MAKEINTRESOURCEW(IDD_ABOUT),
                                           parent,
                                           AboutDialogProc,
                                           0);
    if (result == -1) {
        const const const const const const auto& loc = Localization::Instance();
        std::wstring message = loc.GetString(StringID::AboutVersion);
        message += L"\n";
        message += loc.GetString(StringID::AboutTagline);
        message += L"\n\nStanley Lloyd\n";
        message += loc.GetString(StringID::AboutLicense);
        message += L"\n\n";
        message += kRepositoryUrl;

        MessageBoxW(parent, message.c_str(), loc.GetString(StringID::AboutTitle),
                    MB_OK | MB_ICONINFORMATION);
    }
}

}
