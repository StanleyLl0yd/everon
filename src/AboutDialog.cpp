#include "AboutDialog.h"
#include "Localization.h"
#include "resource.h"

#include <commctrl.h>
#include <shellapi.h>

#include <bit>
#include <string>

namespace Everon {

namespace {

constexpr wchar_t kWebsiteUrl[] = L"https://stanleyll0yd.github.io/apps/everon/";
constexpr wchar_t kPrivacyUrl[] = L"https://stanleyll0yd.github.io/apps/everon/privacy/";
constexpr wchar_t kWebsiteDisplay[] = L"stanleyll0yd.github.io";

std::wstring MakeLinkText(const wchar_t* url,
                          const wchar_t* label,
                          const wchar_t* detail = nullptr) {
    std::wstring text = L"<a href=\"";
    text += url;
    text += L"\">";
    text += label;
    if (detail) {
        text += L" - ";
        text += detail;
    }
    text += L"</a>";
    return text;
}

void OpenUrl(HWND dialog, const wchar_t* url) {
    using enum StringID;

    const HINSTANCE result = ShellExecuteW(dialog, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    if (std::bit_cast<INT_PTR>(result) <= 32) {
        const auto& loc = Localization::Instance();
        MessageBoxW(dialog, url, loc.GetString(AboutTitle), MB_OK | MB_ICONINFORMATION);
    }
}

INT_PTR CALLBACK AboutDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    using enum StringID;

    const auto& loc = Localization::Instance();

    switch (message) {
        case WM_INITDIALOG: {
            SetWindowTextW(dialog, loc.GetString(AboutTitle));
            SetDlgItemTextW(dialog, IDC_ABOUT_VERSION, loc.GetString(AboutVersion));
            SetDlgItemTextW(dialog, IDC_ABOUT_TAGLINE, loc.GetString(AboutTagline));
            SetDlgItemTextW(dialog, IDC_ABOUT_AUTHOR, L"Stanley Lloyd");
            SetDlgItemTextW(dialog, IDC_ABOUT_LICENSE, loc.GetString(AboutLicense));

            const std::wstring website =
                MakeLinkText(kWebsiteUrl, loc.GetString(AboutWebsite), kWebsiteDisplay);
            const std::wstring privacy = MakeLinkText(kPrivacyUrl, loc.GetString(AboutPrivacy));
            SetDlgItemTextW(dialog, IDC_ABOUT_LINK, website.c_str());
            SetDlgItemTextW(dialog, IDC_ABOUT_PRIVACY, privacy.c_str());
            SetDlgItemTextW(dialog, IDOK, loc.GetString(ButtonOK));
            return TRUE;
        }

        case WM_NOTIFY:
            if (const auto* header = std::bit_cast<const NMHDR*>(lParam);
                header &&
                (header->idFrom == IDC_ABOUT_LINK || header->idFrom == IDC_ABOUT_PRIVACY) &&
                (header->code == NM_CLICK || header->code == NM_RETURN)) {
                const auto* link = std::bit_cast<const NMLINK*>(lParam);
                const wchar_t* fallback =
                    header->idFrom == IDC_ABOUT_PRIVACY ? kPrivacyUrl : kWebsiteUrl;
                const wchar_t* url = link->item.szUrl[0] ? link->item.szUrl : fallback;
                OpenUrl(dialog, url);
                return TRUE;
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(dialog, LOWORD(wParam));
                return TRUE;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

}

void ShowAboutDialog(HINSTANCE instance, HWND parent) {
    using enum StringID;

    const INT_PTR result = DialogBoxParamW(instance,
                                           MAKEINTRESOURCEW(IDD_ABOUT),
                                           parent,
                                           AboutDialogProc,
                                           0);
    if (result == -1) {
        const auto& loc = Localization::Instance();
        std::wstring message = loc.GetString(AboutVersion);
        message += L"\n";
        message += loc.GetString(AboutTagline);
        message += L"\n\nStanley Lloyd\n";
        message += loc.GetString(AboutLicense);
        message += L"\n\n";
        message += loc.GetString(AboutWebsite);
        message += L": ";
        message += kWebsiteUrl;
        message += L"\n";
        message += loc.GetString(AboutPrivacy);
        message += L": ";
        message += kPrivacyUrl;

        MessageBoxW(parent, message.c_str(), loc.GetString(AboutTitle),
                    MB_OK | MB_ICONINFORMATION);
    }
}

}
