#include "GSMTasks.hpp"
#include <string>
#include <iostream>

GSMTasks::GSMTasks(const std::string &port) : serial(port)
{
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                std::cout << "new message " << msg << std::endl;
                receivedMessage = std::move(msg);
                isNewMessage = true;
                cv.notify_one();
            });
}

bool GSMTasks::setConfig(const std::string &command)
{
    bool result = false;
    serial.sendMessage(command + "\r\n");
    std::unique_lock<std::mutex> lk(messageMutex);
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

bool GSMTasks::sendSms(const std::string &message, const uint32_t &number)
{
    bool result = false;
    std::string command = "AT+CMGS=\"+48" + std::to_string(number) + "\"\r\n";

    serial.sendMessage(command);
    std::unique_lock<std::mutex> lk(messageMutex);
    cv.wait_for(lk, std::chrono::seconds(5), [this]() { return isNewMessage; });
    isNewMessage = false;
    if(receivedMessage.size() > 0)
    {
        if(receivedMessage.find(">") != std::string::npos)
        {
            serial.sendMessage(message);
            serial.sendChar(0x1A);
        }
        receivedMessage = "";
    }
    else
    {
        std::cout << "failed, timeout" << std::endl;
        return false;
    }


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

    std::cout << "message was send. Status " << result << std::endl;

    return result;
}
