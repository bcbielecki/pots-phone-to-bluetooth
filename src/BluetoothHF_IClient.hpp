#ifndef BLUETOOTH_HF_ICLIENT_HPP
#define BLUETOOTH_HF_ICLIENT_HPP

/**********************************************************************
 * @file BluetoothHF_IClient.hpp
 * @brief Header file for Bluetooth Hands-Free Client interface.
 * This interface can be implemented for various platforms (e.g., ESP32, ATmega, etc.)
 * to provide Bluetooth Hands-Free Client functionality. 
 **********************************************************************/

namespace BluetoothHF {

    enum class ClientErrorCode {
        ERROR_OK = 0,
        ERROR_SERVICE_INIT_FAILED,
        ERROR_SERVICE_NOT_ACTIVE,
        ERROR_CONNECTION_FAILED,
        ERROR_DISCONNECTED,
        ERROR_DISCOVERY_FAILED,
        ERROR_DISCONNECTION_FAILED,
        ERROR_CALL_ANSWER_FAILED,
        ERROR_CALL_END_FAILED,
        ERROR_DIAL_FAILED,
        ERROR_UNKNOWN
    };

    enum class ClientEventType {
        CONNECTION_STATE_CHANGED = 0,
        INCOMING_CALL,
        CALL_ENDED,
        DIALING,
        AUDIO_STATE_CHANGED
    };

    class IClientEventSubscriber
    {
    public:
        virtual ~IClientEventSubscriber() = 0;
        virtual void OnNotifyClientEvent(ClientEventType event) = 0;

    };

    /// @brief Bluetooth Hands-Free Client interface.
    /// Implementations of this interface should enforce the requirement that
    /// only one instance of the client exists per platform, if applicable. As a secondary requirement,
    /// only one command should be processed at a time.
    class IClient {

    public:
        virtual ~IClient() = default;

        /// @brief Initializes the various services required for the Bluetooth Hands-Free Client
        /// @param deviceName - The Bluetooth device name to set for the client. It will be visible to other devices.
        /// @return ClientErrorCode - SUCCESS if successful, error code otherwise. If an error occurs, it is rather fatal for the Bluetooth functionality.
        virtual ClientErrorCode StartCoreService(const char* deviceName) = 0;

        /// @brief Checks if the core Bluetooth Hands-Free service is active
        /// @return bool - true if the service is active (after calling StartCoreService() with success), false otherwise.
        virtual bool IsCoreServiceActive() = 0;

        /// @brief Subscribes the input IClientEventSubscriber to notifications for any client events.
        /// Developers can implement the IClientEventSubscriber interface to handle events as they choose.
        /// Implementations of IClient are also expected to implement event notification when events occur.
        /// @param subscriber - reference to the subscriber object implementing the IClientEventSubscriber interface.
        virtual ClientErrorCode SubscribeToEvents(IClientEventSubscriber& subscriber) = 0;


        /// @brief Starts device discovery to find nearby Bluetooth devices.
        /// This is not expected to fail unless the core service is not started.
        virtual ClientErrorCode StartDiscovery() = 0;

        virtual ClientErrorCode Connect() = 0;
        virtual ClientErrorCode Disconnect() = 0;
        virtual bool IsConnected() = 0;
        virtual ClientErrorCode AnswerCall() = 0;
        virtual ClientErrorCode EndCall() = 0;
        virtual ClientErrorCode DialNumber(const char* number) = 0;

        /// @brief Retrieves the Bluetooth address (MAC) of the device.
        /// @param addressStr - output parameter to hold the Bluetooth address as a string. 
        /// If the client is not initialized, returns nullptr and a NOT_INITIALIZED error code.
        virtual ClientErrorCode GetBluetoothAddress(char addressStr[18]) = 0;
    };

}
#endif // IBLUETOOTH_HF_CLIENT_HPP   