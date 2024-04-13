#include "GSMTasks.hpp"
#include <string>
#include <iostream>

GSMTasks::GSMTasks(const std::string &port) : serial(port)
{
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                std::cout << "new message " << msg << std::endl;
                receivedMessage = msg;
                isNewMessage = true;
                cv.notify_one();
            });
}

bool GSMTasks::setConfig(std::string command)
{
    auto command2 = command + "\r\n";
    serial.sendMessage(command2);
    std::cout << "after send: " << command << std::endl;
    std::unique_lock lk(messageMutex);
    cv.wait_for(lk, std::chrono::seconds(5), [this]() { return isNewMessage; });
    isNewMessage = false;
    if(receivedMessage.size() > 0)
    {
        std::cout << "received message: " << receivedMessage << std::endl;
        receivedMessage = "";
    }
    else
    {
        std::cout << "failed, timeout" << std::endl;
    }

    return true;
}
