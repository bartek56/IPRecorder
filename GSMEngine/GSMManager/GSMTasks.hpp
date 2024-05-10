#ifndef GSMTASKS_HPP
#define GSMTASKS_HPP

#include <string>
#include <queue>
#include "Serial.hpp"


struct Sms
{
    std::string number;
    std::string dateAndTime;
    std::string msg;
};

class GSMTasks
{
public:
    GSMTasks(const std::string& port);
    bool setConfig(const std::string& command);
    bool sendSms(const std::string& number, const std::string& message);
    bool isNewSms();
    Sms getLastSms();
private:
    Serial serial;
    std::mutex messageMutex;
    std::mutex smsMutex;
    std::condition_variable cv;
    bool isNewMessage;
    std::queue<std::string> receivedCommands;
    std::queue<Sms> receivedMessages;
    bool waitForMessage(const std::string& msg, uint32_t sec);
};

#endif // GSMTASKS_HPP
