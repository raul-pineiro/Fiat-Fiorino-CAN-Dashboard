#include "ScreenHandler.h"
#include <stdio.h>
#include <time.h>

ScreenHandler::LGFX_Config::LGFX_Config() {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI3_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 20000000;
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
      _current_style(DisplayStyle::CLASSIC_AMBER),
      _current_page(DisplayPage::MAIN_DASH),
      _current_km(0), 
      _rpm(0), 
      _consumption(0.0f), 
      _temp(0), 
      _fuel(0), 
      _trip_mode(FiatCAN::TRIP_IDLE) {}

ScreenHandler::~ScreenHandler() {
    canvas.deleteSprite();
}

void ScreenHandler::begin() {
    lcd.init();
    lcd.setBrightness(255);
    lcd.setRotation(1);
    
    canvas.setColorDepth(16);
    canvas.createSprite(lcd.width(), lcd.height());
}

void ScreenHandler::updateKM(uint32_t km) { 
    _current_km = km; 
}

void ScreenHandler::updateEngine(const FiatCAN::EngineData& engine) {
    _rpm = engine.rpm;
    _consumption = engine.consumption_lh;
    _temp = engine.temp;
    _fuel = engine.fuel;
}

void ScreenHandler::updateTripMode(FiatCAN::TripMode mode) { 
    _trip_mode = mode; 
}

void ScreenHandler::setStyle(DisplayStyle style) {
    _current_style = style;
}

void ScreenHandler::toggleStyle() {
    _current_style = (_current_style == DisplayStyle::CLASSIC_AMBER) 
                     ? DisplayStyle::MODERN_DARK 
                     : DisplayStyle::CLASSIC_AMBER;
}

void ScreenHandler::nextPage() {
    _current_page = (_current_page == DisplayPage::MAIN_DASH) 
                    ? DisplayPage::TRIP_INFO 
                    : DisplayPage::MAIN_DASH;
}

void ScreenHandler::setPage(DisplayPage page) {
    _current_page = page;
}

void ScreenHandler::render() {
    if (_current_style == DisplayStyle::CLASSIC_AMBER) {
        if (_current_page == DisplayPage::MAIN_DASH) {
            drawClassicMain();
        } else {
            drawClassicTrip();
        }
    } else {
        if (_current_page == DisplayPage::MAIN_DASH) {
            drawModernMain();
        } else {
            drawModernTrip();
        }
    }

    drawOverlays();
    canvas.pushSprite(0, 0);
}

void ScreenHandler::drawClassicMain() {
    canvas.fillSprite(AMBER_RETRO);
    int width = canvas.width();
    int height = canvas.height();

    canvas.setTextColor(TFT_BLACK, AMBER_RETRO);
    canvas.setTextDatum(middle_center);
    
    canvas.setTextSize(3);
    canvas.drawString("TOTAL KM", width / 2, height / 2 - 10);
    
    canvas.setTextSize(4);
    canvas.drawNumber(_current_km, width / 2, height / 3 - 15);
    
    int line_y = height - 55;
    canvas.drawFastHLine(15, line_y, width - 30, TFT_BLACK);
    
    int bottom_y = line_y + (height - line_y) / 2;
    canvas.setTextDatum(middle_left);
    canvas.setTextSize(3);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "RPM: %lu", (unsigned long)_rpm);
    canvas.drawString(buf, 15, bottom_y);
}

void ScreenHandler::drawClassicTrip() {
    canvas.fillSprite(AMBER_RETRO);
    int width = canvas.width();
    int height = canvas.height();

    canvas.setTextColor(TFT_BLACK, AMBER_RETRO);
    canvas.setTextDatum(middle_center);
    
    canvas.setTextSize(3);
    canvas.drawString("INST. CONSUMPTION", width / 2, height / 2 - 10);
    
    canvas.setTextSize(4);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f L/h", _consumption);
    canvas.drawString(buf, width / 2, height / 3 - 15);
    
    int line_y = height - 55;
    canvas.drawFastHLine(15, line_y, width - 30, TFT_BLACK);
    
    int bottom_y = line_y + (height - line_y) / 2;
    canvas.setTextDatum(middle_left);
    canvas.setTextSize(3);
    
    snprintf(buf, sizeof(buf), "Temp: %d C", _temp);
    canvas.drawString(buf, 15, bottom_y);
}

