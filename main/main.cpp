#include <stdio.h>
#include <sys/time.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"

#include "CanManager.h"
#include "ScreenHandler.h"
#include "FiatCanParser.h"
#include "StorageManager.h"

/**
 * @def ENABLE_USB_SNIFFER
 * @brief Toggles the SLCAN compatible USB sniffer output for development.
 */
#define ENABLE_USB_SNIFFER 1

ScreenHandler screen;
CanManager can(GPIO_NUM_26, GPIO_NUM_25);
StorageManager storage(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);

SemaphoreHandle_t dataMutex;

uint32_t extra_km = 0;             
volatile bool shutdown_flag = false; 

/**
 * @brief Thread-safe container for telemetry data shared between cores.
 */
struct SharedData {
    uint32_t total_km = 0;
    FiatCAN::EngineData engine = {};
} shared_data;

/**
 * @brief ISR handler triggered by the ignition cutoff sensor (GPIO 34).
 * 
 * @param arg Unused ISR parameter.
 */
static void IRAM_ATTR ignition_isr_handler(void* arg) {
    shutdown_flag = true;
}

/**
 * @brief Converts a Binary-Coded Decimal (BCD) byte to its decimal equivalent.
 * 
 * @param val BCD formatted byte.
 * @return uint8_t Decimal equivalent.
 */
static uint8_t bcd2dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

/**
 * @brief Reads time from an external DS3231 RTC module and synchronizes the ESP32's internal system clock.
 */
void syncRTCWithDS3231() {
    i2c_master_bus_handle_t bus = storage.getBusHandle();
    if (bus == nullptr) {
#if !ENABLE_USB_SNIFFER
        printf("Error: I2C Bus not initialized. Cannot read RTC.\n");
#endif
        return;
    }

    i2c_device_config_t rtc_config = {};
    rtc_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    rtc_config.device_address = 0x68;
    rtc_config.scl_speed_hz = 100000;
    
    i2c_master_dev_handle_t rtc_handle;
    if (i2c_master_bus_add_device(bus, &rtc_config, &rtc_handle) != ESP_OK) return;

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
#if !ENABLE_USB_SNIFFER
        printf("RTC synchronized successfully.\n");
#endif
    } else {
#if !ENABLE_USB_SNIFFER
        printf("Error reading data from RTC.\n");
#endif
    }
}

/**
 * @brief Core 0 Task: Real-time CAN Bus engine.
 * Handles packet reception, structural decoding, and thread-safe data publishing.
 * 
 * @param pvParameters FreeRTOS task parameters (unused).
 */
void task_can_core0(void *pvParameters) {
    CanFrameWrapper rx_msg;
    uint32_t last_missed_count = 0;

    while (1) {
        if (can.receiveMessage(rx_msg, 10)) {
            
            #if ENABLE_USB_SNIFFER
            if (rx_msg.frame.header.rtr) {
                if (rx_msg.frame.header.ide) {
                    printf("R%08lX%d", (unsigned long)rx_msg.frame.header.id, rx_msg.frame.header.dlc);
                } else {
                    printf("r%03lX%d", (unsigned long)rx_msg.frame.header.id, rx_msg.frame.header.dlc);
                }
            } else {
                if (rx_msg.frame.header.ide) {
                    printf("T%08lX%d", (unsigned long)rx_msg.frame.header.id, rx_msg.frame.header.dlc);
                } else {
                    printf("t%03lX%d", (unsigned long)rx_msg.frame.header.id, rx_msg.frame.header.dlc);
                }
                for (int i = 0; i < rx_msg.frame.header.dlc; i++) {
                    printf("%02X", rx_msg.payload[i]);
                }
            }
            printf("\r");
            #endif

            uint32_t id = rx_msg.frame.header.id;
            uint8_t dlc = rx_msg.frame.header.dlc;
            uint8_t* payload = rx_msg.payload;

            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
                if (id == FiatCAN::ID_CLUSTER_KM) {
                    uint32_t raw_km = FiatCAN::parseClusterKM(payload, dlc);
                    shared_data.total_km = raw_km + extra_km; 
                } else if (id == FiatCAN::ID_ENGINE_DATA) {
                    shared_data.engine = FiatCAN::parseEngineData(payload, dlc);
                }
                xSemaphoreGive(dataMutex);
            }
        }

        uint32_t current_missed = can.getMissedMessagesCount();
        if (current_missed != last_missed_count) {
            last_missed_count = current_missed;
        }
        vTaskDelay(1);
    }
}

