/**********************************************************************
 * @file BluetoothHFClient.cpp
 * @brief Implementation of Bluetooth Hands-Free Client static class.
 **********************************************************************/

#include "BluetoothHFClient.hpp"

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

using namespace BluetoothHF;



Client::Client() : isServiceInitialized(false) {
    // I should probably start the worker thread and job queue here,
    // so their handles are initialized immediately.
    workerJobQueue = xQueueCreate(10, sizeof(JobType));
    xTaskCreate(WorkerThreadJobHandler, "BluetoothHFClientWorker", 4 * 1024, nullptr, configMAX_PRIORITIES - 3, &workerThread);
}

Client::~Client() {

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

esp_err_t Client::StartCoreService(const char* deviceName) {

    /**
     * We'll now initialize the various components of the Bluetooth stack 
     * required for the Hands-Free Client functionality.
     * See below for a simplified architecture diagram:
     * 
     * ESP-IDF Bluetooth Protocol Stack Architecture
     * ================================================
     *
     *        ┌─────────────────────────────────┐
     *        │   BluetoothHFClient Singleton   │
     *        └──────────────┬──────────────────┘
     *                       │
     *        ┌──────────────▼──────────────────┐
     *        │  Hands-Free Profile (HFP)       │
     *        │  Client Implementation          │
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
        return ESP_OK;

    esp_err_t errorCode = ESP_OK;

    // Initialize the NVS partition in flash storage. 
    // NVS is used by the Bluetooth stack to store pairing information and other settings.
    errorCode = nvs_flash_init();
    if (errorCode == ESP_ERR_NVS_NO_FREE_PAGES) {
        
        errorCode = nvs_flash_erase();
        ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - NVS flash erase failed: %s",
            __func__, esp_err_to_name(errorCode));

        errorCode = nvs_flash_init();
        ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - NVS flash initialization failed: %s",
            __func__, esp_err_to_name(errorCode));
    }

    // We are using Bluetooth Classic only, so we can release the BLE memory
    errorCode = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - Bluetooth controller memory release failed: %s",
         __func__, esp_err_to_name(errorCode));

    // Initialize the bluetooth controller and enable it, using the default configuration
    esp_bt_controller_config_t bluetoothConfig = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    errorCode = esp_bt_controller_init(&bluetoothConfig);
    ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - Bluetooth controller initialization failed: %s",
         __func__, esp_err_to_name(errorCode));

    errorCode = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - Bluetooth controller enable failed: %s",
         __func__, esp_err_to_name(errorCode));

    // This is a modified version of the native Android Bluetooth Stack, BlueDroid. It's a middleware
    // between the Bluetooth controller and applications (like us). We need to initialize and enable it.
    esp_bluedroid_config_t BlueDroidConfig = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    errorCode = esp_bluedroid_init_with_cfg(&BlueDroidConfig);
    ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - BlueDroid initialization failed: %s",
         __func__, esp_err_to_name(errorCode));

    errorCode = esp_bluedroid_enable();
    ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - BlueDroid enable failed: %s",
         __func__, esp_err_to_name(errorCode));

    // Set the Bluetooth device name
    esp_bt_gap_set_device_name(deviceName);
  
    // Register the GAP and HFP client event handlers
    esp_bt_gap_register_callback(GAPEventHandler);
    esp_hf_client_register_callback(HFEventHandler);

    // Initialize the Hands-Free Profile (HFP) client
    errorCode = esp_hf_client_init();
    ESP_RETURN_ON_ERROR(errorCode, LOG_TAG_CLIENT, "%s - Hands-Free Client initialization failed: %s",
         __func__, esp_err_to_name(errorCode));

    // If we made it this far, we've passed the major initialization steps (the things most likely to fail).
    // We can now mark the service as initialized. From here on, errors are less likely, but still possible.
    // We won't be reporting them.
    this->isServiceInitialized = true;

    // Initialize the PBAC (Phone Book Access Client) service if needed in the future
    // esp_pbac_register_callback(nullptr);
    // esp_pbac_init();

    initializeBluetoothSecurity();

    return ESP_OK;
}

esp_err_t Client::StartDiscovery() {

    if (IsCoreServiceActive())
    {
        // Set the device to be connectable and discoverable
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

        // Start device discovery with general inquiry mode, inquiry length of 10 seconds, and unlimited responses.
        // The parameter can be adjusted to ESP_BT_INQ_MODE_LIMITED_INQUIRY, which is supposed to search for a limited period,
        // but then again we are already specifying the inquiry length. So, I'm not sure what the exact difference is in practice.
        esp_err_t errorCode = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        if (errorCode == ESP_OK) {
            ESP_LOGI(LOG_TAG_CLIENT, "%s - Device discovery started successfully.", __func__);
        }
        else {
            ESP_LOGE(LOG_TAG_CLIENT, "%s - Failed to start device discovery: %s", __func__, esp_err_to_name(errorCode));
        }
        return errorCode;
    }
    else {
        ESP_LOGE(LOG_TAG_CLIENT, "%s - Cannot start discovery, service not initialized.", __func__);
        return ESP_ERR_INVALID_STATE;
    }
}

void Client::GAPEventHandler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{

}


void Client::HFEventHandler(esp_hf_client_cb_event_t event, esp_hf_client_cb_param_t *param)
{
    
}

void Client::WorkerThreadJobHandler(void* arg)
{
    Client& clientInstance = Client::GetInstance();
    QueueHandle_t jobQueue = clientInstance.workerJobQueue;

    Client::JobType receivedJob;

    while (true) {
        // Wait indefinitely for a job to be available in the queue. xQueueReceive is a blocking call.
        if (xQueueReceive(jobQueue, &receivedJob, portMAX_DELAY) == pdTRUE) {
            // Execute the job function with the provided parameter
            Client::JobFunctionType jobFunction = receivedJob.jobFunction;
            void* jobParam = receivedJob.jobParam;

            if (jobFunction != nullptr) {
                jobFunction(jobParam);
            }
        }
    }
}

void Client::GetBluetoothAddress(char addressStr[18]) {
    
    if (!IsCoreServiceActive())
        return;

    const uint8_t* numAddress = esp_bt_dev_get_address();
    if (numAddress == nullptr)
        return;

    sprintf(addressStr, "%02x:%02x:%02x:%02x:%02x:%02x", numAddress[0], numAddress[1], numAddress[2], numAddress[3], numAddress[4], numAddress[5]);
}

esp_err_t Client::Connect() {
    // Implementation for connecting to a Bluetooth Hands-Free device
    return ESP_OK;
}

esp_err_t Client::Disconnect() {
    // Implementation for disconnecting from a Bluetooth Hands-Free device
    return ESP_OK;
}

bool Client::IsConnected() {
    // Implementation to check if connected to a Bluetooth Hands-Free device
    return false;
}

esp_err_t Client::AnswerCall() {
    // Implementation to answer an incoming call
    return ESP_OK;
}

esp_err_t Client::EndCall() {
    // Implementation to end the current call
    return ESP_OK;
}

esp_err_t Client::DialNumber(const char* number) {
    return esp_hf_client_dial(number);
}