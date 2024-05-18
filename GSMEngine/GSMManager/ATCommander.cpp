#include "ATCommander.hpp"
#include "spdlog/spdlog.h"
#include "ATConfig.hpp"

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


                if(msg.find(SMS_RESPONSE) != std::string::npos)
                {
                    isNewSMS = true;
                    SPDLOG_DEBUG("new SMS: {}", msg);
                    auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
                    auto splitted = split(msgWithoutCRLF, ",,");

                    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
                    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

                    auto number = split(splitted[0], " ")[1];
                    sms.number = number;
                    auto date = splitted[1];
                    sms.dateAndTime = date;
                    SPDLOG_INFO("new SMS: {} {}", date, number);

                    return;
                }
                if(isNewSMS)
                {
                    isNewSMS = false;
                    SPDLOG_INFO("{}", msg);
                    sms.msg = msg.substr(0, msg.size() - 2);
                    {
                        std::lock_guard<std::mutex> lc(smsMutex);
                        receivedSmses.push(sms);
                    }
                    return;
                }

                if(msg.find(RING) != std::string::npos)
                {
                    SPDLOG_INFO("RING !!!");
                    /// TODO
                    return;
                }
                if(msg.find(CALLING) != std::string::npos)
                {
                    SPDLOG_INFO("Calling !!! {}", msg);
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
    serial.sendMessage(command + EOL);
    if(!waitForConfirm(command))
    {
        SPDLOG_ERROR("Failed to set config {}", command);
        return false;
    }

    if(!waitForConfirm("OK"))
    {
        SPDLOG_ERROR("Failed to set config {}", command);
        return false;
    }

    return true;
}

bool ATCommander::sendSms(const SmsRequest &sms)
{
    SPDLOG_DEBUG("sending message: \"{}\" to {}", sms.message, sms.number);
    const std::string sign = "=\"";
    std::string command = AT_SMS_REQUEST + sign + sms.number + "\"";

    serial.sendMessage(command + EOL);
    if(!waitForConfirm(command))
    {
        SPDLOG_ERROR("Error");
        return false;
    }

    if(!waitForMessage(">"))
    {
        SPDLOG_ERROR("Error");
        return false;
    }

    serial.sendMessage(sms.message);
    serial.sendChar(SUB);
    if(!waitForConfirm(sms.message))
    {
        SPDLOG_ERROR("Error");
        return false;
    }

    if(!waitForMessage(SMS_REQUEST))
    {
        SPDLOG_ERROR("Error");
        return false;
    }

    if(!waitForConfirm("OK"))
    {
        SPDLOG_ERROR("Error");
        return false;
    }

    SPDLOG_INFO("message was send");
    return true;
}

bool ATCommander::waitForMessage(const std::string &msg)
{
    return waitForMessageTimeout(msg, k_waitForMessageTimeout);
}

bool ATCommander::waitForConfirm(const std::string &msg)
{
    return waitForMessageTimeout(msg, k_waitForConfirmTimeout);
}

bool ATCommander::waitForMessageTimeout(const std::string &msg, const uint32_t &miliSec)
{
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);
    if(receivedCommands.empty())
    {
        cv.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return isNewMessage; });
        if(!isNewMessage)
        {
            SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
            return false;
        }
        isNewMessage = false;
    }

    auto newMessage = receivedCommands.front();
    receivedCommands.pop();

    if(newMessage.find(msg) == std::string::npos)
    {
        SPDLOG_ERROR("Another message was received then expected: {}!={}", msg, newMessage);
        return false;
    }
    return true;
}
