#ifndef BLUETOOTH_HF_ESP32CLIENT_HPP
#define BLUETOOTH_HF_ESP32CLIENT_HPP

#include "BluetoothHF_IClient.hpp"

/**********************************************************************
 * @file BluetoothHF_ESP32Client.hpp
 * @brief Header file for Bluetooth Hands-Free Client static class. It is expected that 
 * the ESP32 will use one Bluetooth device, so a singleton pattern is applied.
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
    class ESP32Client : public BluetoothHF::IClient {

    public:

        /// @brief Gets the singleton instance of the Bluetooth Hands-Free Client ESP32 implementation
        /// @return ESP32Client& - reference to the singleton client instance
        static ESP32Client& GetInstance() {
            static ESP32Client instance;
            return instance;
        }

        ClientErrorCode StartCoreService(const char* deviceName) override;

        bool IsCoreServiceActive() override { return isServiceInitialized; }

        ClientErrorCode SubscribeToEvents(IClientEventSubscriber& subscriber) override;

        ClientErrorCode StartDiscovery() override;

        ClientErrorCode Connect() override;
        ClientErrorCode Disconnect() override;
        bool IsConnected() override;
        ClientErrorCode AnswerCall() override;
        ClientErrorCode EndCall() override;
        ClientErrorCode DialNumber(const char* number) override;
        ClientErrorCode GetBluetoothAddress(char addressStr[18]) override;

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
        static constexpr const char* LOG_TAG_CLIENT = "BluetoothHF_ESP32Client";

        ESP32Client();
        ~ESP32Client() override;
        ESP32Client(const ESP32Client&) = delete;
        ESP32Client& operator=(const ESP32Client&) = delete;

        void NotifySubscibersOfEvent();
    };
}

#endif // BLUETOOTH_HF_ESP32CLIENT_HPP   