#include "Translator.h"

static const char* const DICTIONARY[static_cast<int>(TextKey::MAX_KEYS)][static_cast<int>(SystemLanguage::MAX_LANGUAGES)] = {
    // ENGLISH                 // SPANISH
    {"TOTAL MI",               "TOTAL MI"},                // TOTAL_MI
    {"TOTAL KM",               "TOTAL KM"},                // TOTAL_KM
    {"AUTONOMY",               "AUTONOMÍA"},               // AUTONOMY
    {"DISTANCE",               "DISTANCIA"},               // DISTANCE
    {"AVG CONS.",              "CONS. MED."},              // AVG_CONS
    {"INST CONS.",             "CONS. INST."},              // INST_CONS
    {"AVG SPEED",              "VEL. MEDIA"},              // AVG_SPEED
    {"TIME",                   "TIEMPO"},                    // TIME
    {"ENGINE RPM",             "RPM MOTOR"},               // ENGINE_RPM
    {"SPEED",                  "VELOCIDAD"},               // SPEED

    {"SYSTEM",                 "SISTEMA"},                 // MENU_SYSTEM_SETTINGS_TITLE
    {"RPM COLOR",              "COLOR RPM"},               // MENU_RPM_TITLE
    {"CLOCK",                  "RELOJ"},                   // MENU_CLOCK_TITLE
    {"RESET TRIP",             "RESETEAR TRIP"},           // MENU_RESET_TITLE
    {"System",                 "Sistema"},                 // MENU_SYSTEM
    {"Language",               "Idioma"},                  // MENU_LANGUAGE
    {"Dynamic",                "Dinámico"},                // MENU_DYNAMIC
    {"Theme",                  "Tema"},                    // MENU_THEME
    {"Cancel",                 "Cancelar"},                // MENU_CANCEL
    {"Clear Data",             "Borrar Datos"},            // MENU_CLEAR_DATA
    {"YES",                    "SI"},                      // MENU_YES
    {"NO",                     "NO"},                      // MENU_NO

    {"Memory Saved",           "Memoria Guardada"},        // MEM_SAVED
    {"Shutting down...",       "Apagando..."},             // SHUTTING_DOWN
    {"MEMORY ERROR",           "ERROR MEMORIA"},           // MEM_ERROR
    {"Data lost",              "Datos perdidos"},          // DATA_LOST
    {"EEPROM CORRUPT / RESET", "EEPROM CORRUPTA / RESET"}, // EEPROM_CORRUPT

    {"Press TRIP to config",   "Pulsa TRIP para conf."},   // MENU_PRESS_TRIP
    {"+ / - to change page",   "+ / - cambiar página"},    // MENU_CHANGE_PAGE
    {"Are you sure?",          "¿Estás seguro?"},          // MENU_ARE_YOU_SURE
    {"METRIC",                 "MÉTRICO"},                 // MENU_METRIC
    {"IMPERIAL",               "IMPERIAL"},                // MENU_IMPERIAL
    {"ON",                     "ON"},                      // MENU_ON
    {"OFF",                    "OFF"},                     // MENU_OFF
    {"BLUE",                   "AZUL"},                    // COLOR_BLUE
    {"CYAN",                   "CIAN"},                    // COLOR_CYAN
    {"GREEN",                  "VERDE"},                   // COLOR_GREEN
    {"YELLOW",                 "AMARILLO"},                // COLOR_YELLOW
    {"RED",                    "ROJO"},                    // COLOR_RED
    {"WHITE",                  "BLANCO"}                   // COLOR_WHITE
};

const char* getText(TextKey key, SystemLanguage lang) {
    return DICTIONARY[static_cast<int>(key)][static_cast<int>(lang)];
}

const char* getLanguageName(SystemLanguage lang) {
    /* Hardcoding endonyms ensures safe navigation recovery. 
       If the system boots in an unfamiliar localized state (e.g. Cyrillic), 
       the user relies on visual pattern recognition of their native spelling 
       to switch it back. Translating these names defeats this safety mechanism.*/
    static const char* const native_names[] = {
        "English",
        "Español"
    };

    // Boundary check prevents memory leaks or out-of-bounds array reads 
    // if the EEPROM reads a corrupted/invalid SystemLanguage enum value.
    if (static_cast<int>(lang) >= static_cast<int>(SystemLanguage::MAX_LANGUAGES)) {
        return "Unknown";
    }

    return native_names[static_cast<int>(lang)];
}