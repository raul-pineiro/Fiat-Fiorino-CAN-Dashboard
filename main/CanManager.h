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
    twai_frame_t frame;     /**< The TWAI frame containing ID, DLC, and metadata flags */
    uint8_t payload[8];     /**< Payload data buffer (Note: Increase to 64 bytes if migrating to CAN FD) */
};

/**
 * @brief Manages TWAI (CAN bus) communication using the ESP-IDF API.
 */
class CanManager {
public:
    /**
     * @brief Constructs the CanManager instance.
     * 
     * @param txPin GPIO pin number to be used for TWAI TX (Transmit).
     * @param rxPin GPIO pin number to be used for TWAI RX (Receive).
     */
    CanManager(gpio_num_t txPin, gpio_num_t rxPin);

    /**
     * @brief Destructor. Ensures the TWAI node is stopped and resources are freed.
     */
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
    gpio_num_t _txPin;                  /**< GPIO pin assigned for transmission */
    gpio_num_t _rxPin;                  /**< GPIO pin assigned for reception */
    bool _isInitialized;                /**< Flag indicating if the TWAI driver is currently running */
    
    twai_node_handle_t _node;           /**< Handle for the initialized TWAI node */
    QueueHandle_t _rxQueue;             /**< FreeRTOS queue to safely pass messages from ISR to tasks */
    uint32_t _missedMessagesQueue;      /**< Counter for messages lost because the RX queue was full */

    /**
     * @brief ISR callback triggered upon successful CAN frame reception.
     * 
     * @param handle The handle of the TWAI node that generated the event.
     * @param edata Pointer to the event data structure containing frame details.
     * @param user_ctx Pointer to the user context (typically the 'this' instance).
     * @return true if a higher priority task was woken up by the queue send, false otherwise.
     */
    static bool onRxDoneCallback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx);
};