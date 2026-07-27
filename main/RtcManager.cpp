#include "RtcManager.h"
#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef DISABLE_DEBUG
#define DISABLE_DEBUG 1
#endif

RtcManager::RtcManager() : _bus(nullptr) {
}

void RtcManager::begin(i2c_master_bus_handle_t bus) {
    _bus = bus;
}

uint8_t RtcManager::bcd2dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

uint8_t RtcManager::dec2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

void RtcManager::syncSystemTime() {
    if (_bus == nullptr) {
#if !DISABLE_DEBUG
        printf("Error: I2C Bus not initialized. Cannot read RTC.\n");
#endif
        return;
    }

    i2c_device_config_t rtc_config = {};
    rtc_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    rtc_config.device_address = 0x68;
    rtc_config.scl_speed_hz = 100000;
    
    i2c_master_dev_handle_t rtc_handle;
    if (i2c_master_bus_add_device(_bus, &rtc_config, &rtc_handle) != ESP_OK) {
        return;
    }

    uint8_t reg = 0x00;
    uint8_t data[7];
    esp_err_t ret = i2c_master_transmit_receive(rtc_handle, &reg, 1, data, 7, pdMS_TO_TICKS(100));
    
    i2c_master_bus_rm_device(rtc_handle);

    if (ret == ESP_OK) {
        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        
        tm.tm_sec  = bcd2dec(data[0] & 0x7F);
        tm.tm_min  = bcd2dec(data[1]);
        tm.tm_hour = bcd2dec(data[2] & 0x3F);
        tm.tm_mday = bcd2dec(data[4]);
        tm.tm_mon  = bcd2dec(data[5] & 0x1F) - 1;
        tm.tm_year = bcd2dec(data[6]) + 100; 
        
        tm.tm_isdst = -1;

        time_t t = mktime(&tm);
        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&tv, NULL); 
#if !DISABLE_DEBUG
        printf("RTC synchronized successfully.\n");
#endif
    } else {
#if !DISABLE_DEBUG
        printf("Error reading data from RTC.\n");
#endif
    }
}

bool RtcManager::setTime(uint8_t hour, uint8_t minute) {
    if (_bus == nullptr) {
        return false;
    }

    i2c_device_config_t rtc_config = {};
    rtc_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    rtc_config.device_address = 0x68;
    rtc_config.scl_speed_hz = 100000;

    i2c_master_dev_handle_t rtc_handle;
    if (i2c_master_bus_add_device(_bus, &rtc_config, &rtc_handle) != ESP_OK) {
        return false;
    }

    uint8_t data[3];
    data[0] = 0x01;
    data[1] = dec2bcd(minute);
    data[2] = dec2bcd(hour) & 0x3F;

    esp_err_t ret = i2c_master_transmit(rtc_handle, data, sizeof(data), pdMS_TO_TICKS(100));
    i2c_master_bus_rm_device(rtc_handle);

    if (ret == ESP_OK) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm *tm_info = localtime(&tv.tv_sec);

        tm_info->tm_hour = hour;
        tm_info->tm_min = minute;
        tm_info->tm_sec = 0;

        time_t t = mktime(tm_info);
        struct timeval new_tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&new_tv, NULL);
        return true;
    }

    return false;
}