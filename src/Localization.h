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
    MenuQuickTimer,
    MenuTimer15,
    MenuTimer30,
    MenuTimer60,
    MenuTimer120,
    MenuTimerUntil,

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
    SettingsHotkeyLabel,
    SettingsHotkeyNone,
    SettingsTimerIndefinite,
    SettingsTimerDuration,
    SettingsTimerUntilTime,
    SettingsTimerMinutes,
    SettingsTimerUntil,
    SettingsTimerTomorrow,

    ButtonOK,
    ButtonCancel,

    AboutTitle,
    AboutVersion,
    AboutTagline,
    AboutLicense,

    ErrorInvalidPeriod,
    ErrorInvalidPeriodTitle,
    ErrorInvalidTimerTitle,
    ErrorInvalidTimerDuration,
    ErrorAutoStart,
    ErrorSaveSettings,
    ErrorPowerState,

    TooltipDisabled,
    TooltipEnabled,

    NotifyEnabled,
    NotifyDisabled,
    NotifyTimerExpired,
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
