#include "ATCommander.hpp"
#include <algorithm>
#include <iostream>

ATCommander::ATCommander(const std::string &port, std::queue<Sms> &receivedSms, std::mutex &smsMux) : serial(port), receivedSmses(receivedSms), smsMutex(smsMux)
{
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                auto split = [](std::string &s, const std::string &delimiter)
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


                if(msg.find("+CMT:") != std::string::npos)
                {
                    isNewSMS = true;
                    //std::cout << "new SMS: " << msg << std::endl;
                    auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
                    auto splitted = split(msgWithoutCRLF, ",,");

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
                        receivedSmses.push(sms);
                    }
                    return;
                }

                if(msg.find("RING") != std::string::npos)
                {
                    std::cout << "RING !!!" << std::endl;
                    /// TODO
                    return;
                }
                if(msg.find("+CLIP:") != std::string::npos)
                {
                    std::cout << "Calling !!! " << msg << std::endl;
                    /// TODO
                    return;
                }
                {
                    std::lock_guard<std::mutex> lk(receivedCommandsMutex);
                    receivedCommands.push(std::move(msg));
                    isNewMessage = true;
                }
                cv.notify_one();
            });
}

bool ATCommander::setConfig(const std::string &command)
{
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

bool ATCommander::sendSms(const SmsRequest &sms)
{
    std::cout << "sending message: \"" << sms.message << "\" to " << sms.number << std::endl;
    std::string command = "AT+CMGS=\"" + sms.number + "\"";

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
    serial.sendMessage(sms.message);
    serial.sendChar(0x1A);

    if(!waitForMessage(sms.message, 5))
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

bool ATCommander::waitForMessage(const std::string &msg, const uint32_t &sec)
{
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);
    if(receivedCommands.empty())
    {
        cv.wait_for(lk, std::chrono::seconds(sec), [this]() { return isNewMessage; });
        if(!isNewMessage)
        {
            std::cout << "wait for AT message timeout" << std::endl;
            return false;
        }
        isNewMessage = false;
    }

    auto newMessage = receivedCommands.front();
    receivedCommands.pop();

    if(newMessage.find(msg) == std::string::npos)
    {
        std::cout << "Another message was received then expected" << std::endl;
        std::cout << msg << " != " << newMessage << std::endl;
        return false;
    }
    return true;
}
