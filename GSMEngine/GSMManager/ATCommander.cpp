#include "ATCommander.hpp"
#include "spdlog/spdlog.h"
#include "ATConfig.hpp"

#include <algorithm>

ATCommander::ATCommander(const std::string &port, std::queue<Sms> &receivedSms, std::mutex &smsMux)
    : serial(port), receivedSmses(receivedSms), smsMutex(smsMux)
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

                SPDLOG_DEBUG("new AT message: {}", msg);

                if(msg.find(SMS_RESPONSE) != std::string::npos and msg.find("\",,\"") != std::string::npos)
                {
                    isNewSMS = true;
                    auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
                    auto splitted = split(msgWithoutCRLF, ",,");

                    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
                    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

                    auto number = split(splitted[0], " ")[1];
                    sms.number = number;
                    auto date = splitted[1];
                    sms.dateAndTime = date;
                    SPDLOG_INFO("new SMS info: {} {}", date, number);

                    return;
                }
                if(isNewSMS)
                {
                    isNewSMS = false;
                    SPDLOG_INFO("new SMS text: {}", msg);
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
                if(msg.find(ERROR) != std::string::npos)
                {
                    SPDLOG_ERROR("ERROR !!!");
                    return;
                }
                SPDLOG_DEBUG("New command:\"{}\"", msg);
                {
                    std::lock_guard<std::mutex> lk(receivedCommandsMutex);
                    receivedCommands.push(std::move(msg));
                }
                cv.notify_one();
            });
    if(!setConfigATE0())
    {
        SPDLOG_ERROR("failed to set ATE0");
    }
}

bool ATCommander::setConfig(const std::string &command)
{
    SPDLOG_DEBUG("set config message: {}", command);
    serial.sendMessage(command + EOL);
    if(!waitForConfirm("OK"))
    {
        SPDLOG_ERROR("Failed to set config {}", command);
        return false;
    }

    return true;
}

bool ATCommander::setConfigATE0()
{
    SPDLOG_DEBUG("Set Config ATE0");
    const std::string ATE0 = "ATE0";
    serial.sendMessage(ATE0 + EOL);
    std::string lastMessage;

    if(!getMessageWithTimeout(k_waitForConfirmTimeout, lastMessage))
        return false;

    if(lastMessage.find("OK") != std::string::npos)
    {
        // it was set on the previous session
        return true;
    }
    else if(lastMessage.find(ATE0) != std::string::npos)
    {
        // std::cout << "it is first setting, get next message" << std::endl;
        if(!getMessageWithTimeout(k_waitForConfirmTimeout, lastMessage))
            return false;

        if(lastMessage.find("OK") != std::string::npos)
        {
            return true;
        }
    }
    else
    {
        SPDLOG_ERROR("another message was received then expected: {}", lastMessage);
        return false;
    }
    SPDLOG_ERROR("another message was received then expected: {}", lastMessage);
    return false;
}

bool ATCommander::sendSms(const SmsRequest &sms)
{
    SPDLOG_DEBUG("sending message: \"{}\" to {}", sms.message, sms.number);
    clearQueue();
    const std::string sign = "=\"";
    std::string command = AT_SMS_REQUEST + sign + sms.number + "\"";

    serial.sendMessage(command + EOL);
    if(!waitForMessage(">"))
    {
        SPDLOG_ERROR("Error");
        return false;
    }
    serial.sendMessage(sms.message);
    serial.sendChar(SUB);

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

    SPDLOG_INFO("message \"{}\" was send to {}", sms.message, sms.number);
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
    std::string newMessage;
    if(!getMessageWithTimeout(miliSec, newMessage))
    {
        return false;
    }

    if(newMessage.find(msg) == std::string::npos)
    {
        SPDLOG_ERROR("another message was received: \"{}\", expected: \"{}\", timeout[ms]:{}", newMessage, msg, miliSec);
        return false;
    }
    return true;
}

bool ATCommander::getMessageWithTimeout(const uint32_t &miliSec, std::string &msg)
{
    SPDLOG_DEBUG("GetMessageWithTimeout {}ms", miliSec);
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);
    if(receivedCommands.empty())
    {
        cv.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return !receivedCommands.empty(); });
        if(receivedCommands.empty())
        {
            SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
            return false;
        }
        SPDLOG_DEBUG("Message was arrived");
    }
    msg = receivedCommands.front();
    SPDLOG_DEBUG("Take message from the queue:\"{}\"", msg);
    receivedCommands.pop();
    
    return true;
}

void ATCommander::clearQueue()
{
    std::lock_guard<std::mutex> lk(receivedCommandsMutex);
    if(!receivedCommands.empty())
    {
        SPDLOG_ERROR("msg queue is not empty, some message will be skipped !!!");
        while(!receivedCommands.empty())
        {
            auto msg = receivedCommands.front();
            SPDLOG_ERROR("{}", msg);
            receivedCommands.pop();
        }
    }
}
