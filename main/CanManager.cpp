#include "CanManager.h"
#include "esp_log.h"

static const char* TAG = "CanManager";

CanManager::CanManager(gpio_num_t txPin, gpio_num_t rxPin) 
    : _txPin(txPin), _rxPin(rxPin), _isInitialized(false), 
      _node(nullptr), _rxQueue(nullptr), _missedMessagesQueue(0) {}

CanManager::~CanManager() {
    stop();
}

/**
 * @brief ISR callback triggered upon successful CAN frame reception.
 * 
 * Reads the frame from the TWAI driver and defers processing by 
 * pushing it to the FreeRTOS receive queue.
 * 
 * @param handle TWAI node handle.
 * @param edata Event data (unused).
 * @param user_ctx Pointer to the CanManager instance.
 * @return true if a high priority task was woken, false otherwise.
 */
bool CanManager::onRxDoneCallback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
    CanManager* manager = static_cast<CanManager*>(user_ctx);

    CanFrameWrapper wrapper = {};
    wrapper.frame.buffer = wrapper.payload;
    wrapper.frame.buffer_len = sizeof(wrapper.payload);

    if (twai_node_receive_from_isr(handle, &wrapper.frame) == ESP_OK) {
        BaseType_t high_priority_task_woken = pdFALSE;
        
        if (xQueueSendFromISR(manager->_rxQueue, &wrapper, &high_priority_task_woken) != pdTRUE) {
            manager->_missedMessagesQueue++; 
        }
        return high_priority_task_woken == pdTRUE;
    }
    return false;
}

/**
 * @brief Initializes the TWAI node in listen-only mode.
 * 
 * @return true if initialization was successful or if already initialized.
 */
bool CanManager::begin() {
    if (_isInitialized) return true;

    _rxQueue = xQueueCreate(256, sizeof(CanFrameWrapper));
    if (!_rxQueue) {
        // ESP_LOGE(TAG, "Failed to create RX queue");
        return false;
    }

    twai_onchip_node_config_t node_config = {};
    node_config.io_cfg.tx = _txPin;
    node_config.io_cfg.rx = _rxPin;
    node_config.bit_timing.bitrate = 50000; 
    node_config.flags.enable_listen_only = true; 

    if (twai_new_node_onchip(&node_config, &_node) != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to create TWAI on-chip node");
        vQueueDelete(_rxQueue);
        return false;
    }

    twai_event_callbacks_t cbs = {};
    cbs.on_rx_done = onRxDoneCallback;

    if (twai_node_register_event_callbacks(_node, &cbs, this) != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to register TWAI callbacks");
        twai_node_delete(_node);
        vQueueDelete(_rxQueue);
        return false;
    }

    if (twai_node_enable(_node) != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to start TWAI node");
        twai_node_delete(_node);
        vQueueDelete(_rxQueue);
        return false;
    }

    _isInitialized = true;
    // ESP_LOGI(TAG, "TWAI node initialized (50 kbps, Listen-Only mode)");
    return true;
}

/**
 * @brief Stops the TWAI node and frees allocated resources.
 */
void CanManager::stop() {
    if (!_isInitialized) return;

    twai_node_disable(_node);
    twai_node_delete(_node);

    if (_rxQueue) {
        vQueueDelete(_rxQueue);
        _rxQueue = nullptr;
    }

    _isInitialized = false;
    // ESP_LOGI(TAG, "TWAI node stopped and deleted");
}

/**
 * @brief Blocks and waits to receive a message from the queue.
 * 
 * @param msg Reference to the wrapper where the message will be stored.
 * @param timeoutMs Maximum time to wait for a message in milliseconds.
 * @return true if a message was successfully received within the timeout.
 */
bool CanManager::receiveMessage(CanFrameWrapper& msg, uint32_t timeoutMs) {
    if (!_isInitialized) return false;
    
    return (xQueueReceive(_rxQueue, &msg, pdMS_TO_TICKS(timeoutMs)) == pdTRUE);
}

/**
 * @brief Retrieves the count of frames dropped due to queue overflow.
 * 
 * @return Number of dropped messages.
 */
uint32_t CanManager::getMissedMessagesCount() {
    if (!_isInitialized) return 0;
    
    return _missedMessagesQueue;
}