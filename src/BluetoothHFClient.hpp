#ifndef BLUETOOTH_HF_CLIENT_HPP
#define BLUETOOTH_HF_CLIENT_HPP

/**********************************************************************
 * @file BluetoothHFClient.hpp
 * @brief Header file for Bluetooth Hands-Free Client static class.
 **********************************************************************/

namespace BluetoothHF {

    enum class ClientErrorType {
        SUCCESS = 0,
        NOT_INITIALIZED,
    };

    class Client {
    public:
        Client() = delete;
        ~Client() = delete;

        static ClientErrorType Initialize();
        static bool IsInitialized() { return Client::isInitialized; }

        static ClientErrorType Connect();
        static ClientErrorType Disconnect();
        static bool IsConnected();

        static ClientErrorType AnswerCall();
        static ClientErrorType EndCall();
        static ClientErrorType DialNumber(const char* number);
        
    private:
        static bool isInitialized;

    };

}
#endif // BLUETOOTH_HF_CLIENT_HPP