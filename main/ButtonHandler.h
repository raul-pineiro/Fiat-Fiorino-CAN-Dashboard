#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "ScreenHandler.h"

/**
 * @class ButtonHandler
 * @brief Handles physical button inputs, ADC voltage-multiplexed buttons, and button combo logic.
 */
class ButtonHandler {
public:
    /**
     * @brief Construct a new Button Handler object.
     * @param screen Reference to the ScreenHandler instance for UI interactions.
     */
    ButtonHandler(ScreenHandler& screen);

    /**
     * @brief Destroy the Button Handler object and release ADC resources.
     */
    ~ButtonHandler();
    
    /**
     * @brief Initializes GPIO pins and configures the ESP-IDF ADC Oneshot driver.
     */
    void begin();
    
    /**
     * @brief Polls button states, updates edge detection, and executes corresponding actions.
     * @note Should be called periodically (~40ms) from task_gui_core1.
     */
    void update();

private:
    ScreenHandler& _screen; /**< Reference to the screen handler instance. */

    /**
     * @brief Native GPIO pin assignments.
     */
    static constexpr gpio_num_t PIN_TRIP = GPIO_NUM_33;
    static constexpr gpio_num_t PIN_MINUS = GPIO_NUM_32;
    
    static constexpr adc_channel_t ADC_CHANNEL_MENU_PLUS = ADC_CHANNEL_7; /**< ADC1 Channel 7 (GPIO 35). */

    adc_oneshot_unit_handle_t _adc_handle; /**< Handle for the ESP-IDF v5.x ADC Oneshot driver. */

    /**
     * @brief ADC raw reading thresholds (range 0 - 4095).
     */
    static constexpr int ADC_PLUS_MAX = 500;   /**< Upper raw threshold for PLUS button (~0V). */
    static constexpr int ADC_MENU_MIN = 1200;  /**< Lower raw threshold for MENU button (~1.5V). */
    static constexpr int ADC_MENU_MAX = 2400;  /**< Upper raw threshold for MENU button (~1.5V). */

    /**
     * @brief Previous button states for edge detection.
     */
    bool _trip_prev;
    bool _minus_prev;
    bool _plus_prev;
    bool _menu_prev;

    bool _trip_consumed_by_combo; /**< Flag indicating the TRIP release event was suppressed by a combination press. */
};