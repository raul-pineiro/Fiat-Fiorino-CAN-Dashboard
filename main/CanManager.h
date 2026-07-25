#pragma once

#include "hal/gpio_types.h"
#include "esp_twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_twai_onchip.h"

/**
 * @brief Wrapper structure to store the TWAI frame and its payload in the FreeRTOS queue.
 */
struct CanFrameWrapper {
    twai_frame_t frame;
    uint8_t payload[8]; // Note: Increase to 64 bytes if migrating to CAN FD.
};

/**
 * @brief Manages TWAI (CAN bus) communication using the ESP-IDF API.
 */
class CanManager {
public:
    CanManager(gpio_num_t txPin, gpio_num_t rxPin);
    ~CanManager();

    /**
     * @brief Initializes the TWAI node in listen-only mode.
     * @return true if initialization was successful.
     */
    bool begin();

    /**
     * @brief Stops the TWAI node and frees allocated resources.
     */
    void stop();

    /**
     * @brief Blocks and waits to receive a message from the queue.
     * 
     * @param msg Reference to the wrapper where the message will be stored.
     * @param timeoutMs Maximum time to wait in milliseconds.
     * @return true if a message was successfully received.
     */
    bool receiveMessage(CanFrameWrapper& msg, uint32_t timeoutMs);

    /**
     * @brief Retrieves the count of frames dropped due to queue overflow.
     * @return Number of dropped messages.
     */
    uint32_t getMissedMessagesCount();

private:
    gpio_num_t _txPin;
    gpio_num_t _rxPin;
    bool _isInitialized;
    
    twai_node_handle_t _node;
    QueueHandle_t _rxQueue;
    uint32_t _missedMessagesQueue;

    /**
     * @brief ISR callback triggered upon successful CAN frame reception.
     */
    static bool onRxDoneCallback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx);
};