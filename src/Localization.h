#pragma once

#include <windows.h>

namespace Everon {

enum class Language : unsigned char {
    English,
    Russian,
    French,
    German,
    Italian,
    Spanish,
    Count
};

enum class StringID : unsigned short {

    MenuEnable,
    MenuDisable,
    MenuSettings,
    MenuAbout,
    MenuExit,

    SettingsTitle,
    SettingsGeneral,
    SettingsHotkeys,
    SettingsTimer,
    SettingsLanguage,
    SettingsPeriod,
    SettingsPeriodSeconds,
    SettingsKeyPress,
    SettingsKeyPressOff,
    SettingsKeepDisplay,
    SettingsNotifyOnToggle,
    SettingsAutoStart,
    SettingsHotkeyEnable,
    SettingsHotkeyLabel,
    SettingsHotkeyNone,
    SettingsTimerIndefinite,
    SettingsTimerDuration,
    SettingsTimerUntilTime,
    SettingsTimerMinutes,
    SettingsTimerUntil,

    ButtonOK,
    ButtonCancel,
    ButtonApply,
    ButtonTest,

    AboutTitle,
    AboutVersion,
    AboutTagline,
    AboutPerfectFor,
    AboutDownloads,
    AboutPresentations,
    AboutMonitoring,
    AboutMediaPlayback,
    AboutInstructions,
    AboutLicense,

    ErrorInvalidPeriod,
    ErrorInvalidPeriodTitle,
    ErrorInvalidTimerTitle,
    ErrorInvalidTimerDuration,
    ErrorInvalidTimerUntil,
    ErrorAutoStart,
    ErrorSaveSettings,

    TooltipDisabled,
    TooltipEnabled,

    NotifyEnabled,
    NotifyDisabled,
    NotifyTimerExpired,
    NotifyHotkeyRegistered,
    NotifyHotkeyFailed,

    ErrorTrayIcon,
    ErrorAlreadyRunning,
    ErrorTitle,

    Count
};

class Localization {
public:
    static Localization& Instance();

    void SetLanguage(Language lang);
    Language GetLanguage() const { return m_currentLanguage; }

    const wchar_t* GetString(StringID id) const;

    static const wchar_t* GetLanguageName(Language lang);
    static Language DetectSystemLanguage();

    static const wchar_t* LanguageToString(Language lang);
    static Language StringToLanguage(const wchar_t* str);

private:
    Localization();
    Language m_currentLanguage = Language::English;
};

}
