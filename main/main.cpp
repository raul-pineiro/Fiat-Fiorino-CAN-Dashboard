/**
 * @file main.cpp
 * @brief Main entry point for the Digital Instrument Cluster firmware.
 * 
 * This module orchestrates a dual-core FreeRTOS architecture. Core 0 is dedicated 
 * to deterministic, real-time CAN bus reception and parsing, while Core 1 handles 
 * the graphical user interface, state management, and non-volatile storage operations.
 * It also includes hardware interrupt handling for graceful shutdown upon ignition cutoff.
 */

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
#include "RtcManager.h"
#include "TripComputer.h"
#include "ButtonHandler.h"

/**
 * @def ENABLE_USB_SNIFFER
 * @brief Toggles the SLCAN compatible USB sniffer output for development.
 */
#define ENABLE_USB_SNIFFER 0

/**
 * @def ENABLE_PERFORMANCE_LOGGING
 * @brief Toggles performance logging for development.
 */
#define ENABLE_PERFORMANCE_LOGGING 0

ScreenHandler screen;
CanManager can(GPIO_NUM_26, GPIO_NUM_25);
StorageManager storage(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
RtcManager rtc;

SemaphoreHandle_t dataMutex;

DashboardData my_data;
DashboardData initial_data;
volatile bool shutdown_flag = false; 
bool show_eeprom_warning = false;

/**
 * @brief Thread-safe container for telemetry data shared between cores.
 */
struct SharedData {
    TripComputer trip;
    FiatCAN::EngineData engine = {};
    float speed_kmh = 0.0f;
    uint8_t fuel_level = 0;
    uint16_t autonomy_km = 0xFFFF;
} shared_data;

/**
 * @brief ISR handler triggered by the ignition cutoff sensor (GPIO 34).
 */
static void IRAM_ATTR ignition_isr_handler(void* arg) {
    shutdown_flag = true;
}

/**
 * @brief Core 0 Task: Real-time CAN Bus engine.
 */
void task_can_core0(void *pvParameters) {
    CanFrameWrapper rx_msg;
    uint32_t last_missed_count = 0;

    TickType_t last_calc_time = xTaskGetTickCount();
    
    while (1) {
        while (can.receiveMessage(rx_msg, 10)) {
            
            #if ENABLE_USB_SNIFFER
            // Format CAN frames for SLCAN compatible tools (e.g., SavvyCAN) over serial
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

            // Short timeout ensures the CAN reception is never blocked for long by the UI core
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
                if (id == FiatCAN::ID_ENGINE_DATA) {
                    shared_data.engine = FiatCAN::parseEngineData(payload, dlc);
                } 
                else if (id == FiatCAN::ID_SPEED) {
                    shared_data.speed_kmh = FiatCAN::parseSpeed(payload, dlc);
                }
                else if (id == FiatCAN::ID_FUEL_LEVEL) {
                    uint8_t current_fuel = FiatCAN::parseFuelLevel(payload, dlc);
                    if (current_fuel <= 100) { 
                        shared_data.fuel_level = current_fuel;
                    }
                } else if (id == FiatCAN::ID_CLUSTER_INFO) {
                    shared_data.autonomy_km = FiatCAN::parseAutonomy(payload, dlc);
                }
                xSemaphoreGive(dataMutex);
            }
        }

        TickType_t current_time = xTaskGetTickCount();
        TickType_t dt_ticks = current_time - last_calc_time;
        
        if (dt_ticks > 0) {
            uint32_t dt_ms = dt_ticks * portTICK_PERIOD_MS;
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
                shared_data.trip.update(dt_ms, shared_data.speed_kmh, shared_data.engine.consumption_lh, shared_data.fuel_level);
                xSemaphoreGive(dataMutex);
            }
            last_calc_time = current_time;
        }

        uint32_t current_missed = can.getMissedMessagesCount();
        if (current_missed != last_missed_count) {
            last_missed_count = current_missed;
        }
        
        // Yield to the scheduler to prevent Task Watchdog triggers on Core 0
        vTaskDelay(1);
    }
}

