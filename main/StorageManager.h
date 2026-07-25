#pragma once
#include "driver/i2c_master.h"
#include <stdint.h>

/**
 * @brief Manages non-volatile storage operations using an I2C EEPROM (e.g., AT24C32).
 */
class StorageManager {
public:
    /**
     * @brief Constructs the StorageManager.
     * 
     * @param port I2C port number.
     * @param sda GPIO number for the I2C SDA line.
     * @param scl GPIO number for the I2C SCL line.
     */
    StorageManager(i2c_port_t port, gpio_num_t sda, gpio_num_t scl);
    
    /**
     * @brief Initializes the I2C master bus and configures the EEPROM device.
     * 
     * @return true if the bus and device were successfully initialized.
     */
    bool begin();

    /**
     * @brief Saves the extra kilometer value to the EEPROM with a validation header.
     * 
     * @param km The kilometer value to store.
     * @return true if the write operation was successful.
     */
    bool saveExtraKM(uint32_t km);

    /**
     * @brief Loads and validates the extra kilometer value from the EEPROM.
     * 
     * @param km Reference to store the retrieved kilometer value.
     * @return true if the data was successfully read and validated against the magic number.
     */
    bool loadExtraKM(uint32_t &km);

    /**
     * @brief Retrieves the underlying I2C master bus handle.
     * 
     * Exposes the handle to allow other I2C devices (e.g., RTC) to share the same bus 
     * without requiring separate initialization.
     * 
     * @return i2c_master_bus_handle_t The initialized I2C bus handle.
     */
    i2c_master_bus_handle_t getBusHandle() { return _bus_handle; }

private:
    i2c_port_t _port;
    gpio_num_t _sda;
    gpio_num_t _scl;
    
    i2c_master_bus_handle_t _bus_handle = nullptr;
    i2c_master_dev_handle_t _eeprom_handle = nullptr;
    
    // Default I2C address for AT24C32 EEPROM
    static constexpr uint8_t EEPROM_ADDR = 0x57; 
    static constexpr uint16_t MEM_ADDR = 0x0000;
    
    // Validation signature ("Fiat 1")
    static constexpr uint32_t MAGIC_NUMBER = 0xFA170001; 

    bool writeEEPROM(uint16_t mem_addr, const uint8_t* data, size_t len);
    bool readEEPROM(uint16_t mem_addr, uint8_t* data, size_t len);
};