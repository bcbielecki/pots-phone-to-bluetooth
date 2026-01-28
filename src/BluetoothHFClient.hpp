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
    class Client {
    public:

        /// @brief This handles jobs added to the queue by GAPEventHandler and HFEventHandler.
        /// It is registered when creating the worker thread in the constructor.
        static void WorkerThreadJobHandler(void* arg);

        /// @brief Gets the singleton instance of the Bluetooth Hands-Free Client
        /// @return  Client& - R=reference to the singleton Client instance
        static Client& GetInstance() {
            static Client instance;
            return instance;
        }

        /// @brief Initializes the various services required for the Bluetooth Hands-Free Client
        /// @return esp_err_t - ESP_OK if successful, error code otherwise. If an error occurs, it is rather fatal for the Bluetooth functionality.
        esp_err_t StartCoreService();

        /// @brief Checks if the core Bluetooth Hands-Free service is active
        /// @return bool - true if the service is active (after calling StartCoreService() with success), false otherwise.
        bool IsCoreServiceActive() { return isServiceInitialized; }

        /// @brief Handles General Access Profile (GAP) events.
        /// Should be registered as a callback with esp_bt_gap_register_callback in StartCoreService.
        /// When it receives an event, the corresponding job will be scheduled with the worker thread.
        /// @param event - the GAP event type
        /// @param param - parameters associated with the event
        void GAPEventHandler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

        /// @brief Handles Hands-Free Profile (HFP) client events.
        /// Should be registered as a callback with esp_hf_client_register_callback in StartCoreService.
        /// @param event - the Hands-Free Client event type
        /// @param param - parameters associated with the event
        void HFEventHandler(esp_hf_client_cb_event_t event, esp_hf_client_cb_param_t *param);


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
        Client();
        ~Client();

        using JobFunctionType = void(*)(void*);

        struct JobType {
            JobFunctionType jobFunction;
            void* jobParam;
        };

        TaskHandle_t workerThread;
        QueueHandle_t workerJobQueue;

        bool isServiceInitialized;
        static constexpr const char* LOG_TAG_CLIENT = "BluetoothHFClient";

    };

}
#endif // BLUETOOTH_HF_CLIENT_HPP