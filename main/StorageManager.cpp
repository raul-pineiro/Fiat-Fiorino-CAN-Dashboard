#include "StorageManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include "esp_log.h"
#include <algorithm>

StorageManager::StorageManager(i2c_port_t port, gpio_num_t sda, gpio_num_t scl) 
    : _port(port), _sda(sda), _scl(scl) {}

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

bool StorageManager::writeEEPROM(uint16_t mem_addr, const uint8_t* data, size_t len) {
    constexpr size_t PAGE_SIZE = 32; 
    size_t bytes_written = 0;

    while (bytes_written < len) {
        uint16_t current_addr = mem_addr + bytes_written;
        
        size_t bytes_left_in_page = PAGE_SIZE - (current_addr % PAGE_SIZE);
        size_t bytes_to_write = std::min(bytes_left_in_page, len - bytes_written);

        uint8_t buffer[PAGE_SIZE + 2];
        buffer[0] = (uint8_t)(current_addr >> 8);
        buffer[1] = (uint8_t)(current_addr & 0xFF);
        memcpy(&buffer[2], data + bytes_written, bytes_to_write);

        esp_err_t ret = i2c_master_transmit(_eeprom_handle, buffer, 2 + bytes_to_write, pdMS_TO_TICKS(100));
        if (ret != ESP_OK) return false;

        // EEPROM requires write cycle time (usually ~5ms)
        vTaskDelay(pdMS_TO_TICKS(10)); 
        
        bytes_written += bytes_to_write;
    }
    
    return true;
}

bool StorageManager::readEEPROM(uint16_t mem_addr, uint8_t* data, size_t len) {
    uint8_t addr_buf[2] = { (uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF) };
    esp_err_t ret = i2c_master_transmit_receive(_eeprom_handle, addr_buf, 2, data, len, pdMS_TO_TICKS(100));
    return (ret == ESP_OK);
}

bool StorageManager::saveData(const DashboardData& data) {
    bool ok1 = writeEEPROM(MEM_ADDR_PRIMARY, (const uint8_t*)&data, sizeof(DashboardData));
    bool ok2 = writeEEPROM(MEM_ADDR_BACKUP, (const uint8_t*)&data, sizeof(DashboardData));
    
    return ok1 && ok2;
}

bool StorageManager::loadData(DashboardData& data) {
    DashboardData temp_data;
    
    if (readEEPROM(MEM_ADDR_PRIMARY, (uint8_t*)&temp_data, sizeof(DashboardData))) {
        if (temp_data.magic == MAGIC_NUMBER) {
            data = temp_data;
            return true;
        }
    }
    
    // Fallback to backup memory slot
    if (readEEPROM(MEM_ADDR_BACKUP, (uint8_t*)&temp_data, sizeof(DashboardData))) {
        if (temp_data.magic == MAGIC_NUMBER) {
            data = temp_data;
            saveData(data); // Restore primary from backup
            return true;
        }
    }
    
    return false;
}