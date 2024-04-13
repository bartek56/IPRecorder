#include "GSMTasks.hpp"
#include <string>
#include <iostream>

GSMTasks::GSMTasks(const std::string &port) : serial(port)
{
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                //                std::cout << "new message " << msg << std::endl;
                receivedMessage = std::move(msg);
                isNewMessage = true;
                cv.notify_one();
            });
}

bool GSMTasks::setConfig(const std::string &command)
{
    bool result = false;
    serial.sendMessage(command + "\r\n");
    std::unique_lock lk(messageMutex);
    cv.wait_for(lk, std::chrono::seconds(5), [this]() { return isNewMessage; });
    isNewMessage = false;
    if(receivedMessage.size() > 0)
    {
        if(receivedMessage.find("OK") != std::string::npos)
        {
            result = true;
        }
        receivedMessage = "";
    }
    else
    {
        std::cout << "failed, timeout" << std::endl;
    }

    return result;
}
