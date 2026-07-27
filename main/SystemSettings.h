#pragma once
#include <stdint.h>
/**
 * @file SystemSettings.h
 * @brief Defines system settings and related enumerations for the application.
 */

 /** @brief Enumerates the available measurement systems. */
enum class MeasurementSystem {
    METRIC,
    IMPERIAL
};

 /** @brief Enumerates the available modern theme colors. */
enum class ModernThemeColor {
    BLUE,
    CYAN,
    GREEN,
    YELLOW,
    RED,
    WHITE,
    MAX_COLORS
};

/**
 * @brief Defines the currently active screen layout.
 */
enum class DisplayPage {
    TOTAL_KM,
    AUTONOMY,
    TRIP_KM,
    TRIP_L_100KM,
    INSTANT_L_100KM,
    TRIP_AVG_KMH,
    TRIP_TIME,
    RPM_TEMP,
    DIGITAL_SPEED,
    MAX_PAGES
};

/**
 * @brief Defines the visual theme of the UI.
 */
enum class DisplayStyle {
    CLASSIC_AMBER,
    MODERN_DARK
};



/** @brief Structure to hold all system settings. */
struct SystemSettings {
    MeasurementSystem unit_system = MeasurementSystem::METRIC;
    ModernThemeColor modern_theme_color = ModernThemeColor::CYAN;
    bool dynamic_rpm_color = true;
    DisplayPage display_page = DisplayPage::TOTAL_KM;
    DisplayStyle display_style = DisplayStyle::CLASSIC_AMBER;
};