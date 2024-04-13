#ifndef GSMTASKS_HPP
#define GSMTASKS_HPP

#include <string>
#include "Serial.hpp"

class GSMTasks
{
public:
    GSMTasks(const std::string& port);
    bool setConfig(const std::string& command);
private:
    Serial serial;
    std::mutex messageMutex;
    std::condition_variable cv;
    bool isNewMessage;
    std::string receivedMessage;
};

#endif // GSMTASKS_HPP
