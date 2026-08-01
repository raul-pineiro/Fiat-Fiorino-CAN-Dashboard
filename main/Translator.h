#pragma once

/**
 * @brief Available system languages for the UI.
 */
enum class SystemLanguage {
    ENGLISH,
    SPANISH,
    MAX_LANGUAGES
};

/**
 * @brief Keys representing translatable text strings in the application.
 */
enum class TextKey {
    TOTAL_MI,
    TOTAL_KM,
    AUTONOMY,
    DISTANCE,
    AVG_CONS,
    INST_CONS,
    AVG_SPEED,
    TIME,
    ENGINE_RPM,
    SPEED,

    MENU_SYSTEM_SETTINGS_TITLE,
    MENU_RPM_TITLE,
    MENU_CLOCK_TITLE,
    MENU_RESET_TITLE,
    MENU_SYSTEM,
    MENU_LANGUAGE,
    MENU_DYNAMIC,
    MENU_THEME,
    MENU_CANCEL,
    MENU_CLEAR_DATA,
    MENU_YES,
    MENU_NO,
    
    MEM_SAVED,
    SHUTTING_DOWN,
    MEM_ERROR,
    DATA_LOST,
    EEPROM_CORRUPT,

    MENU_PRESS_TRIP,
    MENU_CHANGE_PAGE,
    MENU_ARE_YOU_SURE,
    MENU_METRIC,
    MENU_IMPERIAL,
    MENU_ON,
    MENU_OFF,
    COLOR_BLUE,
    COLOR_CYAN,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_RED,
    COLOR_WHITE,

    MAX_KEYS
};

/**
 * @brief Retrieves the translated text for a specific key in the requested language.
 * 
 * @param key The identifier of the text string to retrieve.
 * @param lang The target language for the translation.
 * @return const char* Pointer to the null-terminated translated string.
 */
const char* getText(TextKey key, SystemLanguage lang);

/**
 * @brief Retrieves the localized display name of a given language (e.g., "English", "Español").
 * 
 * @param lang The language whose name is to be retrieved.
 * @return const char* Pointer to the null-terminated language name string.
 */
const char* getLanguageName(SystemLanguage lang);