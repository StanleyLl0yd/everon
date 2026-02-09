#include "Localization.h"
#include "Version.h"

namespace Everon {

namespace {
constexpr int kLangCount = static_cast<int>(Language::Count);
constexpr int kStrCount  = static_cast<int>(StringID::Count);

static const wchar_t* const kLanguageNames[kLangCount] = {
    L"English",
    L"Русский",
    L"Français",
    L"Deutsch",
    L"Italiano",
    L"Español"
};

static const wchar_t* const kLanguageCodes[kLangCount] = {
    L"en", L"ru", L"fr", L"de", L"it", L"es"
};

// IMPORTANT: row order must match StringID enum order.
static const wchar_t* const kStrings[kStrCount][kLangCount] = {
    // Menu items
    { L"Enable",   L"Включить",   L"Activer",     L"Aktivieren",  L"Attiva",     L"Activar" },     // MenuEnable
    { L"Disable",  L"Отключить",  L"Désactiver",  L"Deaktivieren",L"Disattiva",  L"Desactivar" },  // MenuDisable
    { L"Settings", L"Настройки",  L"Paramètres",  L"Einstellungen",L"Impostazioni",L"Configuración" }, // MenuSettings
    { L"About",    L"О программе",L"À propos",    L"Über",        L"Informazioni",L"Acerca de" },   // MenuAbout
    { L"Exit",     L"Выход",      L"Quitter",     L"Beenden",     L"Esci",       L"Salir" },       // MenuExit

    // Settings dialog
    { L"Everon Settings", L"Настройки Everon", L"Paramètres Everon", L"Everon Einstellungen", L"Impostazioni Everon", L"Configuración de Everon" }, // SettingsTitle
    { L"General", L"Общие", L"Général", L"Allgemein", L"Generale", L"General" }, // SettingsGeneral
    { L"Hotkeys", L"Горячие клавиши", L"Raccourcis", L"Tastenkombinationen", L"Tasti rapidi", L"Atajos" }, // SettingsHotkeys
    { L"Timer", L"Таймер", L"Minuteur", L"Timer", L"Timer", L"Temporizador" }, // SettingsTimer
    { L"Language:", L"Язык:", L"Langue:", L"Sprache:", L"Lingua:", L"Idioma:" }, // SettingsLanguage
    { L"Period:", L"Период:", L"Période:", L"Periode:", L"Periodo:", L"Período:" }, // SettingsPeriod
    { L"seconds", L"секунд", L"secondes", L"Sekunden", L"secondi", L"segundos" }, // SettingsPeriodSeconds
    { L"Key press:", L"Нажатие клавиши:", L"Touche:", L"Taste:", L"Tasto:", L"Tecla:" }, // SettingsKeyPress
    { L"Off (no SendInput)", L"Выкл (без SendInput)", L"Désactivé (pas de SendInput)", L"Aus (kein SendInput)", L"Disattivato (nessun SendInput)", L"Desactivado (sin SendInput)" }, // SettingsKeyPressOff
    { L"Keep display on", L"Не выключать экран", L"Garder l'écran allumé", L"Display eingeschaltet lassen", L"Mantieni schermo acceso", L"Mantener pantalla encendida" }, // SettingsKeepDisplay
    { L"Show notifications on Enable/Disable", L"Показывать уведомления при вкл/выкл", L"Afficher des notifications à l'activation/désactivation", L"Benachrichtigungen bei Ein/Aus anzeigen", L"Mostra notifiche su attiva/disattiva", L"Mostrar notificaciones al activar/desactivar" }, // SettingsNotifyOnToggle
    { L"Start with Windows", L"Запускать с Windows", L"Démarrer avec Windows", L"Mit Windows starten", L"Avvia con Windows", L"Iniciar con Windows" }, // SettingsAutoStart
    { L"Enable hotkey", L"Включить горячую клавишу", L"Activer le raccourci", L"Tastenkombination aktivieren", L"Abilita tasto rapido", L"Habilitar atajo" }, // SettingsHotkeyEnable
    { L"Toggle hotkey:", L"Горячая клавиша:", L"Raccourci de basculement:", L"Umschalt-Tastenkombination:", L"Tasto rapido di commutazione:", L"Atajo de alternancia:" }, // SettingsHotkeyLabel
    { L"None", L"Нет", L"Aucun", L"Keine", L"Nessuno", L"Ninguno" }, // SettingsHotkeyNone
    { L"Indefinitely", L"Бесконечно", L"Indéfiniment", L"Unbegrenzt", L"Indefinitamente", L"Indefinidamente" }, // SettingsTimerIndefinite
    { L"For duration:", L"На время:", L"Pour durée:", L"Für Dauer:", L"Per durata:", L"Por duración:" }, // SettingsTimerDuration
    { L"Until time:", L"До времени:", L"Jusqu'à:", L"Bis:", L"Fino a:", L"Hasta:" }, // SettingsTimerUntilTime
    { L"minutes (5-1440)", L"минут (5-1440)", L"minutes (5-1440)", L"Minuten (5-1440)", L"minuti (5-1440)", L"minutos (5-1440)" }, // SettingsTimerMinutes
    { L"Until", L"До", L"Jusqu'à", L"Bis", L"Fino a", L"Hasta" }, // SettingsTimerUntil

    // Buttons
    { L"OK", L"ОК", L"OK", L"OK", L"OK", L"Aceptar" }, // ButtonOK
    { L"Cancel", L"Отмена", L"Annuler", L"Abbrechen", L"Annulla", L"Cancelar" }, // ButtonCancel
    { L"Apply", L"Применить", L"Appliquer", L"Übernehmen", L"Applica", L"Aplicar" }, // ButtonApply
    { L"Test", L"Тест", L"Test", L"Test", L"Test", L"Probar" }, // ButtonTest

    // About dialog
    { L"About Everon", L"О программе Everon", L"À propos d'Everon", L"Über Everon", L"Informazioni su Everon", L"Acerca de Everon" }, // AboutTitle
    { L"Everon v" VER_VERSION_STR_W, L"Everon v" VER_VERSION_STR_W, L"Everon v" VER_VERSION_STR_W, L"Everon v" VER_VERSION_STR_W, L"Everon v" VER_VERSION_STR_W, L"Everon v" VER_VERSION_STR_W }, // AboutVersion
    { L"Keep your PC awake", L"Не дать компьютеру уснуть", L"Gardez votre PC éveillé", L"Halten Sie Ihren PC wach", L"Mantieni il PC sveglio", L"Mantén tu PC despierto" }, // AboutTagline
    { L"Perfect for:", L"Идеально для:", L"Parfait pour:", L"Perfekt für:", L"Perfetto per:", L"Perfecto para:" }, // AboutPerfectFor
    { L"Long downloads and uploads", L"Длительные загрузки и выгрузки", L"Téléchargements longs", L"Lange Downloads und Uploads", L"Download e upload lunghi", L"Descargas y subidas largas" }, // AboutDownloads
    { L"Presentations and meetings", L"Презентации и встречи", L"Présentations et réunions", L"Präsentationen und Meetings", L"Presentazioni e riunioni", L"Presentaciones y reuniones" }, // AboutPresentations
    { L"Monitoring and automation", L"Мониторинг и автоматизация", L"Surveillance et automatisation", L"Überwachung und Automatisierung", L"Monitoraggio e automazione", L"Monitoreo y automatización" }, // AboutMonitoring
    { L"Media playback", L"Воспроизведение медиа", L"Lecture multimédia", L"Medienwiedergabe", L"Riproduzione media", L"Reproducción de medios" }, // AboutMediaPlayback
    { L"Right-click tray icon for settings", L"ПКМ на иконке в трее для настроек", L"Clic droit sur l'icône pour les paramètres", L"Rechtsklick auf Tray-Symbol für Einstellungen", L"Clic destro sull'icona per le impostazioni", L"Clic derecho en el icono para configuración" }, // AboutInstructions
    { L"MIT License - Made with C++", L"Лицензия MIT - Создано на C++", L"Licence MIT - Fait avec C++", L"MIT-Lizenz - Mit C++ erstellt", L"Licenza MIT - Realizzato con C++", L"Licencia MIT - Hecho con C++" }, // AboutLicense

    // Validation messages
    { L"Period must be between 1 and 86400 seconds.\n\n1 second = minimum\n86400 seconds = 24 hours (maximum)",
      L"Период должен быть от 1 до 86400 секунд.\n\n1 секунда = минимум\n86400 секунд = 24 часа (максимум)",
      L"La période doit être entre 1 et 86400 secondes.\n\n1 seconde = minimum\n86400 secondes = 24 heures (maximum)",
      L"Periode muss zwischen 1 und 86400 Sekunden liegen.\n\n1 Sekunde = Minimum\n86400 Sekunden = 24 Stunden (Maximum)",
      L"Il periodo deve essere tra 1 e 86400 secondi.\n\n1 secondo = minimo\n86400 secondi = 24 ore (massimo)",
      L"El período debe estar entre 1 y 86400 segundos.\n\n1 segundo = mínimo\n86400 segundos = 24 horas (máximo)" }, // ErrorInvalidPeriod
    { L"Invalid Period", L"Неверный период", L"Période invalide", L"Ungültige Periode", L"Periodo non valido", L"Período inválido" }, // ErrorInvalidPeriodTitle
    { L"Invalid Timer", L"Неверный таймер", L"Minuteur invalide", L"Ungültiger Timer", L"Timer non valido", L"Temporizador inválido" }, // ErrorInvalidTimerTitle
    { L"Please enter duration between 5 and 1440 minutes.",
      L"Введите длительность от 5 до 1440 минут.",
      L"Veuillez saisir une durée entre 5 et 1440 minutes.",
      L"Bitte geben Sie eine Dauer zwischen 5 und 1440 Minuten ein.",
      L"Inserisci una durata tra 5 e 1440 minuti.",
      L"Introduce una duración entre 5 y 1440 minutos." }, // ErrorInvalidTimerDuration
    { L"Please select a time in the future.",
      L"Выберите время в будущем.",
      L"Veuillez sélectionner une heure dans le futur.",
      L"Bitte wählen Sie eine zukünftige Uhrzeit.",
      L"Seleziona un orario nel futuro.",
      L"Selecciona una hora en el futuro." }, // ErrorInvalidTimerUntil
    { L"Failed to update the autostart setting.",
      L"Не удалось изменить настройку автозапуска.",
      L"Impossible de modifier le démarrage automatique.",
      L"Autostart-Einstellung konnte nicht geändert werden.",
      L"Impossibile modificare l'avvio automatico.",
      L"No se pudo cambiar el inicio automático." }, // ErrorAutoStart
    { L"Failed to save settings. Changes may be lost after restart.",
      L"Не удалось сохранить настройки. После перезапуска изменения могут быть потеряны.",
      L"Impossible d'enregistrer les paramètres. Les modifications peuvent être perdues après redémarrage.",
      L"Einstellungen konnten nicht gespeichert werden. Änderungen können nach einem Neustart verloren gehen.",
      L"Impossibile salvare le impostazioni. Le modifiche potrebbero andare perse dopo il riavvio.",
      L"No se pudieron guardar los ajustes. Los cambios pueden perderse después de reiniciar." }, // ErrorSaveSettings

    // Tooltips
    { L"Everon - Disabled", L"Everon - Отключено", L"Everon - Désactivé", L"Everon - Deaktiviert", L"Everon - Disattivato", L"Everon - Desactivado" }, // TooltipDisabled
    { L"Everon - Enabled",  L"Everon - Включено",  L"Everon - Activé",    L"Everon - Aktiviert",   L"Everon - Attivato",   L"Everon - Activado" }, // TooltipEnabled

    // Notifications
    { L"Everon enabled", L"Everon включен", L"Everon activé", L"Everon aktiviert", L"Everon attivato", L"Everon activado" }, // NotifyEnabled
    { L"Everon disabled", L"Everon отключен", L"Everon désactivé", L"Everon deaktiviert", L"Everon disattivato", L"Everon desactivado" }, // NotifyDisabled
    { L"Timer expired. Everon disabled.",
      L"Таймер истек. Everon отключен.",
      L"Minuteur expiré. Everon désactivé.",
      L"Timer abgelaufen. Everon deaktiviert.",
      L"Timer scaduto. Everon disattivato.",
      L"Temporizador agotado. Everon desactivado." }, // NotifyTimerExpired
    { L"Hotkey registered successfully", L"Горячая клавиша зарегистрирована", L"Raccourci enregistré avec succès", L"Tastenkombination erfolgreich registriert", L"Tasto rapido registrato con successo", L"Atajo registrado correctamente" }, // NotifyHotkeyRegistered
    { L"Failed to register hotkey. It may be in use by another application.",
      L"Не удалось зарегистрировать горячую клавишу. Возможно, она используется другим приложением.",
      L"Échec de l'enregistrement du raccourci. Il peut être utilisé par une autre application.",
      L"Tastenkombination konnte nicht registriert werden. Sie wird möglicherweise von einer anderen Anwendung verwendet.",
      L"Impossibile registrare il tasto rapido. Potrebbe essere in uso da un'altra applicazione.",
      L"No se pudo registrar el atajo. Puede estar en uso por otra aplicación." }, // NotifyHotkeyFailed

    // Errors
    { L"Failed to create tray icon.\nThe application may not function correctly.",
      L"Не удалось создать иконку в трее.\nПриложение может работать некорректно.",
      L"Impossible de créer l'icône de la barre d'état.\nL'application peut ne pas fonctionner correctement.",
      L"Tray-Symbol konnte nicht erstellt werden.\nDie Anwendung funktioniert möglicherweise nicht korrekt.",
      L"Impossibile creare l'icona nella barra.\nL'applicazione potrebbe non funzionare correttamente.",
      L"No se pudo crear el icono de la bandeja.\nLa aplicación puede no funcionar correctamente." }, // ErrorTrayIcon
    { L"Everon is already running.\nCheck the system tray for the icon.",
      L"Everon уже запущен.\nПроверьте системный трей.",
      L"Everon est déjà en cours d'exécution.\nVérifiez la barre d'état système.",
      L"Everon läuft bereits.\nÜberprüfen Sie die Taskleiste.",
      L"Everon è già in esecuzione.\nControlla la barra di sistema.",
      L"Everon ya está en ejecución.\nRevisa la bandeja del sistema." }, // ErrorAlreadyRunning
    { L"Everon", L"Everon", L"Everon", L"Everon", L"Everon", L"Everon" }, // ErrorTitle
};

} // namespace

Localization& Localization::Instance() {
    static Localization instance;
    return instance;
}

Localization::Localization() {
    m_currentLanguage = DetectSystemLanguage();
}

void Localization::SetLanguage(Language lang) {
    if (lang >= Language::Count) {
        m_currentLanguage = Language::English;
        return;
    }
    m_currentLanguage = lang;
}

const wchar_t* Localization::GetString(StringID id) const {
    const int sid = static_cast<int>(id);
    int lang = static_cast<int>(m_currentLanguage);

    if (sid < 0 || sid >= kStrCount) {
        return L"???";
    }
    if (lang < 0 || lang >= kLangCount) {
        lang = 0;
    }

    const wchar_t* s = kStrings[sid][lang];
    if (!s || !*s) {
        s = kStrings[sid][0]; // English fallback
    }
    return s ? s : L"???";
}

const wchar_t* Localization::GetLanguageName(Language lang) {
    const int l = static_cast<int>(lang);
    if (l < 0 || l >= kLangCount) {
        return kLanguageNames[0];
    }
    return kLanguageNames[l];
}

Language Localization::DetectSystemLanguage() {
    LANGID langId = GetUserDefaultUILanguage();
    WORD primaryLang = PRIMARYLANGID(langId);

    switch (primaryLang) {
        case LANG_RUSSIAN:  return Language::Russian;
        case LANG_FRENCH:   return Language::French;
        case LANG_GERMAN:   return Language::German;
        case LANG_ITALIAN:  return Language::Italian;
        case LANG_SPANISH:  return Language::Spanish;
        default:            return Language::English;
    }
}

const wchar_t* Localization::LanguageToString(Language lang) {
    const int l = static_cast<int>(lang);
    if (l < 0 || l >= kLangCount) {
        return L"en";
    }
    return kLanguageCodes[l];
}

Language Localization::StringToLanguage(const wchar_t* str) {
    if (!str) {
        return Language::English;
    }

    if (_wcsicmp(str, L"ru") == 0) return Language::Russian;
    if (_wcsicmp(str, L"fr") == 0) return Language::French;
    if (_wcsicmp(str, L"de") == 0) return Language::German;
    if (_wcsicmp(str, L"it") == 0) return Language::Italian;
    if (_wcsicmp(str, L"es") == 0) return Language::Spanish;
    return Language::English;
}

} // namespace Everon
