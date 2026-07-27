#pragma once

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "driver/i2c_master.h"

/**
 * @class RtcManager
 * @brief Manages I2C communication with the DS3231 RTC module and handles ESP32 system clock synchronization.
 */
class RtcManager {
public:
    /**
     * @brief Default constructor for RtcManager.
     */
    RtcManager();

    /**
     * @brief Initializes the RTC manager with an existing I2C master bus handle.
     * 
     * @param bus Active I2C master bus handle provided by the storage manager.
     */
    void begin(i2c_master_bus_handle_t bus);

    /**
     * @brief Reads time from the external DS3231 RTC module and synchronizes the ESP32 internal system clock.
     */
    void syncSystemTime();

    /**
     * @brief Writes updated time values to the DS3231 RTC module and immediately updates the internal system clock.
     * 
     * @param hour Hours value in 24-hour format (0-23).
     * @param minute Minutes value (0-59).
     * @return true if I2C transmission succeeded and time was updated, false otherwise.
     */
    bool setTime(uint8_t hour, uint8_t minute);

private:
    i2c_master_bus_handle_t _bus;

    /**
     * @brief Converts a Binary-Coded Decimal (BCD) byte to its decimal equivalent.
     * 
     * @param val BCD formatted byte.
     * @return uint8_t Decimal equivalent.
     */
    uint8_t bcd2dec(uint8_t val);

    /**
     * @brief Converts a decimal byte to its Binary-Coded Decimal (BCD) equivalent.
     * 
     * @param val Decimal formatted byte.
     * @return uint8_t BCD equivalent.
     */
    uint8_t dec2bcd(uint8_t val);
};