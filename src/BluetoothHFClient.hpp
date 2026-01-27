#ifndef BLUETOOTH_HF_CLIENT_HPP
#define BLUETOOTH_HF_CLIENT_HPP

/**********************************************************************
 * @file BluetoothHFClient.hpp
 * @brief Header file for Bluetooth Hands-Free Client static class.
 **********************************************************************/
class BluetoothHFClient {
public:
    BluetoothHFClient() = delete;
    ~BluetoothHFClient() = delete;

    static void Initialize();
    static bool IsInitialized() { return BluetoothHFClient::isInitialized; }

    static void Connect();
    static void Disconnect();
    static bool IsConnected();

    static void AnswerCall();
    static void EndCall();
    static void DialNumber(const char* number);
    
private:
    static bool isInitialized;

};















#endif // BLUETOOTH_HF_CLIENT_HPP