#include "ScreenHandler.h"
#include <stdio.h>
#include <time.h>
#include "esp_timer.h"
#include "Translator.h"
#include "MyFonts.h"

// Viewport dimensions and offsets for the display
#define VIEWPORT_WIDTH  290  
#define VIEWPORT_HEIGHT 170  
#define VIEWPORT_OFFSET_X 32
#define VIEWPORT_OFFSET_Y 40

// RPM limit thresholds for color change
#define RPM_LIMIT_1 3000
#define RPM_LIMIT_2 3500
#define RPM_LIMIT_3 5000

ScreenHandler::LGFX_Config::LGFX_Config() {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI3_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 23;
        cfg.pin_miso = -1;
        cfg.pin_dc   = 19;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs   = 5;
        cfg.pin_rst  = 4;
        cfg.panel_width  = 240;
        cfg.panel_height = 320;
        cfg.offset_x     = 0;
        cfg.offset_y     = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.invert       = false;   
        cfg.rgb_order    = false;    
        _panel_instance.config(cfg);
    }
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = 16;
        cfg.invert = false;
        cfg.freq   = 44100;
        cfg.pwm_channel = 7;
        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
}

ScreenHandler::ScreenHandler() 
    : canvas(&lcd),
      _current_km(0), 
      _rpm(0), 
      _consumption(0.0f), 
      _temp(0), 
      _fuel(0), 
      _speed(0), _trip_km(0), _trip_l_100km(0.0f), _trip_avg_kmh(0), _trip_time(0),
      _autonomy_km(0) {}

ScreenHandler::~ScreenHandler() {
    canvas.deleteSprite();
}

void ScreenHandler::begin() {
    lcd.wakeup();
    lcd.init();
    lcd.setBrightness(255);
    lcd.setRotation(1);
    lcd.fillScreen(TFT_BLACK);
    
    canvas.setColorDepth(16);
    if (canvas.createSprite(VIEWPORT_WIDTH, VIEWPORT_HEIGHT) == nullptr) {
        printf("ERROR: Insufficient memory for the Sprite!\n");
    }
}

void ScreenHandler::sleep() {
    lcd.setBrightness(0);
    lcd.sleep();
}

void ScreenHandler::updateKM(uint32_t km) { 
    _current_km = km; 
}

void ScreenHandler::updateEngine(const FiatCAN::EngineData& engine) {
    _rpm = engine.rpm;
    _consumption = engine.consumption_lh;
    _temp = engine.temp;
}

void ScreenHandler::updateSpeed(uint16_t speed) { _speed = speed; }

void ScreenHandler::updateFuel(uint8_t fuel_liters) {
    _fuel = fuel_liters;
}

void ScreenHandler::updateAutonomy(uint16_t autonomy_km) {
    _autonomy_km = autonomy_km;
}

void ScreenHandler::updateTripData(float trip_km, float trip_l_100km, uint16_t trip_avg_kmh, uint32_t trip_time) {
    _trip_km = trip_km;
    _trip_l_100km = trip_l_100km;
    _trip_avg_kmh = trip_avg_kmh;
    _trip_time = trip_time;
}

void ScreenHandler::setStyle(DisplayStyle style) {
    _settings.display_style = style;
}

void ScreenHandler::toggleStyle() {
    _settings.display_style = (_settings.display_style == DisplayStyle::CLASSIC_AMBER) 
                     ? DisplayStyle::MODERN_DARK 
                     : DisplayStyle::CLASSIC_AMBER;
}

void ScreenHandler::nextPage() {
    int next = (static_cast<int>(_settings.display_page) + 1) % static_cast<int>(DisplayPage::MAX_PAGES);
    _settings.display_page = static_cast<DisplayPage>(next);
}

void ScreenHandler::setPage(DisplayPage page) {
    _settings.display_page = page;
}

void ScreenHandler::render() {
    canvas.setTextSize(1);
    if (_ui_mode == UIMode::DASHBOARD) {
        drawPage();
    } else {
        renderMenu();
    }
    drawOverlays();
    uint16_t current_bg_color = (_settings.display_style == DisplayStyle::CLASSIC_AMBER) 
                                ? AMBER_RETRO 
                                : TFT_BLACK;
    static uint16_t last_bg_color = 0xFFFF;
    if (current_bg_color != last_bg_color) {
        lcd.startWrite();
        lcd.fillRect(VIEWPORT_OFFSET_X - 20, VIEWPORT_OFFSET_Y - 20, VIEWPORT_WIDTH + 40, 20, current_bg_color);
        lcd.fillRect(VIEWPORT_OFFSET_X - 20, VIEWPORT_OFFSET_Y + VIEWPORT_HEIGHT, VIEWPORT_WIDTH + 40, 20, current_bg_color);
        lcd.fillRect(VIEWPORT_OFFSET_X - 20, VIEWPORT_OFFSET_Y, 20, VIEWPORT_HEIGHT, current_bg_color);
        lcd.fillRect(VIEWPORT_OFFSET_X + VIEWPORT_WIDTH, VIEWPORT_OFFSET_Y, 50, VIEWPORT_HEIGHT, current_bg_color);
        lcd.endWrite();

        last_bg_color = current_bg_color;
    }
    canvas.pushSprite(VIEWPORT_OFFSET_X, VIEWPORT_OFFSET_Y);
}