/**
 * @brief Core 1 Task: Visual Engine and Non-blocking Shutdown Evaluation.
 */
void task_gui_core1(void *pvParameters) {
    screen.begin();
    if (show_eeprom_warning) {
        my_data.settings = screen.getSettings();
        storage.saveData(my_data);
        initial_data = my_data;
        screen.showEepromWarning();
    } else {
        screen.applySettings(my_data.settings);
    }
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(40);

    SharedData local_snapshot;

    bool evaluating_shutdown = false;
    TickType_t shutdown_eval_start = 0;

    ButtonHandler buttonHandler(screen);
    buttonHandler.begin();

    uint16_t displayed_autonomy = 0xFFFF;
    TickType_t last_autonomy_update = 0;
    const TickType_t AUTONOMY_STEP_DELAY = pdMS_TO_TICKS(1500);
#if !ENABLE_USB_SNIFFER && ENABLE_PERFORMANCE_LOGGING
    int frame_counter = 0;
#endif
    while (1) {
#if !ENABLE_USB_SNIFFER && ENABLE_PERFORMANCE_LOGGING
        // When USB sniffer is enabled, the GUI task is not needed. Yield to avoid
        int64_t t_start = esp_timer_get_time();
#endif
        // Snapshot the shared data to minimize mutex hold time and prevent blocking the CAN core
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            local_snapshot = shared_data;
            xSemaphoreGive(dataMutex);
        }

        if (shutdown_flag) {
            if (!evaluating_shutdown) {
                evaluating_shutdown = true;
                shutdown_eval_start = xTaskGetTickCount();
            }

            // Debounce the ignition signal. If voltage returns, abort shutdown sequence
            if (gpio_get_level(GPIO_NUM_34) == 1) {
                shutdown_flag = false;
                evaluating_shutdown = false;
            } else {
                // Require a stable 2-second power loss before committing to EEPROM write to avoid corruption during voltage spikes
                if ((xTaskGetTickCount() - shutdown_eval_start) >= pdMS_TO_TICKS(2000)) {
                    my_data.total_km = local_snapshot.trip.getTotalKm();
                    my_data.fractional_km = local_snapshot.trip.getFractionalKm();
                    my_data.trip_km = local_snapshot.trip.getTripKm();
                    my_data.trip_time = local_snapshot.trip.getTripTimeSec();
                    my_data.trip_fuel_consumed = local_snapshot.trip.getTripFuelConsumed();
                    my_data.recent_avg_l_100km = local_snapshot.trip.getRecentAvg();
                    my_data.settings = screen.getSettings();
                    if (memcmp(&my_data, &initial_data, sizeof(my_data)) != 0) {
                        bool save_ok = storage.saveData(my_data);
                        screen.setShutdownState(true, save_ok);
                    } else {
                        screen.setShutdownState(true, true);
                    }
                    screen.render();
                    // Allow the TFT enough time to refresh and show the saving status before deep sleep
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    screen.sleep();
                    esp_deep_sleep_start();
                }
            }
        }

        buttonHandler.update();

        if (local_snapshot.autonomy_km == 0xFFFF) {
            displayed_autonomy = 0xFFFF; 
        } else if (displayed_autonomy == 0xFFFF) {
            displayed_autonomy = local_snapshot.autonomy_km;
        } else if (displayed_autonomy != local_snapshot.autonomy_km) {
            int diff = abs((int)local_snapshot.autonomy_km - (int)displayed_autonomy);
            if (diff > 40) {
                displayed_autonomy = local_snapshot.autonomy_km;
            } 
            else {
                TickType_t now = xTaskGetTickCount();
                if ((now - last_autonomy_update) >= AUTONOMY_STEP_DELAY) {
                    if (displayed_autonomy < local_snapshot.autonomy_km) {
                        displayed_autonomy++;
                    } else {
                        displayed_autonomy--;
                    }
                    last_autonomy_update = now;
                }
            }
        }

        screen.updateKM(local_snapshot.trip.getTotalKm());
        screen.updateEngine(local_snapshot.engine);
        screen.updateSpeed(local_snapshot.speed_kmh);
        screen.updateAutonomy(displayed_autonomy);
        
        screen.updateTripData(
            local_snapshot.trip.getTripKm(), 
            local_snapshot.trip.getTripAvgL100km(), 
            local_snapshot.trip.getTripAvgKmh(), 
            local_snapshot.trip.getTripTimeSec()
        );
        
        screen.updateFuel(local_snapshot.fuel_level);
        screen.render();

#if !ENABLE_USB_SNIFFER && ENABLE_PERFORMANCE_LOGGING
        int64_t t_end = esp_timer_get_time();

        int elapsed_ms = (t_end - t_start) / 1000;

        frame_counter++;
        if (frame_counter >= 25) {
            ESP_LOGI("GUI_PERF", "Render time: %d ms", elapsed_ms);
            frame_counter = 0;
        }
#endif

        // Enforce a strict ~25 FPS frame rate (40ms) to ensure smooth animations and feed the task watchdog
        if ((xTaskGetTickCount() - xLastWakeTime) < xFrequency) {
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        } else {
            vTaskDelay(1);
            xLastWakeTime = xTaskGetTickCount();
        }
    }
}

