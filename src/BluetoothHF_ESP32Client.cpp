/**************************************************************************
 * @file BluetoothHF_ESP32Client.cpp
 * @brief An ESP32 implementation of Bluetooth Hands-Free Client interface
 **************************************************************************/

#include "BluetoothHF_ESP32Client.hpp"

#include "esp_err.h"
#include "esp_check.h"

#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_hf_client_api.h"

#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "nvs.h"
#include "nvs_flash.h"

#define RETURN_ON_ESP_ERROR(espError, returnVal, ...) ESP_LOGE(__VA_ARGS__); if (espError != ESP_OK) { return returnVal; }  

using namespace BluetoothHF;

ESP32Client::ESP32Client() : isServiceInitialized(false), isConnected(false) {
    // I should probably start the worker thread and job queue here,
    // so their handles are initialized immediately.
    workerJobQueue = xQueueCreate(10, sizeof(JobType));
    xTaskCreate(WorkerThreadJobHandler, "BluetoothHF_ESP32Worker", 4 * 1024, nullptr, configMAX_PRIORITIES - 3, &workerThread);
}

ESP32Client::~ESP32Client() {

    if (workerThread) {
        vTaskDelete(workerThread);
    }

    if (workerJobQueue) {
        vQueueDelete(workerJobQueue);
    }
}

void initializeBluetoothSecurity() {

// Set the security parameters for Bluetooth pairing
#if defined(CONFIG_BT_SSP_ENABLED)
    esp_bt_sp_param_t securityParam = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t inputOutputCapability = ESP_BT_IO_CAP_NONE; // NoInputNoOutput - suitable for headless devices
    esp_bt_gap_set_security_param(securityParam, &inputOutputCapability, sizeof(esp_bt_sp_param_t));
#endif

    // Set a fixed PIN code "0000" for pairing. Although, with NoInputNoOutput, 
    // this may not be requested from the Audio Gateway (AG).
    esp_bt_pin_type_t pinType = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pinCode;
    pinCode[0] = '0';
    pinCode[1] = '0';
    pinCode[2] = '0';
    pinCode[3] = '0';
    esp_bt_gap_set_pin(pinType, 4, pinCode);
}

