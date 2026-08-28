#include <iostream>
#include <cwchar>

#include "Localization.h"

namespace {

int g_failures = 0;

void Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++g_failures;
    }
}

}

int main() {
    using namespace Everon;

    auto& localization = Localization::Instance();
    for (int language = 0; language < static_cast<int>(Language::Count); ++language) {
        const auto lang = static_cast<Language>(language);
        localization.SetLanguage(lang);
        Expect(Localization::GetLanguageName(lang) != nullptr, "language name should exist");

        for (int stringId = 0; stringId < static_cast<int>(StringID::Count); ++stringId) {
            const wchar_t* value = localization.GetString(static_cast<StringID>(stringId));
            const bool hasValue = value != nullptr && *value != L'\0';
            Expect(hasValue, "localized string should not be empty");
            if (hasValue) {
                Expect(std::wcscmp(value, L"???") != 0, "localized string should resolve to a known entry");
            }
        }

        const wchar_t* code = Localization::LanguageToString(lang);
        Expect(code != nullptr, "language code should exist");
        if (code != nullptr) {
            Expect(Localization::StringToLanguage(code) == lang, "language code should round-trip");
        }
    }

    localization.SetLanguage(Language::English);

    if (g_failures == 0) {
        std::cout << "All Localization tests passed.\n";
        return 0;
    }

    return 1;
}
