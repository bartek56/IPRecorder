#include "GSMTasks.hpp"
#include <string>
#include <iostream>
#include <algorithm>

GSMTasks::GSMTasks(const std::string &port) : serial(port)
{
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                auto split = [](std::string s, std::string delimiter)
                {
                    std::vector<std::string> vec;
                    size_t pos = 0;
                    std::string token;
                    while((pos = s.find(delimiter)) != std::string::npos)
                    {
                        token = s.substr(0, pos);
                        vec.push_back(token);
                        s.erase(0, pos + delimiter.length());
                    }
                    vec.push_back(s);
                    return vec;
                };
                static bool isNewSMS = false;
                static Sms sms{};


                //std::cout << "new message " << msg << std::endl;
                if(msg.find("+CMT:") != std::string::npos)
                {
                    isNewSMS = true;
                    //std::cout << "new SMS: " << msg << std::endl;
                    auto msg2 = msg.substr(0, msg.size() - 2);
                    auto splitted = split(msg2, ",,");

                    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
                    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

                    auto number = split(splitted[0], " ")[1];
                    sms.number = number;
                    auto date = splitted[1];
                    sms.dateAndTime = date;

                    return;
                }
                if(isNewSMS)
                {
                    isNewSMS = false;
                    //std::cout << "text of SMS: " << msg << std::endl;
                    sms.msg = msg.substr(0, msg.size() - 2);
                    {
                        std::lock_guard<std::mutex> lc(smsMutex);
                        receivedMessages.push(sms);
                    }

                    return;
                }

                if(msg.find("RING") != std::string::npos)
                {
                    std::cout << "RING !!!" << std::endl;
                    return;
                }
                if(msg.find("+CLIP:") != std::string::npos)
                {
                    std::cout << "Calling !!!" << std::endl;
                    return;
                }

                receivedCommands.push(std::move(msg));
                isNewMessage = true;
                cv.notify_one();
            });
}

bool GSMTasks::setConfig(const std::string &command)
{
    bool result = false;
    serial.sendMessage(command + "\r\n");
    if(!waitForMessage(command, 5))
    {
        std::cout << "Failed to set config 1" << std::endl;
        return false;
    }

    if(!waitForMessage("OK", 5))
    {
        std::cout << "Failed to set config 2" << std::endl;
        return false;
    }

    return true;
}

bool GSMTasks::sendSms(const std::string &message, const uint32_t &number)
{
    std::string command = "AT+CMGS=\"+48" + std::to_string(number) + "\"";

    serial.sendMessage(command + "\r\n");
    if(!waitForMessage(command, 5))
    {
        std::cout << "error 1" << std::endl;
        return false;
    }

    if(!waitForMessage(">", 5))
    {
        std::cout << "error 2" << std::endl;
        return false;
    }
    serial.sendMessage(message);
    serial.sendChar(0x1A);

    if(!waitForMessage(message, 5))
    {
        std::cout << "error 3" << std::endl;
        return false;
    }

    if(!waitForMessage("+CMGS", 5))
    {
        std::cout << "error 4" << std::endl;
        return false;
    }

    if(!waitForMessage("OK", 5))
    {
        std::cout << "error 5" << std::endl;
        return false;
    }

    std::cout << "message was send" << std::endl;
    return true;
}

bool GSMTasks::isNewSms()
{
    return !receivedMessages.empty();
}

Sms GSMTasks::getLastSms()
{
    std::lock_guard<std::mutex> lc(smsMutex);
    auto lastSms = receivedMessages.front();
    receivedMessages.pop();
    return lastSms;
}

bool GSMTasks::waitForMessage(const std::string &msg, uint32_t sec)
{
    std::unique_lock<std::mutex> lk(messageMutex);
    if(receivedCommands.empty())
    {
        cv.wait_for(lk, std::chrono::seconds(sec), [this]() { return isNewMessage; });
        if(!isNewMessage)
        {
            std::cout << "wait for message timeout" << std::endl;
            return false;
        }
        isNewMessage = false;
    }
    auto newMessage = receivedCommands.front();
    receivedCommands.pop();


    isNewMessage = false;
    if(newMessage.find(msg) == std::string::npos)
    {
        std::cout << "Another message was received then expected" << std::endl;
        std::cout << msg << " != " << newMessage << std::endl;
        return false;
    }
    return true;
}
