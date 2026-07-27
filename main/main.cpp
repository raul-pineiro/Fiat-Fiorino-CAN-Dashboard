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

/**
 * @def ENABLE_USB_SNIFFER
 * @brief Toggles the SLCAN compatible USB sniffer output for development.
 */
#define ENABLE_USB_SNIFFER 1

ScreenHandler screen;
CanManager can(GPIO_NUM_26, GPIO_NUM_25);
StorageManager storage(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
RtcManager rtc;

SemaphoreHandle_t dataMutex;

DashboardData my_data;
DashboardData initial_data;
volatile bool shutdown_flag = false; 

/**
 * @brief Thread-safe container for telemetry data shared between cores.
 */
struct SharedData {
    TripComputer trip;
    
    FiatCAN::EngineData engine = {};
    uint16_t speed_kmh = 0;
    uint8_t fuel_level = 0;
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
                
                /* TODO: Reverse engineer and implement parsers for the following missing CAN IDs:
                   - Vehicle Speed (currently mocked/missing)
                   - Fuel Level (raw sensor data)*/
                /*
                else if (id == ID_SPEED) {
                    shared_data.speed_kmh = (payload[2] << 8) | payload[3]; 
                }
                else if (id == ID_FUEL_LEVEL) {
                    shared_data.fuel_level = payload[0]; 
                }
                */
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
    TickType_t last_cycle_time = xTaskGetTickCount();
    const TickType_t cycle_interval = pdMS_TO_TICKS(3000);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(40);

    SharedData local_snapshot;

    bool evaluating_shutdown = false;
    TickType_t shutdown_eval_start = 0;

    while (1) {
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
                // Require a stable 5-second power loss before committing to EEPROM write to avoid corruption during voltage spikes
                if ((xTaskGetTickCount() - shutdown_eval_start) >= pdMS_TO_TICKS(5000)) {
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
                    esp_deep_sleep_start();
                }
            }
        }

        /* TODO: Implement hardware button polling/interrupts (Trip, Menu, Plus, Minus).
           Once physical GPIO buttons are wired and debounced, remove this automated 
           navigation block and trigger the screen.handleButtonX() functions via ISR or polling.*/
        
        // --- TEMPORAL MOCKED UI NAVIGATION ---
        if ((xTaskGetTickCount() - last_cycle_time) >= cycle_interval) {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                
                static int dashboard_step = 0;
                static int settings_step = 0;
                static bool in_settings = false;

                if (!in_settings) {
                    if (dashboard_step < static_cast<int>(DisplayPage::MAX_PAGES) - 1) {
                        screen.handleButtonTrip(); 
                        dashboard_step++;
                    } else {
                        screen.handleButtonMenu(); 
                        in_settings = true;
                        dashboard_step = 0;
                        settings_step = 0;
                    }
                } else {
                    switch (settings_step) {
                        case 0:
                            screen.handleButtonTrip(); 
                            settings_step++;
                            break;
                        case 1:
                            screen.handleButtonMenu(); 
                            settings_step++;
                            break;
                        case 2:
                        case 3:
                        case 4:
                            screen.handleButtonPlus(); 
                            settings_step++;
                            break;
                        case 5:
                            screen.handleButtonMenu(); 
                            in_settings = false;
                            settings_step = 0;
                            
                            screen.handleComboTripPlus();
                            break;
                        default:
                            settings_step = 0;
                            break;
                    }
                }

                xSemaphoreGive(dataMutex);
            }
            last_cycle_time = xTaskGetTickCount();
        }

        screen.updateKM(local_snapshot.trip.getTotalKm());
        screen.updateEngine(local_snapshot.engine);
        screen.updateSpeed(local_snapshot.speed_kmh);
        screen.updateAutonomy(local_snapshot.trip.getAutonomyKm());
        
        screen.updateTripData(
            local_snapshot.trip.getTripKm(), 
            local_snapshot.trip.getTripAvgL100km(), 
            local_snapshot.trip.getTripAvgKmh(), 
            local_snapshot.trip.getTripTimeSec()
        );
        
        screen.updateFuel(local_snapshot.fuel_level);
        screen.render();

        // Enforce a strict ~25 FPS frame rate (40ms) to ensure smooth animations and feed the task watchdog
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
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

    if (storage.begin()) {
        rtc.begin(storage.getBusHandle());
        rtc.syncSystemTime();
        
        if (storage.loadData(my_data)) {
            initial_data = my_data;
            shared_data.trip.init(my_data.total_km, my_data.fractional_km, my_data.trip_km, my_data.trip_time, my_data.trip_fuel_consumed, my_data.recent_avg_l_100km);
            screen.applySettings(my_data.settings);
        } else {
            // First boot or EEPROM corruption: initialize with safe defaults and show warning UI
            my_data.magic = 0xFA170001;
            my_data.total_km = 402000;
            my_data.fractional_km = 0.0f;
            my_data.trip_km = 0.0f;
            my_data.trip_time = 0;
            my_data.trip_fuel_consumed = 0.0f;
            my_data.recent_avg_l_100km = 0.0f;
            my_data.settings = screen.getSettings();
            
            storage.saveData(my_data); 
            initial_data = my_data;
            shared_data.trip.init(402000, 0.0f, 0.0f, 0, 0.0f, 0.0f);
            screen.showEepromWarning();
        }
    }

    screen.begin();

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