#include "BluetoothHFClient.hpp"

#include "esp_err.h"
#include "esp_check.h"

#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_hf_client_api.h"

#include "nvs.h"
#include "nvs_flash.h"

using namespace BluetoothHF;



Client::Client() : isServiceInitialized(false) {
    // Constructor implementation (if needed)
}

/// @brief Initializes the various services required for the Bluetooth Hands-Free Client.
/// @return esp_err_t - ESP_OK if successful, error code otherwise. If an error occurs, it is rather fatal for the Bluetooth functionality.
esp_err_t Client::InitializeService() {

    if (this->isServiceInitialized) {
        return ESP_OK; // Already initialized
    }

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

    this->isServiceInitialized = true;

    char bluetoothAddressStr[18] {0};
    GetBluetoothAddress(bluetoothAddressStr);
    ESP_LOGI(LOG_TAG_CLIENT, "Bluetooth address (MAC): %s", bluetoothAddressStr);

    esp_bt_gap_set_device_name("Ben_BT_Device");

    return ESP_OK;
}

void Client::GetBluetoothAddress(char addressStr[18]) {
    if (!this->isServiceInitialized) {
        return; // Service not initialized
    }

    const uint8_t* numAddress = esp_bt_dev_get_address();

    if (numAddress == nullptr) {
        return;
    }

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
    // Implementation to dial a number
    return ESP_OK;
}