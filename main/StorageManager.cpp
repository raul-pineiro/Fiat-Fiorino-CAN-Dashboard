#include "StorageManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include "esp_log.h"

/**
 * @brief Constructs the StorageManager instance.
 * 
 * @param port I2C port number.
 * @param sda GPIO number for the I2C SDA line.
 * @param scl GPIO number for the I2C SCL line.
 */
StorageManager::StorageManager(i2c_port_t port, gpio_num_t sda, gpio_num_t scl) 
    : _port(port), _sda(sda), _scl(scl) {}

/**
 * @brief Initializes the I2C master bus and configures the EEPROM device.
 * 
 * @return true if the bus and device were successfully initialized.
 */
bool StorageManager::begin() {
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = _port;
    bus_config.sda_io_num = _sda;
    bus_config.scl_io_num = _scl;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    if (i2c_new_master_bus(&bus_config, &_bus_handle) != ESP_OK) return false;

    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = EEPROM_ADDR;
    dev_config.scl_speed_hz = 400000;

    if (i2c_master_bus_add_device(_bus_handle, &dev_config, &_eeprom_handle) != ESP_OK) return false;

    return true;
}

/**
 * @brief Writes a block of data to a specific memory address in the EEPROM.
 * 
 * @param mem_addr The 16-bit internal memory address of the EEPROM.
 * @param data Pointer to the payload buffer to be written.
 * @param len Number of bytes to write.
 * @return true if the I2C transmission was successful and length is valid.
 */
bool StorageManager::writeEEPROM(uint16_t mem_addr, const uint8_t* data, size_t len) {
    // 64 bytes is more than enough for car metrics and 100% safe for ESP32.
    constexpr size_t MAX_PAYLOAD_SIZE = 64; 

    if (len == 0 || len > MAX_PAYLOAD_SIZE) {
        return false; 
    }

    uint8_t buffer[MAX_PAYLOAD_SIZE + 2];
    
    // I2C EEPROM format requires a 2-byte memory address preceding the payload
    buffer[0] = (uint8_t)(mem_addr >> 8);
    buffer[1] = (uint8_t)(mem_addr & 0xFF);
    
    memcpy(&buffer[2], data, len);

    esp_err_t ret = i2c_master_transmit(_eeprom_handle, buffer, 2 + len, pdMS_TO_TICKS(100));
    
    // Blocks task to allow the EEPROM's internal physical write cycle to complete
    vTaskDelay(pdMS_TO_TICKS(10)); 
    return (ret == ESP_OK);
}

/**
 * @brief Reads a block of data from a specific memory address in the EEPROM.
 * 
 * @param mem_addr The 16-bit internal memory address of the EEPROM.
 * @param data Pointer to the buffer where the read data will be stored.
 * @param len Number of bytes to read.
 * @return true if the I2C transaction was successful.
 */
bool StorageManager::readEEPROM(uint16_t mem_addr, uint8_t* data, size_t len) {
    uint8_t addr_buf[2] = { (uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF) };
    esp_err_t ret = i2c_master_transmit_receive(_eeprom_handle, addr_buf, 2, data, len, pdMS_TO_TICKS(100));
    return (ret == ESP_OK);
}

/**
 * @brief Saves the extra kilometer value to the EEPROM with a validation header.
 * 
 * @param km The kilometer value to store.
 * @return true if the write operation was successful.
 */
bool StorageManager::saveExtraKM(uint32_t km) {
    uint8_t buffer[8];
    uint32_t magic = MAGIC_NUMBER; 
    
    memcpy(&buffer[0], &magic, 4);
    memcpy(&buffer[4], &km, 4);
    
    return writeEEPROM(MEM_ADDR, buffer, 8);
}

/**
 * @brief Loads and validates the extra kilometer value from the EEPROM.
 * 
 * @param km Reference to store the retrieved kilometer value.
 * @return true if the data was successfully read and validated against the magic number.
 */
bool StorageManager::loadExtraKM(uint32_t &km) {
    uint8_t buffer[8];
    if (!readEEPROM(MEM_ADDR, buffer, 8)) return false;

    uint32_t magic;
    memcpy(&magic, &buffer[0], 4);
    
    if (magic == MAGIC_NUMBER) {
        memcpy(&km, &buffer[4], 4);
        return true;
    }
    return false;
}