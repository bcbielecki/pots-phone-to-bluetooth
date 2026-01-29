#ifndef BLUETOOTH_HF_CLIENT_HPP
#define BLUETOOTH_HF_CLIENT_HPP

/**********************************************************************
 * @file BluetoothHFClient.hpp
 * @brief Header file for Bluetooth Hands-Free Client static class.
 **********************************************************************/

#include "esp_err.h"

#include "esp_gap_bt_api.h"
#include "esp_hf_client_api.h"

#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace BluetoothHF {

    /// @brief Bluetooth Hands-Free Client class.
    /// Security settings are currently hardcoded to NoInputNoOutput and PIN "0000".
    /// Those settings could be exposed in the future if needed.
    class Client {

    public:

        /// @brief Gets the singleton instance of the Bluetooth Hands-Free Client
        /// @return  Client& - R=reference to the singleton Client instance
        static Client& GetInstance() {
            static Client instance;
            return instance;
        }

        /// @brief Initializes the various services required for the Bluetooth Hands-Free Client
        /// @param deviceName - The Bluetooth device name to set for the client. It will be visible to other devices.
        /// @return esp_err_t - ESP_OK if successful, error code otherwise. If an error occurs, it is rather fatal for the Bluetooth functionality.
        esp_err_t StartCoreService(const char* deviceName);

        /// @brief Checks if the core Bluetooth Hands-Free service is active
        /// @return bool - true if the service is active (after calling StartCoreService() with success), false otherwise.
        bool IsCoreServiceActive() { return isServiceInitialized; }

        /// @brief Starts device discovery to find nearby Bluetooth devices.
        /// This is not expected to fail unless the core service is not started.
        esp_err_t StartDiscovery();

        esp_err_t Connect();
        esp_err_t Disconnect();
        bool IsConnected();
        esp_err_t AnswerCall();
        esp_err_t EndCall();
        esp_err_t DialNumber(const char* number);

        /// @brief Retrieves the Bluetooth address (MAC) of the device.
        /// @param addressStr - output parameter to hold the Bluetooth address as a string. 
        /// If the client is not initialized, returns nullptr.
        void GetBluetoothAddress(char addressStr[18]);

    private:

        /// @brief This handles jobs added to the queue by GAPEventHandler and HFEventHandler.
        /// It is registered when creating the worker thread in the constructor.
        static void WorkerThreadJobHandler(void* arg);

        /// @brief Handles General Access Profile (GAP) events.
        /// Should be registered as a callback with esp_bt_gap_register_callback in StartCoreService.
        /// When it receives an event, the corresponding job will be scheduled with the worker thread.
        /// @param event - the GAP event type
        /// @param param - parameters associated with the event
        static void GAPEventHandler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

        /// @brief Handles Hands-Free Profile (HFP) client events.
        /// Should be registered as a callback with esp_hf_client_register_callback in StartCoreService.
        /// @param event - the Hands-Free Client event type
        /// @param param - parameters associated with the event
        static void HFEventHandler(esp_hf_client_cb_event_t event, esp_hf_client_cb_param_t *param);

        using JobFunctionType = void(*)(void*);

        struct JobType {
            JobFunctionType jobFunction;
            void* jobParam;
        };

        TaskHandle_t workerThread;
        QueueHandle_t workerJobQueue;

        bool isServiceInitialized;
        bool isConnected;
        static constexpr const char* LOG_TAG_CLIENT = "BluetoothHFClient";

        Client();
        ~Client();
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
    };

}
#endif // BLUETOOTH_HF_CLIENT_HPP   