#pragma once

#include <stdint.h>
#include <LovyanGFX.hpp>
#include "FiatCanParser.h"
#include <functional>   
#include "SystemSettings.h"

/**
 * @brief Defines the type of icon to be displayed on the screen.
 */
enum class IconType {
    NONE,
    FUEL,
    TEMP,
    WARNING
};

/**
 * @brief Defines the current mode of the user interface.
 */
enum class UIMode {
    DASHBOARD,
    SETTINGS
};

/**
 * @brief Enumerates the available pages within the settings menu.
 */
enum class SettingsPage {
    REGIONAL_SETUP,
    DYNAMIC_RPM_COLOR,
    CLOCK_CONFIGURATION,
    RESET_TRIP,
    MAX_PAGES
};

/**
 * @brief Enumerates the navigation levels within the settings menu.
 */
enum class MenuLevel {
    PAGE_SELECT,
    SUB_SELECT,
    EDIT_VALUE,
    MAX_LEVELS
};

/**
 * @brief Enumerates the steps for editing the system clock.
 */
enum class ClockEditStep {
    NONE,
    HOURS,
    MINUTES,
    MAX_STEPS
};

/**
 * @brief Manages TFT display rendering, UI layouts, and hardware initialization using LovyanGFX.
 */
class ScreenHandler {
public:
    /**
     * @brief Constructs a new ScreenHandler object.
     */
    ScreenHandler();

    /**
     * @brief Destroys the ScreenHandler object.
     */
    ~ScreenHandler();

    /**
     * @brief Initializes the display hardware, backlights, and sprite buffers.
     */
    void begin();

    /**
     * @brief Turns off the backlight and suspends the display hardware to minimize power consumption.
     */
    void sleep();
    
    /**
     * @brief Explicitly sets the UI color theme.
     * 
     * @param style The desired DisplayStyle.
     */
    void setStyle(DisplayStyle style);

    /**
     * @brief Toggles between the available UI themes.
     */
    void toggleStyle();

    /**
     * @brief Cycles to the next available display layout.
     */
    void nextPage();

    /**
     * @brief Explicitly sets the active display layout.
     * 
     * @param page The desired DisplayPage.
     */
    void setPage(DisplayPage page);

    /**
     * @brief Updates the total odometer reading.
     * 
     * @param km Total distance in kilometers.
     */
    void updateKM(uint32_t km);

    /**
     * @brief Updates engine telemetry data for rendering.
     * 
     * @param engine Reference to the decoded EngineData structure.
     */
    void updateEngine(const FiatCAN::EngineData& engine);

    /**
     * @brief Updates the vehicle speed for rendering.
     * 
     * @param speed The current speed in km/h.
     */
    void updateSpeed(uint16_t speed);

    /**
     * @brief Updates the current fuel level in the tank.
     * 
     * @param fuel_liters Remaining fuel in liters.
     */
    void updateFuel(uint8_t fuel_liters);

    /**
     * @brief Updates the estimated autonomy.
     * 
     * @param autonomy_km Estimated distance in kilometers before refueling is needed.
     */
    void updateAutonomy(uint16_t autonomy_km);

    /**
     * @brief Updates the trip data for rendering.
     * 
     * @param trip_km Total distance for the current trip.
     * @param trip_l_100km Instantaneous fuel consumption per 100 km.
     * @param trip_avg_kmh Average speed during the current trip.
     * @param trip_time Duration of the current trip in seconds.
     */
    void updateTripData(float trip_km, float trip_l_100km, uint16_t trip_avg_kmh, uint32_t trip_time);

    /**
     * @brief Renders the current frame and pushes the sprite to the TFT display.
     */
    void render(); 

    /**
     * @brief Triggers the emergency shutdown overlay.
     * 
     * @param state True if the shutdown sequence is active.
     * @param success True if the EEPROM save was successful prior to shutdown.
     */
    void setShutdownState(bool state, bool success);

    /**
     * @brief Handles the menu button press event.
     */
    void handleButtonMenu();

    /**
     * @brief Handles the trip button press event.
     */
    void handleButtonTrip();

    /**
     * @brief Handles the plus button press event.
     */
    void handleButtonPlus();

    /**
     * @brief Handles the minus button press event.
     */
    void handleButtonMinus();

    /**
     * @brief Handles the combination of trip and plus buttons being pressed simultaneously.
     */
    void handleComboTripPlus();

    /**
     * @brief Retrieves the current system settings.
     * 
     * @return const SystemSettings& Reference to the current settings.
     */
    const SystemSettings& getSettings() const { return _settings; }

    /**
     * @brief Retrieves the current UI mode.
     * 
     * @return UIMode The active UI mode.
     */
    UIMode getUiMode() const { return _ui_mode; }

    /**
     * @brief Registers a callback to be invoked when the trip data is reset.
     * 
     * @param callback Function to execute on trip reset.
     */
    void setOnTripResetCallback(std::function<void()> callback) {
        _on_trip_reset_cb = callback;
    }

