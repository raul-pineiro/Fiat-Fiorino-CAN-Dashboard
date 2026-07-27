#pragma once
#include "driver/i2c_master.h"
#include <stdint.h>
#include "SystemSettings.h"

/**
 * @struct DashboardData
 * @brief Represents the complete state of the dashboard to be persisted in memory.
 */
struct DashboardData {
    uint32_t magic;                 /**< Magic number for data validation and corruption detection */
    uint32_t total_km;              /**< Odometer total accumulated kilometers */
    float fractional_km;            /**< Fractional part of the current kilometer (0.0 to <1.0) */
    SystemSettings settings;        /**< Current user preferences and system settings */
    float trip_km;                  /**< Distance covered in the current trip */
    uint32_t trip_time;             /**< Elapsed time for the current trip in seconds */
    float trip_fuel_consumed;       /**< Accumulated fuel consumed in the current trip in liters */
    float recent_avg_l_100km;       /**< Recent average fuel consumption in liters per 100 kilometers */
};

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
     * @brief Retrieves the underlying I2C master bus handle.
     * 
     * Exposes the handle to allow other I2C devices (e.g., RTC) to share the same bus 
     * without requiring separate initialization.
     * 
     * @return i2c_master_bus_handle_t The initialized I2C bus handle.
     */
    i2c_master_bus_handle_t getBusHandle() { return _bus_handle; }

    /**
     * @brief Saves the entire dashboard data structure to EEPROM with a validation header.
     * 
     * @param data Reference to the DashboardData structure to store.
     * @return true if the write operation was successful.
     */
    bool saveData(const DashboardData& data);

    /**
     * @brief Loads and validates the entire dashboard data structure from EEPROM.
     * 
     * @param data Reference to the DashboardData structure to populate.
     * @return true if the data was successfully read and validated against the magic number.
     */
    bool loadData(DashboardData& data);

private:
    i2c_port_t _port;                               /**< I2C peripheral port number */
    gpio_num_t _sda;                                /**< I2C SDA GPIO pin */
    gpio_num_t _scl;                                /**< I2C SCL GPIO pin */
    
    i2c_master_bus_handle_t _bus_handle = nullptr;  /**< Handle for the initialized I2C master bus */
    i2c_master_dev_handle_t _eeprom_handle = nullptr;/**< Handle for the EEPROM device on the I2C bus */
    
    static constexpr uint8_t EEPROM_ADDR = 0x57;    /**< Default I2C address for AT24C32 EEPROM */
    
    static constexpr uint16_t MEM_ADDR_PRIMARY = 0x0000; /**< Primary memory slot starting address */
    static constexpr uint16_t MEM_ADDR_BACKUP  = 0x0100; /**< Backup memory slot starting address for redundancy */
    
    static constexpr uint32_t MAGIC_NUMBER = 0xFA170001; /**< Unique signature to verify data integrity upon reading */

    /**
     * @brief Writes raw byte data to a specific memory address in the EEPROM.
     * 
     * @param mem_addr The 16-bit internal memory address to write to.
     * @param data Pointer to the buffer containing the data to write.
     * @param len Number of bytes to write.
     * @return true if the I2C transmission was successful.
     */
    bool writeEEPROM(uint16_t mem_addr, const uint8_t* data, size_t len);

    /**
     * @brief Reads raw byte data from a specific memory address in the EEPROM.
     * 
     * @param mem_addr The 16-bit internal memory address to read from.
     * @param data Pointer to the buffer where read data will be stored.
     * @param len Number of bytes to read.
     * @return true if the I2C reception was successful.
     */
    bool readEEPROM(uint16_t mem_addr, uint8_t* data, size_t len);
};