ClientErrorCode ESP32Client::StartCoreService(const char* deviceName) {

    /**
     * We'll now initialize the various components of the Bluetooth stack 
     * required for the Hands-Free Client functionality.
     * See below for a simplified architecture diagram:
     * 
     * ESP-IDF Bluetooth Protocol Stack Architecture
     * ================================================
     *
     *        ┌─────────────────────────────────┐
     *        │   BluetoothHF::ESP32Client      │
     *        └──────────────┬──────────────────┘
     *                       │
     *        ┌──────────────▼──────────────────┐
     *        │  Hands-Free Profile (HFP)       │
     *        │    Client Implementation        │
     *        └──────────────┬──────────────────┘
     *                       │
     *        ┌──────────────▼──────────────────┐
     *        │       ESP-BlueDroid Stack       │
     *        │   ┌──────────────────────────┐  │
     *        │   │  General Access Profile  │  │
     *        │   └──────────────────────────┘  │
     *        └──────────────┬──────────────────┘
     *                       │
     *        ┌──────────────▼──────────────────┐
     *        │  Bluetooth Controller (Hardware)│
     *        │  - Link Manager                 │
     *        │  - Baseband                     │
     *        │  - RF Transceiver               │
     *        └─────────────────────────────────┘
     */

    if (IsCoreServiceActive())
        return ClientErrorCode::ERROR_OK;

    esp_err_t espErrorCode = ESP_OK;

    // Initialize the NVS partition in flash storage. 
    // NVS is used by the Bluetooth stack to store pairing information and other settings.
    espErrorCode = nvs_flash_init();
    if (espErrorCode == ESP_ERR_NVS_NO_FREE_PAGES) {
        
        espErrorCode = nvs_flash_erase();
        RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
            LOG_TAG_CLIENT, "%s - NVS flash erase failed: %s", __func__, esp_err_to_name(espErrorCode));


        espErrorCode = nvs_flash_init();
        RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
            LOG_TAG_CLIENT, "%s - NVS flash initialization failed after erase: %s", __func__, esp_err_to_name(espErrorCode));
    }

    // We are using Bluetooth Classic only, so we can release the BLE memory
    espErrorCode = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
        LOG_TAG_CLIENT, "%s - Bluetooth controller memory release failed: %s", __func__, esp_err_to_name(espErrorCode));

    // Initialize the bluetooth controller and enable it, using the default configuration
    esp_bt_controller_config_t bluetoothConfig = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    espErrorCode = esp_bt_controller_init(&bluetoothConfig);
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
        LOG_TAG_CLIENT, "%s - Bluetooth controller initialization failed: %s", __func__, esp_err_to_name(espErrorCode));

    espErrorCode = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
        LOG_TAG_CLIENT, "%s - Bluetooth controller enable failed: %s", __func__, esp_err_to_name(espErrorCode));

    // This is a modified version of the native Android Bluetooth Stack, BlueDroid. It's a middleware
    // between the Bluetooth controller and applications (like us). We need to initialize and enable it.
    esp_bluedroid_config_t BlueDroidConfig = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    espErrorCode = esp_bluedroid_init_with_cfg(&BlueDroidConfig);
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
        LOG_TAG_CLIENT, "%s - BlueDroid initialization failed: %s", __func__, esp_err_to_name(espErrorCode));

    espErrorCode = esp_bluedroid_enable();
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
        LOG_TAG_CLIENT, "%s - BlueDroid enable failed: %s", __func__, esp_err_to_name(espErrorCode));

    // Set the Bluetooth device name
    esp_bt_gap_set_device_name(deviceName);
  
    // Register the GAP and HFP client event handlers
    esp_bt_gap_register_callback(GAPEventHandler);
    esp_hf_client_register_callback(HFEventHandler);

    // Initialize the Hands-Free Profile (HFP) client
    espErrorCode = esp_hf_client_init();
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_SERVICE_INIT_FAILED, 
        LOG_TAG_CLIENT, "%s - HFP client initialization failed: %s", __func__, esp_err_to_name(espErrorCode));

    // If we made it this far, we've passed the major initialization steps (the things most likely to fail).
    // We can now mark the service as initialized. From here on, errors are less likely, but still possible.
    // We won't be reporting them.
    this->isServiceInitialized = true;

    // Initialize the PBAC (Phone Book Access ESP32) service if needed in the future
    // esp_pbac_register_callback(nullptr);
    // esp_pbac_init();

    initializeBluetoothSecurity();

    return ClientErrorCode::ERROR_OK;
}

ClientErrorCode ESP32Client::StartDiscovery() {

    if (!IsCoreServiceActive())
    {
        ESP_LOGE(LOG_TAG_CLIENT, "%s - Cannot start discovery, service not initialized.", __func__);
        return ClientErrorCode::ERROR_SERVICE_NOT_ACTIVE;
    }

    // Set the device to be connectable and discoverable
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    // Start device discovery with general inquiry mode, inquiry length of 10 seconds, and unlimited responses.
    // The parameter can be adjusted to ESP_BT_INQ_MODE_LIMITED_INQUIRY, which is supposed to search for a limited period,
    // but then again we are already specifying the inquiry length. So, I'm not sure what the exact difference is in practice.
    esp_err_t espErrorCode = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    if (espErrorCode == ESP_OK) {
        ESP_LOGI(LOG_TAG_CLIENT, "%s - Device discovery started successfully.", __func__);
        return ClientErrorCode::ERROR_OK;
    }
    else {
        ESP_LOGE(LOG_TAG_CLIENT, "%s - Failed to start device discovery: %s", __func__, esp_err_to_name(espErrorCode));
        return ClientErrorCode::ERROR_DISCOVERY_FAILED;
    }
}

