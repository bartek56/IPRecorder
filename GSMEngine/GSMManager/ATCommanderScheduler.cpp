#include "spdlog/spdlog.h"
#include "ATCommanderScheduler.hpp"

#include "ATConfig.hpp"

ATCommanderScheduler::ATCommanderScheduler(const std::string &port) : serial(port)
{
    receivedCommands.reserve(20);
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                SPDLOG_DEBUG("new AT message: {}", msg);
                std::lock_guard<std::mutex> lk(receivedCommandsMutex);
                isNewMsgFromAt = true;
                receivedCommands.push_back(msg);
                cvATReceiver.notify_one();
            });


    atCommandManagerIsRunning.store(true);
    atThread = std::make_unique<std::thread>([this]() { this->atCommandManager(); });
}

bool ATCommanderScheduler::setConfigATE0()
{
    SPDLOG_DEBUG("Set Config ATE0");
    const std::string ATE0 = "ATE0";
    serial.sendMessage(ATE0);
    std::string lastMessage;
    if(!getLastMessageWithTimeout(k_waitForConfirmTimeout, lastMessage))
        return false;

    if(lastMessage.find("OK") != std::string::npos)
    {
        // it was set on the previous session
        return true;
    }
    else if(lastMessage.find(ATE0) != std::string::npos)
    {
        // std::cout << "it is first setting, get next message" << std::endl;
        if(!getLastMessageWithTimeout(k_waitForConfirmTimeout, lastMessage))
            return false;

        if(lastMessage.find("OK") != std::string::npos)
        {
            return true;
        }
    }

    SPDLOG_ERROR("setConfigATE0 failed!");
    return false;
}

bool ATCommanderScheduler::sendSync()
{
    const std::string atSync = "AT";
    serial.sendMessage(atSync);
    if(!waitForMessage("OK"))
    {
        SPDLOG_ERROR("OK message was not arrived! SendSync failed!");
        return false;
    }
    return true;
}

bool ATCommanderScheduler::getLastMessageWithTimeout(const uint32_t &miliSec, std::string &msg)
{
    SPDLOG_TRACE("getLastMessageWithTimeout {}ms", miliSec);

    std::unique_lock<std::mutex> lk(receivedCommandsMutex);

    if(!receivedCommands.empty())
    {
        msg = receivedCommands.back();
        receivedCommands.pop_back();
        return true;
    }
    SPDLOG_TRACE("Message queue is empty, waiting for new meesage");

    auto startPt = std::chrono::steady_clock::now();
    auto endPt = startPt;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(endPt - startPt).count() < miliSec)
    {
        SPDLOG_TRACE("cycle: wait for new AT message: \"{}\"", msg);
        cvATReceiver.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
        if(!isNewMsgFromAt)
        {
            SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
            return false;
        }
        isNewMsgFromAt = false;
        // refresh heart beat
        lastRefresh = std::chrono::steady_clock::now();
        SPDLOG_TRACE("new message was arrived");

        if(!receivedCommands.empty())
        {
            SPDLOG_TRACE("Get last message");
            msg = receivedCommands.back();
            receivedCommands.pop_back();
            return true;
        }

        endPt = std::chrono::steady_clock::now();
    }

    SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
    return false;
}

std::string ATCommanderScheduler::getOldestMessage()
{
    SPDLOG_TRACE("getOldestMessage");
    std::lock_guard lk(receivedCommandsMutex);
    auto msg = receivedCommands.front();
    receivedCommands.erase(receivedCommands.begin());
    return msg;
}

bool ATCommanderScheduler::waitForMessage(const std::string &msg)
{
    return waitForMessageTimeout(msg, k_waitForMessageTimeout);
}

bool ATCommanderScheduler::waitForConfirm(const std::string &msg)
{
    return waitForMessageTimeout(msg, k_waitForConfirmTimeout);
}

bool ATCommanderScheduler::waitForMessageTimeout(const std::string &msg, const uint32_t &miliSec)
{
    SPDLOG_TRACE("waitForMessageTimeout: msg: {}, timeout: {}ms", msg, miliSec);
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);

    auto result = std::find_if(receivedCommands.begin(), receivedCommands.end(),
                               [&msg](std::string m) { return m.find(msg) != std::string::npos; });

    if(result != receivedCommands.end())
    {
        SPDLOG_TRACE("Message was found");
        receivedCommands.erase(result);
        return true;
    }
    SPDLOG_TRACE("Expected message was not found: {}, loop is starting", msg);

    auto startPt = std::chrono::steady_clock::now();
    auto endPt = startPt;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(endPt - startPt).count() < miliSec)
    {
        SPDLOG_TRACE("cycle: wait for new AT message: \"{}\"", msg);
        cvATReceiver.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
        if(!isNewMsgFromAt)
        {
            SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
            return false;
        }
        isNewMsgFromAt = false;
        // refresh heart beat
        lastRefresh = std::chrono::steady_clock::now();
        SPDLOG_TRACE("new message was arrived");

        result = std::find_if(receivedCommands.begin(), receivedCommands.end(),
                              [&msg](std::string m) { return m.find(msg) != std::string::npos; });

        if(result != receivedCommands.end())
        {
            SPDLOG_TRACE("Message was found");
            receivedCommands.erase(result);
            return true;
        }
        else
        {
            SPDLOG_TRACE("New message is not correct, wait for next message");
        }

        endPt = std::chrono::steady_clock::now();
    }
    SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
    return false;
}