void ScreenHandler::renderClassicTemplate(const char* sub_text, const char* main_value, const char* unit_text, bool show_trip, IconType icon, const char* bottom_left_text) {
    canvas.fillSprite(AMBER_RETRO);
    int width = canvas.width();
    int height = canvas.height();
    
    canvas.setFont(&::FreeSansBold18pt8b);

    canvas.setTextColor(TFT_BLACK, AMBER_RETRO);

    int value_y = height / 3 - 15;

    int value_width = canvas.textWidth(main_value);

    canvas.setFont(&::FreeSans9pt8b);    
    int unit_width = (unit_text && unit_text[0] != '\0') ? canvas.textWidth(unit_text) : 0;

    canvas.setFont(&::FreeSansBold20pt8b);
    canvas.setTextDatum(middle_center);
    canvas.drawString(main_value, width / 2, value_y);

    if (unit_width > 0) {
        int unit_x = (width / 2) + (value_width / 2) + 4;
        canvas.setFont(&::FreeSans9pt8b);
        canvas.setTextDatum(bottom_left);
        canvas.drawString(unit_text, unit_x, value_y + 14);
    }

    if (icon != IconType::NONE) {
        drawPlaceholderIcon(icon, width - 25, value_y, TFT_BLACK);
    }

    int text_y = height / 2 + 10;
    canvas.setFont(&::FreeSans16pt8b);
    canvas.setTextDatum(middle_center);
    canvas.drawString(sub_text, width / 2, text_y);
    
    text_y = height / 2 - 10;
    canvas.setFont(&::FreeSans9pt8b);
    if (show_trip) {
        canvas.setTextDatum(middle_left);
        canvas.drawString("TRIP", 15, text_y);
    }

    int line_y = height - 55;
    canvas.drawFastHLine(15, line_y, width - 30, TFT_BLACK);
    
    int bottom_y = line_y + (height - line_y) / 2;
    canvas.setTextDatum(middle_left);
    canvas.drawString(bottom_left_text, 15, bottom_y);
}

