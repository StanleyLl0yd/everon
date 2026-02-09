#pragma once

#include <windows.h>

namespace Everon {

// Supported languages
enum class Language : unsigned char {
    English,
    Russian,
    French,
    German,
    Italian,
    Spanish,
    Count
};

// String IDs for localization
enum class StringID : unsigned short {
    // Menu items
    MenuEnable,
    MenuDisable,
    MenuSettings,
    MenuAbout,
    MenuExit,

    // Settings dialog
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

    // Buttons
    ButtonOK,
    ButtonCancel,
    ButtonApply,
    ButtonTest,

    // About dialog
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

    // Validation messages
    ErrorInvalidPeriod,
    ErrorInvalidPeriodTitle,
    ErrorInvalidTimerTitle,
    ErrorInvalidTimerDuration,
    ErrorInvalidTimerUntil,
    ErrorAutoStart,
    ErrorSaveSettings,

    // Tooltips
    TooltipDisabled,
    TooltipEnabled,

    // Notifications
    NotifyEnabled,
    NotifyDisabled,
    NotifyTimerExpired,
    NotifyHotkeyRegistered,
    NotifyHotkeyFailed,

    // Errors
    ErrorTrayIcon,
    ErrorAlreadyRunning,
    ErrorTitle,

    Count
};

// Localization manager (zero-alloc, static tables)
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

} // namespace Everon