    /**
     * @brief Registers a callback to be invoked when system time is edited via the user interface.
     * 
     * @param callback Function accepting new hours and minutes values.
     */
    void setOnClockSetCallback(std::function<void(uint8_t, uint8_t)> callback) {
        _on_clock_set_cb = callback;
    }

    /**
     * @brief Applies stored system settings to the screen handler.
     * 
     * @param saved_settings The settings to apply.
     */
    void applySettings(const SystemSettings& saved_settings) {
        _settings = saved_settings;
    }
    
    /**
     * @brief Triggers the display of an EEPROM corruption warning overlay.
     */
    void showEepromWarning();

private:
    bool _is_shutting_down = false;
    bool _save_success = false;
    bool _show_eeprom_warning = false;
    uint32_t _warning_start_time = 0;

    std::function<void()> _on_trip_reset_cb = nullptr;
    std::function<void(uint8_t, uint8_t)> _on_clock_set_cb = nullptr;
    
    /**
     * @brief Custom RGB565 color definition for the retro amber theme.
     */
    static constexpr uint16_t AMBER_RETRO = 0xFA80; 
    
    /**
     * @brief Hardware configuration struct for LovyanGFX (ST7789 via SPI).
     */
    struct LGFX_Config : public lgfx::LGFX_Device {
        lgfx::Panel_ST7789 _panel_instance;
        lgfx::Bus_SPI      _bus_instance;
        lgfx::Light_PWM    _light_instance;
        LGFX_Config();
    };

    LGFX_Config lcd;
    lgfx::LGFX_Sprite canvas;

    uint32_t _current_km;
    uint32_t _rpm;
    float _consumption;
    int _temp;
    uint8_t _fuel;
    uint16_t _speed;
    float _trip_km;
    float _trip_l_100km;
    uint16_t _trip_avg_kmh;
    uint32_t _trip_time;
    uint16_t _autonomy_km;

    SystemSettings _settings;
    UIMode _ui_mode;
    MenuLevel _menu_level = MenuLevel::PAGE_SELECT;
    SettingsPage _current_settings_page = SettingsPage::REGIONAL_SETUP;

    int _current_sub_option = 0;
    int _edit_hour = 0;
    int _edit_minute = 0;
    ClockEditStep _clock_edit_step = ClockEditStep::NONE;

    /**
     * @brief Draws the currently selected dashboard page.
     */
    void drawPage();

    /**
     * @brief Renders the classic visual template.
     * 
     * @param top_text Text to display at the top.
     * @param center_text Main text in the center.
     * @param unit_text Text to display as the unit.
     * @param show_trip Whether to display trip information.
     * @param icon The type of icon to display.
     * @param bottom_left_text Text to display in the bottom left.
     */
    void renderClassicTemplate(const char* top_text, const char* center_text, const char* unit_text, bool show_trip, IconType icon, const char* bottom_left_text);

    /**
     * @brief Renders the modern visual template.
     * 
     * @param top_text Text to display at the top.
     * @param center_text Main text in the center.
     * @param unit_text Text to display as the unit.
     * @param show_trip Whether to display trip information.
     * @param icon The type of icon to display.
     * @param bottom_left_text Text to display in the bottom left.
     */
    void renderModernTemplate(const char* top_text, const char* center_text, const char* unit_text, bool show_trip, IconType icon, const char* bottom_left_text);
    
    /**
     * @brief Draws a placeholder icon based on the specified type.
     * 
     * @param type The type of the icon.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color The color of the icon.
     */
    void drawPlaceholderIcon(IconType type, int x, int y, uint16_t color);

    /**
     * @brief Draws system overlays on top of the main UI.
     */
    void drawOverlays();

    /**
     * @brief Renders the settings menu interface.
     */
    void renderMenu();
    
    /**
     * @brief Renders the menu using the classic theme.
     */
    void renderClassicMenu();

    /**
     * @brief Renders the menu using the modern theme.
     */
    void renderModernMenu();

    /**
     * @brief Converts and formats speed based on current unit settings.
     * 
     * @param speed_kmh The raw speed in km/h.
     * @return float The formatted speed.
     */
    float getFormattedSpeed(uint16_t speed_kmh);

    /**
     * @brief Retrieves the correct string for the currently selected speed unit.
     * 
     * @return const char* Pointer to the unit string.
     */
    const char* getSpeedUnitText();

    /**
     * @brief Draws the menu UI elements with specified colors.
     * 
     * @param bg_color Background color.
     * @param text_color Text color.
     * @param highlight_color Highlight color for selected items.
     */
    void drawMenuUI(uint16_t bg_color, uint16_t text_color, uint16_t highlight_color);

    /**
     * @brief Retrieves the exact RGB565 value for a modern theme color.
     * 
     * @param color The enum value of the modern color.
     * @return uint16_t The calculated RGB565 color.
     */
    uint16_t getThemeColorValue(ModernThemeColor color);
};