#ifndef GSMTASKS_HPP
#define GSMTASKS_HPP

#include <string>
#include <queue>
#include "Serial.hpp"

class GSMTasks
{
public:
    GSMTasks(const std::string& port);
    bool setConfig(const std::string& command);
    bool sendSms(const std::string& message, const uint32_t& number);
private:
    Serial serial;
    std::mutex messageMutex;
    std::condition_variable cv;
    bool isNewMessage;
    std::queue<std::string> receivedMessages;
    bool waitForMessage(const std::string& msg, uint32_t sec);
};

#endif // GSMTASKS_HPP
