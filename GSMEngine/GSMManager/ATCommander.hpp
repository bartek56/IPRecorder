#ifndef ATCOMMANDER_HPP
#define ATCOMMANDER_HPP

#include <string>
#include <queue>
#include "Serial.hpp"

struct Sms
{
    std::string number;
    std::string dateAndTime;
    std::string msg;
};

class ATCommander
{
public:
    ATCommander(const std::string &port, std::queue<Sms>& receivedSms, std::mutex& smsMutex);

    bool sendSmsSerial(const std::string& number, const std::string& message);
    bool setConfig(const std::string& command);
private:
    Serial serial;
    std::queue<Sms>& receivedSmses;
    std::mutex& smsMutex;

    std::mutex receivedCommandsMutex;
    std::queue<std::string> receivedCommands;

    bool isNewMessage;
    std::condition_variable cv;
    bool waitForMessage(const std::string& msg, const uint32_t& sec);
};

#endif // ATCOMMANDER_HPP