void ScreenHandler::drawPage() {
    char value_buf[32] = {0};
    char unit_buf[16] = {0};
    char bottom_buf[32] = {0};

    const char* sub_text = "";
    bool show_trip = false;
    IconType icon = IconType::NONE;

    snprintf(bottom_buf, sizeof(bottom_buf), "RPM: %u", static_cast<unsigned>(_rpm));

    switch (_settings.display_page) {
        case DisplayPage::TOTAL_KM:
            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                sub_text = getText(TextKey::TOTAL_MI, _settings.language);
                snprintf(value_buf, sizeof(value_buf), "%u", static_cast<unsigned>(_current_km * 0.621371f));
            } else {
                sub_text = getText(TextKey::TOTAL_KM, _settings.language);
                snprintf(value_buf, sizeof(value_buf), "%u", static_cast<unsigned>(_current_km));
            }
            break;

        case DisplayPage::AUTONOMY: {
            sub_text = getText(TextKey::AUTONOMY, _settings.language);
            icon = IconType::FUEL;
            
            if (_autonomy_km != 0xFFFF && _autonomy_km > 0) {
                if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                    snprintf(value_buf, sizeof(value_buf), "%u", (unsigned int)(_autonomy_km * 0.621371f));
                    snprintf(unit_buf, sizeof(unit_buf), "mi");
                } else {
                    snprintf(value_buf, sizeof(value_buf), "%u", (unsigned int)_autonomy_km);
                    snprintf(unit_buf, sizeof(unit_buf), "km");
                }
            } else {
                snprintf(value_buf, sizeof(value_buf), "---");
                snprintf(unit_buf, sizeof(unit_buf), _settings.unit_system == MeasurementSystem::IMPERIAL ? "mi" : "km");
            }
            float liters = (float)_fuel * 0.45f;
            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                float gallons = liters / 3.78541f;
                snprintf(bottom_buf, sizeof(bottom_buf), "%.1f gal (%u%%)", gallons, (unsigned int)_fuel);
            } else {
                snprintf(bottom_buf, sizeof(bottom_buf), "%.1fL (%u%%)", liters, (unsigned int)_fuel);
            }
            break;
        }

        case DisplayPage::TRIP_KM:
            sub_text = getText(TextKey::DISTANCE, _settings.language);
            show_trip = true;
            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                snprintf(value_buf, sizeof(value_buf), "%.1f", static_cast<float>(_trip_km * 0.621371f));
                snprintf(unit_buf, sizeof(unit_buf), "mi");
            } else {
                snprintf(value_buf, sizeof(value_buf), "%.1f", static_cast<float>(_trip_km));
                snprintf(unit_buf, sizeof(unit_buf), "km");
            }
            break;

        case DisplayPage::TRIP_L_100KM:
            sub_text = getText(TextKey::AVG_CONS, _settings.language);
            show_trip = true;

            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                float trip_mpg = (_trip_l_100km > 0.1f) ? (235.215f / _trip_l_100km) : 99.9f;
                if (trip_mpg > 99.9f) trip_mpg = 99.9f;

                snprintf(value_buf, sizeof(value_buf), "%.1f", trip_mpg);
                snprintf(unit_buf, sizeof(unit_buf), "mpg");
                snprintf(bottom_buf, sizeof(bottom_buf), "%.1f mi", static_cast<float>(_trip_km * 0.621371f));
            } else {
                snprintf(value_buf, sizeof(value_buf), "%.1f", _trip_l_100km);
                snprintf(unit_buf, sizeof(unit_buf), "L/100km");
                snprintf(bottom_buf, sizeof(bottom_buf), "%.1f km", static_cast<float>(_trip_km));
            }
            break;

        case DisplayPage::INSTANT_L_100KM:
            sub_text = getText(TextKey::INST_CONS, _settings.language);

            if (_speed < 3) {
                if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                    float gal_h = _consumption / 3.78541f;
                    snprintf(value_buf, sizeof(value_buf), "%.1f", gal_h);
                    snprintf(unit_buf, sizeof(unit_buf), "gal/h");
                } else {
                    snprintf(value_buf, sizeof(value_buf), "%.1f", _consumption);
                    snprintf(unit_buf, sizeof(unit_buf), "L/h");
                }
            } else {
                if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                    float mph = _speed * 0.621371f;
                    float gal_h = _consumption / 3.78541f;
                    
                    float mpg = (gal_h > 0.05f) ? (mph / gal_h) : 99.9f; 
                    if (mpg > 99.9f) mpg = 99.9f;

                    snprintf(value_buf, sizeof(value_buf), "%.1f", mpg);
                    snprintf(unit_buf, sizeof(unit_buf), "mpg");
                } else {
                    float l_100km = (_consumption / _speed) * 100.0f;
                    if (l_100km > 99.9f) l_100km = 99.9f; 

                    snprintf(value_buf, sizeof(value_buf), "%.1f", l_100km);
                    snprintf(unit_buf, sizeof(unit_buf), "L/100km");
                }
            }
            break;

        case DisplayPage::TRIP_AVG_KMH:
            sub_text = getText(TextKey::AVG_SPEED, _settings.language);
            show_trip = true; 
            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                snprintf(value_buf, sizeof(value_buf), "%u", static_cast<unsigned>(_trip_avg_kmh * 0.621371f));
                snprintf(unit_buf, sizeof(unit_buf), "mph");
            } else {
                snprintf(value_buf, sizeof(value_buf), "%u", static_cast<unsigned>(_trip_avg_kmh));
                snprintf(unit_buf, sizeof(unit_buf), "km/h");
            }
            break;

        case DisplayPage::TRIP_TIME:
            sub_text = getText(TextKey::TIME, _settings.language);
            show_trip = true;

            if (_trip_time >= 3600) {
                unsigned hours = static_cast<unsigned>(_trip_time / 3600);
                unsigned minutes = static_cast<unsigned>((_trip_time % 3600) / 60);
                snprintf(value_buf, sizeof(value_buf), "%02u:%02u", hours, minutes);
            } else {
                unsigned minutes = static_cast<unsigned>(_trip_time / 60);
                unsigned seconds = static_cast<unsigned>(_trip_time % 60);
                snprintf(value_buf, sizeof(value_buf), "%02u:%02u", minutes, seconds);
            }
            break;
            
        case DisplayPage::RPM_TEMP:
            sub_text = getText(TextKey::ENGINE_RPM, _settings.language);
            icon = IconType::TEMP;
            snprintf(value_buf, sizeof(value_buf), "%u", static_cast<unsigned>(_rpm));
            snprintf(unit_buf ,sizeof(unit_buf), "rpm");
            
            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                int temp_f = static_cast<int>((_temp * 9.0f / 5.0f) + 32.0f);
                snprintf(bottom_buf, sizeof(bottom_buf), "Temp: %d F", temp_f);
            } else {
                snprintf(bottom_buf, sizeof(bottom_buf), "Temp: %d C", _temp);
            }
            break;

        case DisplayPage::DIGITAL_SPEED:
            sub_text = getText(TextKey::SPEED, _settings.language);
            if (_settings.unit_system == MeasurementSystem::IMPERIAL) {
                snprintf(value_buf, sizeof(value_buf), "%u", (unsigned int)(_speed * 0.621371f));
                snprintf(unit_buf, sizeof(unit_buf), "mph");
            } else {
                snprintf(value_buf, sizeof(value_buf), "%u", (unsigned int)(_speed));
                snprintf(unit_buf, sizeof(unit_buf), "km/h");
            }
            break;

        default:
            return;
    }

    if (_settings.display_style == DisplayStyle::CLASSIC_AMBER) {
        renderClassicTemplate(sub_text, value_buf, unit_buf, show_trip, icon, bottom_buf);
    } else {
        renderModernTemplate(sub_text, value_buf, unit_buf, show_trip, icon, bottom_buf);
    }
}