void ScreenHandler::drawModernMain() {
    canvas.fillSprite(TFT_BLACK); 
    int width = canvas.width();
    int height = canvas.height();

    canvas.drawFastHLine(20, height / 3 - 10, width - 40, TFT_CYAN);

    canvas.setFont(&fonts::Font2); 
    canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.drawString("ODO", width / 2, height / 4 - 10);
    
    canvas.setFont(&fonts::FreeSansBold24pt7b); 
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1); 
    canvas.drawNumber(_current_km, width / 2 - 15, height / 2 + 5);
    
    canvas.setFont(&fonts::Font2);
    canvas.drawString("km", width / 2 + 75, height / 2 + 10);

    // Reset font for subsequent renders
    canvas.setFont(nullptr);
}

void ScreenHandler::drawModernTrip() {
    canvas.fillSprite(TFT_BLACK);
    int width = canvas.width();
    int height = canvas.height();

    canvas.fillRoundRect(10, height / 3 - 20, width - 20, 100, 8, TFT_DARKGREY);

    canvas.setFont(&fonts::Font2);
    canvas.setTextColor(TFT_CYAN, TFT_BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.drawString("INSTANTANEOUS", width / 2, 30);

    canvas.setFont(&fonts::FreeSansBold24pt7b); 
    canvas.setTextColor(TFT_WHITE, TFT_DARKGREY);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", _consumption);
    canvas.drawString(buf, width / 2, height / 2 - 5);
    
    canvas.setFont(&fonts::Font2);
    canvas.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
    canvas.drawString("L/h", width / 2, height / 2 + 25);

    canvas.setFont(nullptr);
}

void ScreenHandler::drawOverlays() {
    int width = canvas.width();
    int height = canvas.height();

    // Clock Overlay
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char time_str[10];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    
    if (_current_style == DisplayStyle::CLASSIC_AMBER) {
        int line_y = height - 55;
        int bottom_y = line_y + (height - line_y) / 2;
        
        canvas.setFont(nullptr);
        canvas.setTextSize(3);
        canvas.setTextColor(TFT_BLACK, AMBER_RETRO);
        canvas.setTextDatum(middle_right);
        canvas.drawString(time_str, width - 15, bottom_y);
    } else {
        canvas.setFont(&fonts::FreeSansBold12pt7b);
        canvas.setTextSize(1); 
        canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        canvas.setTextDatum(bottom_right);
        canvas.drawString(time_str, width - 15, height - 10);
        canvas.setFont(nullptr);
    }

    // Shutdown Sequence Overlay
    if (_is_shutting_down) {
        canvas.fillRect(10, height / 2 - 30, width - 20, 60, TFT_DARKGREY);
        canvas.drawRect(10, height / 2 - 30, width - 20, 60, TFT_WHITE);
        
        canvas.setTextColor(TFT_WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(1);
        
        if (_save_success) {
            canvas.setTextColor(TFT_GREEN);
            canvas.drawString("Memory Saved", width / 2, height / 2 - 10);
            canvas.drawString("Shutting down...", width / 2, height / 2 + 10);
        } else {
            canvas.setTextColor(TFT_RED);
            canvas.drawString("MEMORY ERROR", width / 2, height / 2 - 10);
            canvas.drawString("Data lost", width / 2, height / 2 + 10);
        }
    }
}

void ScreenHandler::setShutdownState(bool active, bool success) {
    _is_shutting_down = active;
    _save_success = success;
}