std::vector<std::string> ATCommanderScheduler::split(std::string &s, const std::string &delimiter)
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
}

void ATCommanderScheduler::atCommandManager()
{
    if(!setConfigATE0())
    {
        SPDLOG_ERROR("failed to set ATE0");
        std::exit(0);
    }
    lastRefresh = std::chrono::steady_clock::now();
    while(atCommandManagerIsRunning.load())
    {
        // Requests, status etc from GSM
        while(!receivedCommands.empty())
        {
            std::string msg = getOldestMessage();
            SPDLOG_DEBUG("AT response/msg from receivedCommands: {}", msg);

            if(msg.find(SMS_RESPONSE) != std::string::npos and msg.find("\",,\"") != std::string::npos)
            {
                auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
                auto splitted = split(msgWithoutCRLF, ",,");

                splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
                splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

                auto number = split(splitted[0], " ")[1];
                Sms sms;
                sms.number = number;
                auto date = splitted[1];
                sms.dateAndTime = date;
                SPDLOG_INFO("new SMS info: {} {}", date, number);

                // get next message from the queue (text of SMS)
                std::string msgSms;
                bool result = getLastMessageWithTimeout(k_waitForMessageTimeout, msgSms);
                if(!result)
                {
                    SPDLOG_ERROR("Failed to get SMS message");
                    continue;
                }
                SPDLOG_INFO("new SMS text: {}", msgSms);
                sms.msg = msgSms.substr(0, msgSms.size() - 2);
                {
                    std::lock_guard<std::mutex> lc(smsMutex);
                    receivedSmses.push(std::move(sms));
                }
                continue;
            }

            if(msg.find(RING) != std::string::npos)
            {
                SPDLOG_INFO("RING !!!");
                continue;
            }
            if(msg.find(CALLING) != std::string::npos)
            {
                SPDLOG_INFO("Calling !!! {}", msg);
                ATRequest request = ATRequest();
                request.request = "ATH";
                request.responsexpected.push_back("NO CARRIER");
                request.responsexpected.push_back("OK");
                {
                    std::lock_guard lock(atRequestsMutex);
                    atRequestsQueue.push(request);
                }
                continue;
            }
            if(msg.find(ERROR) != std::string::npos)
            {
                SPDLOG_ERROR("ERROR !!!");
                continue;
            }

            SPDLOG_WARN("Message \"{}\" was skipped !", msg);
        }

        // Request config to GSM
        if(atRequestsQueue.size() > 0)
        {
            while(atRequestsQueue.size() > 0)
            {
                ATRequest lastTask;
                {
                    std::lock_guard lock(atRequestsMutex);
                    lastTask = atRequestsQueue.front();
                    atRequestsQueue.pop();
                }
                SPDLOG_DEBUG("AT request: {}", lastTask.request);
                serial.sendMessage(lastTask.request);
                auto expectedResponses = lastTask.responsexpected;
                for(const auto &expect : expectedResponses)
                {
                    /// TODO it should be safe when new meesage was came on this loop.
                    /// Right now message received queue has to be empty
                    if(!waitForConfirm(expect))
                    {
                        SPDLOG_ERROR("Expected msg was not arrived: {}", expect);
                        SPDLOG_ERROR("Failed to set config {}", lastTask.request);
                    }
                }
            }
            cvSmsRequests.notify_one();
        }

        // Request SMS to GSM
        if(atSmsRequestQueue.size() > 0 && receivedCommands.empty())
        {
            SmsRequest sms;
            {
                std::lock_guard lock(atSmsRequestMutex);
                sms = atSmsRequestQueue.front();
                atSmsRequestQueue.pop();
            }
            SPDLOG_DEBUG("Sending SMS: \"{}\" to {}", sms.message, sms.number);
            const std::string sign = "=\"";
            std::string command = AT_SMS_REQUEST + sign + sms.number + "\"";

            serial.sendMessage(command);
            if(!waitForMessage(">"))
            {
                SPDLOG_ERROR("Error");
                continue;
            }
            serial.sendMessage(sms.message);
            serial.sendChar(SUB);


            if(!waitForMessage(SMS_REQUEST))
            {
                SPDLOG_ERROR("Error");
            }

            if(!waitForConfirm("OK"))
            {
                SPDLOG_ERROR("Error");
            }

            SPDLOG_INFO("message \"{}\" was send to {}", sms.message, sms.number);
        }

        if((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastRefresh).count()) >
           10)
        {
            SPDLOG_TRACE("TIMEOUT");

            if(!sendSync())
            {
                SPDLOG_ERROR("Critical issue !!!");
                std::exit(0);
            }

            lastRefresh = std::chrono::steady_clock::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    SPDLOG_DEBUG("AT comander thread closed");
}

ATCommanderScheduler::~ATCommanderScheduler()
{
    atCommandManagerIsRunning.store(false);
}