/**
 * @brief Core 1 Task: Visual Engine and Non-blocking Shutdown Evaluation.
 * Handles TFT rendering, style cycling, and graceful degradation during ignition cutoff.
 * 
 * @param pvParameters FreeRTOS task parameters (unused).
 */
void task_gui_core1(void *pvParameters) {
    TickType_t last_cycle_time = xTaskGetTickCount();
    const TickType_t cycle_interval = pdMS_TO_TICKS(5000);
    uint8_t cycle_state = 0;

    SharedData local_snapshot;

    bool evaluating_shutdown = false;
    TickType_t shutdown_eval_start = 0;

    while (1) {
        // 1. Non-blocking Ignition Cutoff Evaluation
        if (shutdown_flag) {
            if (!evaluating_shutdown) {
                evaluating_shutdown = true;
                shutdown_eval_start = xTaskGetTickCount();
            }

            if (gpio_get_level(GPIO_NUM_34) == 1) {
                shutdown_flag = false;
                evaluating_shutdown = false;
            } else {
                if ((xTaskGetTickCount() - shutdown_eval_start) >= pdMS_TO_TICKS(5000)) {
                    bool save_ok = storage.saveExtraKM(extra_km);
                    screen.setShutdownState(true, save_ok);
                    screen.render();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    
                    esp_deep_sleep_start();
                }
            }
        }

        // 2. Auto-carousel Execution
        if ((xTaskGetTickCount() - last_cycle_time) >= cycle_interval) {
            cycle_state = (cycle_state + 1) % 4;
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                switch (cycle_state) {
                    case 0:
                        screen.setStyle(DisplayStyle::CLASSIC_AMBER);
                        screen.setPage(DisplayPage::MAIN_DASH);
                        break;
                    case 1:
                        screen.setStyle(DisplayStyle::CLASSIC_AMBER);
                        screen.setPage(DisplayPage::TRIP_INFO);
                        break;
                    case 2:
                        screen.setStyle(DisplayStyle::MODERN_DARK);
                        screen.setPage(DisplayPage::MAIN_DASH);
                        break;
                    case 3:
                        screen.setStyle(DisplayStyle::MODERN_DARK);
                        screen.setPage(DisplayPage::TRIP_INFO);
                        break;
                }
                xSemaphoreGive(dataMutex);
            }
            last_cycle_time = xTaskGetTickCount();
        }

        // 3. Thread-safe Data Copy and Render
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            local_snapshot = shared_data;
            xSemaphoreGive(dataMutex);
        }

        screen.updateKM(local_snapshot.total_km);
        screen.updateEngine(local_snapshot.engine);
        
        screen.render();

        // 4. Mandatory delay to feed the watchdog (~20 FPS)
        vTaskDelay(pdMS_TO_TICKS(40)); 
    }
}

/**
 * @brief Application entry point. Bootstraps hardware, synchronization primitives, and FreeRTOS tasks.
 */
extern "C" void app_main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
#if !ENABLE_USB_SNIFFER
        printf("Error: Failed to create Mutex\n");
#endif
        return;
    }

    // Configure GPIO 34 (Ignition Key Sensor)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE; 
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_34);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_34, ignition_isr_handler, NULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 1);

    // Initialize EEPROM and RTC
    if (storage.begin()) {
        syncRTCWithDS3231(); 
        if (!storage.loadExtraKM(extra_km)) {
            extra_km = 15;
            storage.saveExtraKM(extra_km); 
        }
    }

    screen.begin();
    if (!can.begin()) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }

    xTaskCreatePinnedToCore(task_can_core0, "Task_CAN", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_gui_core1, "Task_GUI", 8192, NULL, 2, NULL, 1);
}