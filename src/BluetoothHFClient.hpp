#ifndef BLUETOOTH_HF_CLIENT_HPP
#define BLUETOOTH_HF_CLIENT_HPP

#include "esp_err.h"

/**********************************************************************
 * @file BluetoothHFClient.hpp
 * @brief Header file for Bluetooth Hands-Free Client static class.
 **********************************************************************/

namespace BluetoothHF {

    class Client {
    public:
        static Client& GetInstance() {
            static Client instance;
            return instance;
        }
        /// @brief Initializes the various services required for the Bluetooth Hands-Free Client.
        /// @return esp_err_t - ESP_OK if successful, error code otherwise. If an error occurs, it is rather fatal for the Bluetooth functionality.
        esp_err_t StartCoreService();
        bool IsCoreServiceActive() { return isServiceInitialized; }

        esp_err_t Connect();
        esp_err_t Disconnect();
        bool IsConnected();
        esp_err_t AnswerCall();
        esp_err_t EndCall();
        esp_err_t DialNumber(const char* number);

        // Returns an 18-character string representation of the Bluetooth address (MAC).
        // If the client is not initialized, returns nullptr.
        void GetBluetoothAddress(char addressStr[18]);
        
    private:
        Client();
        ~Client() {};

        bool isServiceInitialized;
        static constexpr const char* LOG_TAG_CLIENT = "BluetoothHFClient";

    };

}
#endif // BLUETOOTH_HF_CLIENT_HPP