#include "ButtonHandler.h"

ButtonHandler::ButtonHandler(ScreenHandler& screen) 
    : _screen(screen), 
      _adc_handle(nullptr), 
      _trip_prev(false), 
      _minus_prev(false), 
      _plus_prev(false), 
      _menu_prev(false), 
      _trip_consumed_by_combo(false),
      _menu_pending(false),
      _menu_lockout(false) {}

ButtonHandler::~ButtonHandler() {
    if (_adc_handle != nullptr) {
        adc_oneshot_del_unit(_adc_handle);
    }
}

void ButtonHandler::begin() {
    // Disable internal pull resistors as physical resistors are already present on the PCB layout.
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_TRIP) | (1ULL << PIN_MINUS);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &_adc_handle);

    // 12dB attenuation extends full-scale dynamic range up to ~3.3V to match board voltage rail.
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(_adc_handle, ADC_CHANNEL_MENU_PLUS, &config);
}

void ButtonHandler::update() {
    bool current_trip = (gpio_get_level(PIN_TRIP) == 1);
    bool current_minus = (gpio_get_level(PIN_MINUS) == 1);

    int adc_val = 4095;
    if (_adc_handle != nullptr) {
        adc_oneshot_read(_adc_handle, ADC_CHANNEL_MENU_PLUS, &adc_val);
    }

    // Demultiplex discrete buttons sharing a single ADC input via resistor divider levels.
    bool current_plus = (adc_val <= ADC_PLUS_MAX);
    bool current_menu = (adc_val >= ADC_MENU_MIN && adc_val <= ADC_MENU_MAX);
    bool current_idle = (adc_val > ADC_MENU_MAX);

    // TRIP standalone action evaluates on release, allowing it to act as a modifier key for press combos.
    if (current_trip && !_trip_prev) {
        _trip_consumed_by_combo = false;
    } 
    else if (!current_trip && _trip_prev) {
        if (!_trip_consumed_by_combo) {
            _screen.handleButtonTrip();
        }
    }

    if (current_plus) {
        if (!_plus_prev) {
            if (current_trip) {
                _screen.handleComboTripPlus();
                _trip_consumed_by_combo = true; // Suppress standalone TRIP action when button is released.
            } else {
                _screen.handleButtonPlus();
            }
        }
        
        // Target voltage reached the lowest threshold, meaning PLUS was intended.
        // Lockout prevents a false MENU trigger as the voltage rises back through the intermediate threshold on release.
        _menu_pending = false;
        _menu_lockout = true;
    } 
    else if (current_menu) {
        if (!_menu_lockout) {
            _menu_pending = true;
        }
    } 
    else if (current_idle) {
        if (_menu_pending) {
            _screen.handleButtonMenu();
            _menu_pending = false;
        }
        _menu_lockout = false;
    }

    if (current_minus && !_minus_prev) {
        _screen.handleButtonMinus();
    }

    _trip_prev = current_trip;
    _plus_prev = current_plus;
    _menu_prev = current_menu;
    _minus_prev = current_minus;
}