ClientErrorCode ESP32Client::GetBluetoothAddress(char addressStr[18]) {
    
    if (!IsCoreServiceActive())
        return ClientErrorCode::ERROR_SERVICE_NOT_ACTIVE;

    const uint8_t* numAddress = esp_bt_dev_get_address();
    if (numAddress == nullptr)
        return ClientErrorCode::ERROR_UNKNOWN;

    sprintf(addressStr, "%02x:%02x:%02x:%02x:%02x:%02x", numAddress[0], numAddress[1], numAddress[2], numAddress[3], numAddress[4], numAddress[5]);
    return ClientErrorCode::ERROR_OK;
}

ClientErrorCode ESP32Client::Connect() {
    // Implementation for connecting to a Bluetooth Hands-Free device
    return ClientErrorCode::ERROR_OK;
}

ClientErrorCode ESP32Client::Disconnect() {
    // Implementation for disconnecting from a Bluetooth Hands-Free device
    return ClientErrorCode::ERROR_OK;
}

bool ESP32Client::IsConnected() {
    return isConnected;
}

ClientErrorCode ESP32Client::AnswerCall() {
    // Implementation to answer an incoming call
    return ClientErrorCode::ERROR_OK;
}

ClientErrorCode ESP32Client::EndCall() {
    // Implementation to end the current call
    return ClientErrorCode::ERROR_OK;
}

ClientErrorCode ESP32Client::DialNumber(const char* number) {
    if (!IsConnected()) {
        ESP_LOGE(LOG_TAG_CLIENT, "%s - Cannot dial number, not connected to any device.", __func__);
        return ClientErrorCode::ERROR_DISCONNECTED;
    }

    // I'm thinking the work below should be queued as a job in the worker thread.
    // Moreover, we should wait until the job is done and the result is known before returning from this function.
    esp_err_t espErrorCode = esp_hf_client_dial(number);
    RETURN_ON_ESP_ERROR(espErrorCode, ClientErrorCode::ERROR_DIAL_FAILED, 
        LOG_TAG_CLIENT, "%s - Failed to dial number %s: %s", __func__, number, esp_err_to_name(espErrorCode));

    return ClientErrorCode::ERROR_OK;
}

void ESP32Client::GAPEventHandler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    ESP32Client& clientInstance = ESP32Client::GetInstance();
    if (!clientInstance.IsCoreServiceActive()) {
        ESP_LOGE(LOG_TAG_CLIENT, "%s - GAP event received but service not initialized.", __func__);
    }

    switch(event)
    {
    // Connection established with an Audio Gateway (AG) device
    case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        // Needs to be made thread-safe
        // clientInstance.isConnected = true;
        ESP_LOGI(LOG_TAG_CLIENT, "%s - Connection established.", __func__);
        break;
    // Connection broken with an Audio Gateway (AG) device
    case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        // Needs to be made thread-safe
        // clientInstance.isConnected = false;
        ESP_LOGI(LOG_TAG_CLIENT, "%s - Remove bonded device completed.", __func__);
        break;
    default:
        ESP_LOGE(LOG_TAG_CLIENT, "%s - Unhandled GAP event: %d", __func__, event);
    }
}


void ESP32Client::HFEventHandler(esp_hf_client_cb_event_t event, esp_hf_client_cb_param_t *param)
{
    
}

void ESP32Client::WorkerThreadJobHandler(void* arg)
{
    ESP32Client& clientInstance = ESP32Client::GetInstance();
    QueueHandle_t jobQueue = clientInstance.workerJobQueue;

    ESP32Client::JobType receivedJob;

    while (true) {
        // Wait indefinitely for a job to be available in the queue. xQueueReceive is a blocking call.
        if (xQueueReceive(jobQueue, &receivedJob, portMAX_DELAY) == pdTRUE) {
            // Execute the job function with the provided parameter
            ESP32Client::JobFunctionType jobFunction = receivedJob.jobFunction;
            void* jobParam = receivedJob.jobParam;

            if (jobFunction != nullptr) {
                jobFunction(jobParam);
            }
        }
    }
}

ClientErrorCode ESP32Client::SubscribeToEvents(IClientEventSubscriber& subscriber)
{
    return ClientErrorCode::ERROR_OK;
}