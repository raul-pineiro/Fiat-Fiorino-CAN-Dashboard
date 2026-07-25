#pragma once

#include <stdint.h>
#include <LovyanGFX.hpp>
#include "FiatCanParser.h"

/**
 * @brief Defines the visual theme of the UI.
 */
enum class DisplayStyle {
    CLASSIC_AMBER,
    MODERN_DARK
};

/**
 * @brief Defines the currently active screen layout.
 */
enum class DisplayPage {
    MAIN_DASH,
    TRIP_INFO
};

/**
 * @brief Manages TFT display rendering, UI layouts, and hardware initialization using LovyanGFX.
 */
class ScreenHandler {
public:
    ScreenHandler();
    ~ScreenHandler();

    /**
     * @brief Initializes the display hardware, backlights, and sprite buffers.
     */
    void begin();
    
    /**
     * @brief Explicitly sets the UI color theme.
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
     * @param page The desired DisplayPage.
     */
    void setPage(DisplayPage page);

    /**
     * @brief Updates the total odometer reading.
     * @param km Total distance in kilometers.
     */
    void updateKM(uint32_t km);

    /**
     * @brief Updates engine telemetry data for rendering.
     * @param engine Reference to the decoded EngineData structure.
     */
    void updateEngine(const FiatCAN::EngineData& engine);

    /**
     * @brief Updates the active trip computer mode requested by the CAN bus.
     * @param mode The decoded TripMode.
     */
    void updateTripMode(FiatCAN::TripMode mode);

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

private:
    bool _is_shutting_down = false;
    bool _save_success = false;
    
    // Custom RGB565 color definition for the retro amber theme
    static constexpr uint16_t AMBER_RETRO = 0xFB20; 
    
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

    DisplayStyle _current_style;
    DisplayPage _current_page;

    uint32_t _current_km;
    uint32_t _rpm;
    float _consumption;
    int _temp;
    uint8_t _fuel;
    FiatCAN::TripMode _trip_mode;

    void drawClassicMain();
    void drawClassicTrip();
    void drawModernMain();
    void drawModernTrip();
    void drawOverlays();
};