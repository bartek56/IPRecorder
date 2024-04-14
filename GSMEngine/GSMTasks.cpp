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
    if(!waitForMessage("OK", 5))
    {
        std::cout << "Failed to set config";
        return false;
    }
    return true;
}

bool GSMTasks::sendSms(const std::string &message, const uint32_t &number)
{
    std::string command = "AT+CMGS=\"+48" + std::to_string(number) + "\"\r\n";

    serial.sendMessage(command);

    if(!waitForMessage(">", 5))
    {
        std::cout << "error 1" << std::endl;
        return false;
    }
    serial.sendMessage(message);
    serial.sendChar(0x1A);

    if(!waitForMessage(message, 5))
    {
        std::cout << "error 2" << std::endl;
        return false;
    }

    if(!waitForMessage("OK", 5))
    {
        std::cout << "error 3" << std::endl;
        return false;
    }

    std::cout << "message was send" << std::endl;
    return true;
}

bool GSMTasks::waitForMessage(const std::string &msg, uint32_t sec)
{
    std::unique_lock<std::mutex> lk(messageMutex);
    cv.wait_for(lk, std::chrono::seconds(sec), [this]() { return isNewMessage; });
    bool result = false;
    if(isNewMessage)
    {
        isNewMessage = false;
        if(receivedMessage.find(msg) != std::string::npos)
        {
            receivedMessage = "";
            result = true;
        }
        else
        {
            std::cout << "Another message was received then expected" << std::endl;
            std::cout << msg << " != " << receivedMessage << std::endl;
        }
    }
    else
    {
        std::cout << "wait for message timeout" << std::endl;
    }
    return result;
}