void ScreenHandler::drawPlaceholderIcon(IconType icon, int x, int y, uint16_t color) {
    canvas.setFont(&::FreeSansBold9pt8b); 
    canvas.setTextDatum(middle_center);
    ;
    canvas.setTextColor(color);
    canvas.drawCircle(x, y, 14, color);
    
    switch (icon) {
        case IconType::FUEL:
            canvas.drawString("F", x, y);
            break;
        case IconType::TEMP:
            canvas.drawString("C", x, y);
            break;
        case IconType::WARNING:
            canvas.drawString("!", x, y);
            break;
        default:
            break;
    }
    
}

void ScreenHandler::renderModernTemplate(const char* sub_text, const char* main_value, const char* unit_text, bool show_trip, IconType icon, const char* bottom_left_text) {
    canvas.fillSprite(TFT_BLACK); 
    int width = canvas.width();
    int height = canvas.height();

    uint16_t theme_color = getThemeColorValue(_settings.modern_theme_color);

    // RPM sensitive color line
    uint16_t line_color;

    if (_rpm <= RPM_LIMIT_1) {
        line_color = theme_color;
    } else if (_rpm <= RPM_LIMIT_2) {
        float t = (float)(_rpm - RPM_LIMIT_1) / (RPM_LIMIT_2 - RPM_LIMIT_1);
        uint8_t b = 255 - (uint8_t)(255 * t);
        line_color = lgfx::color565(0, 255, b);
    } else if (_rpm <= RPM_LIMIT_3) {
        float t = (float)(_rpm - RPM_LIMIT_2) / (RPM_LIMIT_3 - RPM_LIMIT_2);
        uint8_t r;       
        uint8_t g; 
        if (t < 0.5f) {
            r = (uint8_t)(255 * (t * 2));
            g = 255;
        } else {
            r = 255;
            g = (uint8_t)(255 * (1 - ((t - 0.5f) * 2)));
        }
        line_color = lgfx::color565(r, g, 0);
    } else {
        uint32_t ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if ((ms / 150) % 2 == 0) { 
            line_color = TFT_RED;
        } else {
            line_color = lgfx::color565(40, 0, 0);
        }
    }

    canvas.drawFastHLine(20, height / 3 - 10, width - 40, line_color);

    canvas.setFont(&::FreeSansBold12pt8b);
    canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    canvas.setTextDatum(middle_center);
    
    canvas.drawString(sub_text, width / 2, height / 4 - 10);

    if (show_trip) {
        canvas.setTextColor(theme_color, TFT_BLACK);
        canvas.setTextDatum(middle_left);
        canvas.drawString("TRIP", 15, height / 4 - 10);
    }

    int value_y = height / 2 + 5;

    canvas.setFont(&::FreeSansBold24pt8b);
    int value_width = canvas.textWidth(main_value);
    
    canvas.setFont(&::FreeSansBold9pt8b);
    int unit_width = (unit_text && unit_text[0] != '\0') ? canvas.textWidth(unit_text) : 0;

    canvas.setFont(&::FreeSansBold24pt8b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setTextDatum(middle_center);
    canvas.drawString(main_value, width / 2, value_y);

    int unit_x = (width / 2) + (value_width / 2) + 6;

    if (unit_width > 0) {
        canvas.setFont(&::FreeSansBold9pt8b);
        canvas.setTextColor(theme_color, TFT_BLACK);
        canvas.setTextDatum(bottom_left);
        canvas.drawString(unit_text, unit_x, value_y + 12);
    }

    // 3. Posicionamos el icono después de la unidad
    if (icon != IconType::NONE) {
        int icon_x = unit_x + unit_width + 15; 
        if (icon_x < width - 20) {
            drawPlaceholderIcon(icon, icon_x, value_y - 5, theme_color); 
        }
    }

    if (bottom_left_text && bottom_left_text[0] != '\0') {
        canvas.setFont(&::FreeSansBold9pt8b);
        
        canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        canvas.setTextDatum(bottom_left);
        canvas.drawString(bottom_left_text, 15, height - 10);
    }

    canvas.setFont(&::FreeSans9pt8b);
}

void ScreenHandler::renderMenu() {
    if (_settings.display_style == DisplayStyle::CLASSIC_AMBER) {
        drawMenuUI(AMBER_RETRO, TFT_BLACK, TFT_BLACK);
    } else {
        drawMenuUI(TFT_BLACK, TFT_WHITE, getThemeColorValue(_settings.modern_theme_color));
    }
}

void ScreenHandler::showEepromWarning() {
    _show_eeprom_warning = true;
    _warning_start_time = xTaskGetTickCount(); 
}

void ScreenHandler::drawOverlays() {
    int width = canvas.width();
    int height = canvas.height();

    // Clock Overlay
    if (_ui_mode == UIMode::DASHBOARD){
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        char time_str[10];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        
        if (_settings.display_style == DisplayStyle::CLASSIC_AMBER) {
            int line_y = height - 55;
            int bottom_y = line_y + (height - line_y) / 2;
            
            canvas.setFont(&::FreeSans12pt8b); 
            
            canvas.setTextColor(TFT_BLACK, AMBER_RETRO);
            canvas.setTextDatum(middle_right);
            canvas.drawString(time_str, width - 15, bottom_y);
        } else {
            canvas.setFont(&::FreeSansBold12pt8b);
             
            canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            canvas.setTextDatum(bottom_right);
            canvas.drawString(time_str, width - 15, height - 10);
            canvas.setFont(&::FreeSans9pt8b);
        }
    }

    // Shutdown Sequence Overlay
    if (_is_shutting_down) {
        canvas.fillRect(10, height / 2 - 30, width - 20, 60, TFT_DARKGREY);
        canvas.drawRect(10, height / 2 - 30, width - 20, 60, TFT_WHITE);
        
        canvas.setTextColor(TFT_WHITE);
        canvas.setTextDatum(middle_center);
        
        
        if (_save_success) {
            canvas.setTextColor(TFT_GREEN);
            canvas.drawString(getText(TextKey::MEM_SAVED, _settings.language), width / 2, height / 2 - 10);
            canvas.drawString(getText(TextKey::SHUTTING_DOWN, _settings.language), width / 2, height / 2 + 10);
        } else {
            canvas.setTextColor(TFT_RED);
            canvas.drawString(getText(TextKey::MEM_ERROR, _settings.language), width / 2, height / 2 - 10);
            canvas.drawString(getText(TextKey::DATA_LOST, _settings.language), width / 2, height / 2 + 10);
        }
    }
    
    if (_show_eeprom_warning) {
        if (xTaskGetTickCount() - _warning_start_time < pdMS_TO_TICKS(5000)) {
            canvas.fillRect(10, 10, width - 20, 40, TFT_RED);
            canvas.drawRect(10, 10, width - 20, 40, TFT_WHITE);
            
            canvas.setTextColor(TFT_WHITE);
            canvas.setTextDatum(middle_center);
            
            canvas.drawString(getText(TextKey::EEPROM_CORRUPT, _settings.language), width / 2, 30);
        } else {
            _show_eeprom_warning = false;
        }
    }
}

void ScreenHandler::drawMenuUI(uint16_t bg_color, uint16_t text_color, uint16_t highlight_color) {
    canvas.fillSprite(bg_color);
    
    int w = canvas.width();
    int h = canvas.height();

    const char* page_titles[] = {
        getText(TextKey::MENU_SYSTEM_SETTINGS_TITLE, _settings.language),
        getText(TextKey::MENU_RPM_TITLE, _settings.language),
        getText(TextKey::MENU_CLOCK_TITLE, _settings.language),
        getText(TextKey::MENU_RESET_TITLE, _settings.language)
    };

    // Level 1: Page navigation (Large font)
    if (_menu_level == MenuLevel::PAGE_SELECT) {
        canvas.setFont(&::FreeSansBold18pt8b);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(highlight_color, bg_color);
        
        canvas.drawString(page_titles[static_cast<int>(_current_settings_page)], w / 2, h / 2 - 20);

        canvas.setFont(&::FreeSans9pt8b);
        canvas.setTextColor(text_color, bg_color);
        canvas.drawString(getText(TextKey::MENU_PRESS_TRIP, _settings.language), w / 2, h - 70);
        canvas.drawString(getText(TextKey::MENU_CHANGE_PAGE, _settings.language), w / 2, h - 50);
        return; 
    }

    // Levels 2 and 3: Inside a specific page
    canvas.setFont(&::FreeSansBold12pt8b);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(highlight_color, bg_color);
    canvas.drawString(page_titles[static_cast<int>(_current_settings_page)], w / 2, (h / 4) - 10);
    canvas.drawFastHLine(10, (h / 4) + 12, w - 20, highlight_color);

    // Special screen for clock configuration
    if (_current_settings_page == SettingsPage::CLOCK_CONFIGURATION) {
        canvas.setTextDatum(middle_center);
        canvas.setFont(&::FreeSansBold24pt8b);

        bool blink_off = false;
        if (_menu_level == MenuLevel::EDIT_VALUE){
            uint32_t ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
            blink_off = (ms / 500) % 2 != 0;
        }
        
        char hr_buf[4], min_buf[4];
        snprintf(hr_buf, sizeof(hr_buf), "%02d", _edit_hour);
        snprintf(min_buf, sizeof(min_buf), "%02d", _edit_minute);

        int colon_w = canvas.textWidth(":");
        int offset = (colon_w / 2) + 4; 

        canvas.setTextDatum(middle_center);
        canvas.drawString(":", w/2, h/2);

        if (!(blink_off && _clock_edit_step == ClockEditStep::HOURS)) {
            canvas.setTextDatum(middle_right);
            canvas.drawString(hr_buf, w/2 - offset, h/2);
        }

        if (!(blink_off && _clock_edit_step == ClockEditStep::MINUTES)) {
            canvas.setTextDatum(middle_left);
            canvas.drawString(min_buf, w/2 + offset, h/2);
        }

        canvas.setFont(&::FreeSans9pt8b);
        canvas.setTextColor(highlight_color);
        if (_clock_edit_step == ClockEditStep::HOURS) {
            canvas.drawString("^^", w/2 - 35, h/2 + 30);
        } else {
            canvas.drawString("^^", w/2 + 35, h/2 + 30);
        }
        return;
    }

    canvas.setFont(&::FreeSans12pt8b);
    int y_opt1 = (h / 2) + 5;
    
    // Extra spacing for readability
    int y_opt2 = (h / 2) + 50; 

    // Extra space for the ARE_YOU_SURE text
    if (_current_settings_page == SettingsPage::RESET_TRIP) {
        y_opt1 += 15; 
        y_opt2 += 15; 
    }

    char opt1_text[32], opt2_text[32];
    char val1_text[16], val2_text[16];

    if (_current_settings_page == SettingsPage::REGIONAL_SETUP) {
        snprintf(opt1_text, sizeof(opt1_text), getText(TextKey::MENU_SYSTEM, _settings.language));
        snprintf(val1_text, sizeof(val1_text), _settings.unit_system == MeasurementSystem::METRIC ? getText(TextKey::MENU_METRIC, _settings.language) : getText(TextKey::MENU_IMPERIAL, _settings.language));
        snprintf(opt2_text, sizeof(opt2_text), getText(TextKey::MENU_LANGUAGE, _settings.language));
        snprintf(val2_text, sizeof(val2_text), getLanguageName(_settings.language));
    }
    else if (_current_settings_page == SettingsPage::DYNAMIC_RPM_COLOR) {
        const char* color_names[] = {
            getText(TextKey::COLOR_BLUE, _settings.language),
            getText(TextKey::COLOR_CYAN, _settings.language),
            getText(TextKey::COLOR_GREEN, _settings.language),
            getText(TextKey::COLOR_YELLOW, _settings.language),
            getText(TextKey::COLOR_RED, _settings.language),
            getText(TextKey::COLOR_WHITE, _settings.language)};
        snprintf(opt1_text, sizeof(opt1_text), getText(TextKey::MENU_DYNAMIC, _settings.language));
        snprintf(opt2_text, sizeof(opt2_text), getText(TextKey::MENU_THEME, _settings.language));
        snprintf(val1_text, sizeof(val1_text), _settings.dynamic_rpm_color ? getText(TextKey::MENU_ON, _settings.language) : getText(TextKey::MENU_OFF, _settings.language));
        snprintf(val2_text, sizeof(val2_text), color_names[static_cast<int>(_settings.modern_theme_color)]);
    }
    else if (_current_settings_page == SettingsPage::RESET_TRIP) {
        canvas.setTextDatum(middle_center);
        canvas.drawString(getText(TextKey::MENU_ARE_YOU_SURE, _settings.language), w / 2, (h / 2) - 15);
        
        snprintf(opt1_text, sizeof(opt1_text), getText(TextKey::MENU_CANCEL, _settings.language));
        snprintf(opt2_text, sizeof(opt2_text), getText(TextKey::MENU_CLEAR_DATA, _settings.language));
        snprintf(val1_text, sizeof(val1_text), getText(TextKey::MENU_NO, _settings.language));
        snprintf(val2_text, sizeof(val2_text), getText(TextKey::MENU_YES, _settings.language));
    }

    char draw_buf[32];

    if (_current_sub_option == 0) canvas.setTextColor(highlight_color, bg_color);
    else canvas.setTextColor(text_color, bg_color);
    
    canvas.setTextDatum(middle_left);
    canvas.drawString(opt1_text, 40, y_opt1);
    canvas.setTextDatum(middle_right);
    
    if (_menu_level == MenuLevel::EDIT_VALUE && _current_sub_option == 0) {
        snprintf(draw_buf, sizeof(draw_buf), "< %s >", val1_text);
        canvas.drawString(draw_buf, w - 20, y_opt1);
    } else {
        canvas.drawString(val1_text, w - 30, y_opt1);
    }

    if (opt2_text[0] != '\0') {
        if (_current_sub_option == 1) canvas.setTextColor(highlight_color, bg_color);
        else canvas.setTextColor(text_color, bg_color);

        canvas.setTextDatum(middle_left);
        canvas.drawString(opt2_text, 40, y_opt2);
        canvas.setTextDatum(middle_right);
        
        if (_menu_level == MenuLevel::EDIT_VALUE && _current_sub_option == 1) {
            snprintf(draw_buf, sizeof(draw_buf), "< %s >", val2_text);
            canvas.drawString(draw_buf, w - 20, y_opt2);
        } else {
            canvas.drawString(val2_text, w - 30, y_opt2);
        }
    }

    // Draw cursor
    canvas.setTextColor(highlight_color, bg_color);
    canvas.setTextDatum(middle_left);
    if (_current_sub_option == 0) canvas.drawString(">", 20, y_opt1);
    else canvas.drawString(">", 20, y_opt2);
}

void ScreenHandler::setShutdownState(bool active, bool success) {
    _is_shutting_down = active;
    _save_success = success;
}

void ScreenHandler::handleButtonMenu() {
    if (_ui_mode == UIMode::DASHBOARD) {
        _ui_mode = UIMode::SETTINGS;
        _menu_level = MenuLevel::PAGE_SELECT;
        _current_settings_page = SettingsPage::REGIONAL_SETUP;
    } else {
        // Go back one level or exit
        if (_menu_level == MenuLevel::EDIT_VALUE) {
            if (_current_settings_page == SettingsPage::CLOCK_CONFIGURATION) {
                _menu_level = MenuLevel::PAGE_SELECT;
                _clock_edit_step = ClockEditStep::NONE;
            } else {
                _menu_level = MenuLevel::SUB_SELECT;
            }
        } else if (_menu_level == MenuLevel::SUB_SELECT) {
            _menu_level = MenuLevel::PAGE_SELECT;
        } else if (_menu_level == MenuLevel::PAGE_SELECT) {
            _ui_mode = UIMode::DASHBOARD;
        }
    }
}

void ScreenHandler::handleButtonTrip() {
    if (_ui_mode == UIMode::DASHBOARD) {
        nextPage();
        return;
    }

    if (_menu_level == MenuLevel::PAGE_SELECT) {
        if (_current_settings_page == SettingsPage::CLOCK_CONFIGURATION) {
            // Clock configuration bypasses sub-selection and goes directly to hours edit
            _menu_level = MenuLevel::EDIT_VALUE;
            _clock_edit_step = ClockEditStep::HOURS;
            
            // Load current time into temporary variables
            time_t now; struct tm timeinfo;
            time(&now); localtime_r(&now, &timeinfo);
            _edit_hour = timeinfo.tm_hour;
            _edit_minute = timeinfo.tm_min;
        } else {
            _menu_level = MenuLevel::SUB_SELECT;
            _current_sub_option = 0;
        }
    } 
    else if (_menu_level == MenuLevel::SUB_SELECT) {
        if (_current_settings_page == SettingsPage::RESET_TRIP) {
            // Execute Reset directly
            if (_current_sub_option == 1) { 
                if (_on_trip_reset_cb) { 
                    _on_trip_reset_cb(); 
                }
            }
            // Return to main menu regardless of choice
            _menu_level = MenuLevel::PAGE_SELECT;
            _current_sub_option = 0;
        } else {
            _menu_level = MenuLevel::EDIT_VALUE;
        }
    }
    else if (_menu_level == MenuLevel::EDIT_VALUE) {
        if (_current_settings_page == SettingsPage::CLOCK_CONFIGURATION) {
            if (_clock_edit_step == ClockEditStep::HOURS) {
                _clock_edit_step = ClockEditStep::MINUTES;
            } else if (_clock_edit_step == ClockEditStep::MINUTES) {
                // Save system time and exit
                time_t now;
                struct tm t;
                time(&now);
                localtime_r(&now, &t);
                t.tm_hour = _edit_hour;
                t.tm_min = _edit_minute;
                struct timeval tv = { mktime(&t), 0 };
                settimeofday(&tv, NULL);

                if (_on_clock_set_cb) {
                    _on_clock_set_cb(_edit_hour, _edit_minute);
                }
                
                _clock_edit_step = ClockEditStep::NONE;
                _menu_level = MenuLevel::PAGE_SELECT; 
            }
        } else {
            _menu_level = MenuLevel::SUB_SELECT;
        }
    }
}

void ScreenHandler::handleButtonPlus() {
    if (_ui_mode == UIMode::DASHBOARD) {  return; }

    if (_menu_level == MenuLevel::PAGE_SELECT) {
        int next = (static_cast<int>(_current_settings_page) + 1) % static_cast<int>(SettingsPage::MAX_PAGES);
        _current_settings_page = static_cast<SettingsPage>(next);
    } 
    else if (_menu_level == MenuLevel::SUB_SELECT) {
        _current_sub_option = (_current_sub_option + 1) % 2;
    }
    else if (_menu_level == MenuLevel::EDIT_VALUE) {
        if (_current_settings_page == SettingsPage::REGIONAL_SETUP) {
            if (_current_sub_option == 0) {
                _settings.unit_system = (_settings.unit_system == MeasurementSystem::METRIC) ? MeasurementSystem::IMPERIAL : MeasurementSystem::METRIC;
            } else if (_current_sub_option == 1) {
                int next_lang = (static_cast<int>(_settings.language) + 1) % static_cast<int>(SystemLanguage::MAX_LANGUAGES);
                _settings.language = static_cast<SystemLanguage>(next_lang);
            }
        } else if (_current_settings_page == SettingsPage::DYNAMIC_RPM_COLOR) {
            if (_current_sub_option == 0) {
                _settings.dynamic_rpm_color = !_settings.dynamic_rpm_color;
            } else {
                int next_col = (static_cast<int>(_settings.modern_theme_color) + 1) % static_cast<int>(ModernThemeColor::MAX_COLORS);
                _settings.modern_theme_color = static_cast<ModernThemeColor>(next_col);
            }
        } else if (_current_settings_page == SettingsPage::CLOCK_CONFIGURATION) {
            if (_clock_edit_step == ClockEditStep::HOURS) {
                _edit_hour = (_edit_hour + 1) % 24;
            } else if (_clock_edit_step == ClockEditStep::MINUTES) {
                _edit_minute = (_edit_minute + 1) % 60;
            }
        }
    }
}

void ScreenHandler::handleButtonMinus() {
    if (_ui_mode == UIMode::DASHBOARD) { return; }

    if (_menu_level == MenuLevel::PAGE_SELECT) {
        int prev = (static_cast<int>(_current_settings_page) - 1 + static_cast<int>(SettingsPage::MAX_PAGES)) % static_cast<int>(SettingsPage::MAX_PAGES);
        _current_settings_page = static_cast<SettingsPage>(prev);
    } 
    else if (_menu_level == MenuLevel::SUB_SELECT) {
        _current_sub_option = (_current_sub_option - 1 + 2) % 2;
    }
    else if (_menu_level == MenuLevel::EDIT_VALUE) {
        // Binary options toggle the same way as the Plus button
        // Clock and color options rotate backwards
        if (_current_settings_page == SettingsPage::DYNAMIC_RPM_COLOR && _current_sub_option == 1) {
            int prev_col = (static_cast<int>(_settings.modern_theme_color) - 1 + static_cast<int>(ModernThemeColor::MAX_COLORS)) % static_cast<int>(ModernThemeColor::MAX_COLORS);
            _settings.modern_theme_color = static_cast<ModernThemeColor>(prev_col);

        } else if (_current_settings_page == SettingsPage::REGIONAL_SETUP && _current_sub_option == 1) {
            int prev_lang = (static_cast<int>(_settings.language) - 1 + static_cast<int>(SystemLanguage::MAX_LANGUAGES)) % static_cast<int>(SystemLanguage::MAX_LANGUAGES);
            _settings.language = static_cast<SystemLanguage>(prev_lang);
            
        } else if (_current_settings_page == SettingsPage::CLOCK_CONFIGURATION) {
            if (_clock_edit_step == ClockEditStep::HOURS) {
                _edit_hour = (_edit_hour - 1 + 24) % 24;
            } else if (_clock_edit_step == ClockEditStep::MINUTES) {
                _edit_minute = (_edit_minute - 1 + 60) % 60;
            }
        } else {
            // Reuse logic for binary toggles
            handleButtonPlus(); 
        }
    }
}

void ScreenHandler::handleComboTripPlus() {
    if (_ui_mode == UIMode::DASHBOARD) {
        toggleStyle();
    }
}

uint16_t ScreenHandler::getThemeColorValue(ModernThemeColor color) {
    switch (color) {
        case ModernThemeColor::BLUE:   return TFT_BLUE;
        case ModernThemeColor::CYAN:   return TFT_CYAN;
        case ModernThemeColor::GREEN:  return TFT_GREEN;
        case ModernThemeColor::YELLOW: return TFT_YELLOW;
        case ModernThemeColor::RED:    return TFT_RED;
        case ModernThemeColor::WHITE:  return TFT_WHITE;
        default: return TFT_CYAN;
    }
}