/**
 * @brief Application entry point. Bootstraps hardware, synchronization primitives, and FreeRTOS tasks.
 */
extern "C" void app_main(void) {
    // Disable stdout buffering. Crucial for real-time serial output when ENABLE_USB_SNIFFER is active
    setvbuf(stdout, NULL, _IONBF, 0);

    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
#if !ENABLE_USB_SNIFFER
        printf("Error: Failed to create Mutex\n");
#endif
        return;
    }

    // Configure ignition sensing on GPIO 34 (Active-low on power loss)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE; 
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_34);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_34, ignition_isr_handler, NULL);
    
    // Ensure the processor wakes up when the ignition is turned back on
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 1);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (storage.begin()) {
        rtc.begin(storage.getBusHandle());
        rtc.syncSystemTime();
        
        if (storage.loadData(my_data)) {
            initial_data = my_data;
            shared_data.trip.init(my_data.total_km, my_data.fractional_km, my_data.trip_km, my_data.trip_time, my_data.trip_fuel_consumed, my_data.recent_avg_l_100km);
        } else {
            // First boot or EEPROM corruption: initialize with safe defaults and show warning UI
            my_data.magic = storage.get_magic_number();
            my_data.total_km = 402373;
            my_data.fractional_km = 0.0f;
            my_data.trip_km = 0.0f;
            my_data.trip_time = 0;
            my_data.trip_fuel_consumed = 0.0f;
            my_data.recent_avg_l_100km = 0.0f;

            initial_data = my_data;
            shared_data.trip.init(402373, 0.0f, 0.0f, 0, 0.0f, 0.0f);
            show_eeprom_warning = true;
        }
    }

    screen.setOnTripResetCallback([]() {
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            shared_data.trip.resetTrip();
            xSemaphoreGive(dataMutex);
        }

        my_data.trip_km = 0.0f;
        my_data.trip_time = 0;
        my_data.trip_fuel_consumed = 0.0f;

        // Immediate EEPROM write ensures trip reset isn't lost if power is cut shortly after
        if (storage.saveData(my_data)) {
            initial_data = my_data; 
        } 
    });

    screen.setOnClockSetCallback([](uint8_t hour, uint8_t minute) {
        rtc.setTime(hour, minute);
    });

    // CAN bus is critical. If it fails to initialize, reboot the system to attempt recovery
    if (!can.begin()) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }

    // Assign tasks to specific cores. 
    // Core 0 handles real-time CAN and Wi-Fi/BT stacks (if used later).
    // Core 1 handles the heavy lifting of the TFT display to prevent interrupting CAN frames.
    xTaskCreatePinnedToCore(task_can_core0, "Task_CAN", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_gui_core1, "Task_GUI", 8192, NULL, 2, NULL, 1);
}