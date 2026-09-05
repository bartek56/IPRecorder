#include "ATCommanderScheduler.hpp"
#include "ATConfig.hpp"
#include "ATParser.hpp"
#include "spdlog/spdlog.h"
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <stop_token>
#include <thread>
#include <utility>


namespace AT
{
ATCommanderScheduler::ATCommanderScheduler(std::string_view port) : serial(port)
{
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                SPDLOG_TRACE("new AT message: {}", msg);
                const std::lock_guard<std::mutex> lockReceivedCommads(receivedCommandsMutex);
                isNewMsgFromAt = true;
                receivedCommands.emplace_back(std::chrono::steady_clock::now(), msg);
                cvATReceiver.notify_one();
            });

    atThread = std::jthread([this](std::stop_token stopToken) { atCommandManager(stopToken); });
}

void ATCommanderScheduler::shutdown()
{
    if(!isAliveFlag.load(std::memory_order_acquire))
        return;

    isAliveFlag.store(false, std::memory_order_release);
    atThread.request_stop();
    cvATReceiver.notify_all();
}

bool ATCommanderScheduler::isAlive() const
{
    return isAliveFlag.load(std::memory_order_acquire);
}

bool ATCommanderScheduler::setConfigATE0()
{
    SPDLOG_DEBUG("Set Config ATE0");
    const std::string ATE0 = "ATE0";
    serial.sendMessage(ATE0);
    std::string lastMessage;
    if(!getLastMessageWithTimeout(k_waitForConfirmTimeout, lastMessage))
    {
        return false;
    }

    if(lastMessage.find("OK") != std::string::npos)
    {
        // it was set on the previous session
        return true;
    }
    if(lastMessage.find(ATE0) != std::string::npos)
    {
        // std::cout << "it is first setting, get next message" << std::endl;
        if(!getLastMessageWithTimeout(k_waitForConfirmTimeout, lastMessage))
        {
            return false;
        }

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
    serial.sendMessage(std::string(AT_SYNC));
    if(!waitForSyncConfirm("OK"))
    {
        SPDLOG_ERROR("OK message was not arrived! SendSync failed!");
        return false;
    }
    if(hasReceivedCommands())
    {
        SPDLOG_ERROR("Unexpected message was came! SendSync failed!");
        return false;
    }

    return true;
}

bool ATCommanderScheduler::getLastMessageWithTimeout(const uint32_t &miliSec, std::string &msg)
{
    SPDLOG_TRACE("getLastMessageWithTimeout {}ms", miliSec);

    std::unique_lock<std::mutex> lockReceivedCommands(receivedCommandsMutex);

    if(!receivedCommands.empty())
    {
        auto receivedCommand = receivedCommands.back();
        msg = receivedCommand.command;
        receivedCommands.pop_back();
        return true;
    }
    SPDLOG_TRACE("Message queue is empty, waiting for new meesage");

    isNewMsgFromAt = false;
    cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
    if(!isNewMsgFromAt)
    {
        SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
        return false;
    }
    isNewMsgFromAt = false;
    // refresh heart beat
    heartBeatRefresh();
    SPDLOG_TRACE("new message was arrived");

    if(!receivedCommands.empty())
    {
        auto receivedCommand = receivedCommands.back();
        msg = receivedCommand.command;
        SPDLOG_TRACE("Last message:{}", msg);
        receivedCommands.pop_back();
        return true;
    }

    SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
    return false;
}

bool ATCommanderScheduler::hasReceivedCommands()
{
    const std::lock_guard lockReceivedCommands(receivedCommandsMutex);
    return !receivedCommands.empty();
}

bool ATCommanderScheduler::hasAtRequests()
{
    const std::lock_guard lockRequests(atRequestsMutex);
    return !atRequestsQueue.empty();
}

bool ATCommanderScheduler::hasSmsRequests()
{
    const std::lock_guard lockRequests(atSmsRequestMutex);
    return !atSmsRequestQueue.empty();
}

bool ATCommanderScheduler::getOldestMessage(std::string &msg)
{
    SPDLOG_TRACE("getOldestMessage");
    const std::lock_guard lockReceivedCommands(receivedCommandsMutex);
    if(receivedCommands.empty())
        return false;

    msg = receivedCommands.front().command;
    receivedCommands.pop_front();
    return true;
}

bool ATCommanderScheduler::getOldestMessageWithTimeout(const uint32_t &miliSec, std::string &msg)
{
    SPDLOG_TRACE("getOldestMessageWithTimeout");
    std::unique_lock<std::mutex> lockReceivedCommands(receivedCommandsMutex);
    if(!receivedCommands.empty())
    {
        msg = receivedCommands.front().command;
        receivedCommands.pop_front();
        SPDLOG_DEBUG("\"{}\" - last message", msg);
        return true;
    }
    SPDLOG_TRACE("wait for new AT message");
    isNewMsgFromAt = false;
    cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
    if(!isNewMsgFromAt)
    {
        SPDLOG_ERROR("wait for the oldest message timeout: {}ms", miliSec);
        return false;
    }
    isNewMsgFromAt = false;
    heartBeatRefresh();
    SPDLOG_TRACE("new message was arrived");

    if(!receivedCommands.empty())
    {
        msg = receivedCommands.front().command;
        receivedCommands.pop_front();
        SPDLOG_DEBUG("\"{}\" - new message", msg);
        return true;
    }
    return false;
}

bool ATCommanderScheduler::waitForMessage(std::string_view msg,
                                          const std::chrono::steady_clock::time_point &timePoint)
{
    return waitForMessageTimeout(msg, timePoint, k_waitForMessageTimeout);
}

bool ATCommanderScheduler::waitForConfirm(std::string_view msg,
                                          const std::chrono::steady_clock::time_point &timePoint)
{
    return waitForMessageTimeout(msg, timePoint, k_waitForConfirmTimeout);
}

bool ATCommanderScheduler::waitForSyncConfirm(const std::string &msg)
{
    SPDLOG_TRACE("waitForSyncConfirm: msg: {}", msg);
    std::unique_lock<std::mutex> lockReceivedCommands(receivedCommandsMutex);

    for(auto it = receivedCommands.rbegin(); it != receivedCommands.rend(); ++it)
    {
        if(it->command.find(msg) != std::string::npos)
        {
            SPDLOG_TRACE("\"{}\" was confirmed", msg);
            receivedCommands.erase((it + 1).base());
            return true;
        }
    }

    SPDLOG_TRACE("Expected message was not found: {}, loop is starting", msg);
    auto startPt = std::chrono::steady_clock::now();
    auto endPt = startPt;
    SPDLOG_TRACE("cycle: wait for new AT message: \"{}\"", msg);
    isNewMsgFromAt = false;
    cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(k_waitForConfirmTimeout), [this]() { return isNewMsgFromAt; });
    if(!isNewMsgFromAt)
    {
        SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, k_waitForConfirmTimeout);
        return false;
    }
    isNewMsgFromAt = false;
    heartBeatRefresh();
    SPDLOG_TRACE("new message was arrived");

    auto lastMessage = receivedCommands.begin();
    if(lastMessage->command.find(msg) != std::string::npos)
    {
        SPDLOG_TRACE("\"{}\" new msg was confirmed", msg);
        receivedCommands.erase(lastMessage);
        return true;
    }

    endPt = std::chrono::steady_clock::now();
    SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, k_waitForConfirmTimeout);
    return false;
}

bool ATCommanderScheduler::waitForMessageTimeout(std::string_view msg,
                                                 const std::chrono::steady_clock::time_point &timePoint,
                                                 const uint32_t &miliSec)
{
    SPDLOG_TRACE("waitForLastMessageTimeout: msg: {}, timeout: {}ms", msg, miliSec);
    std::unique_lock<std::mutex> lockReceivedCommands(receivedCommandsMutex);

    for(auto it = receivedCommands.rbegin(); it != receivedCommands.rend(); ++it)
    {
        if(it->timestamp < timePoint)
        {
            break;
        }
        if(it->command.find(msg) != std::string::npos)
        {
            SPDLOG_DEBUG("\"{}\" was confirmed", msg);
            receivedCommands.erase((it + 1).base());
            return true;
        }
    }

    SPDLOG_TRACE("Expected message was not found: {}, loop is starting", msg);
    auto startPt = std::chrono::steady_clock::now();
    auto endPt = startPt;
    while(std::chrono::duration_cast<std::chrono::milliseconds>(endPt - startPt).count() < miliSec)
    {
        SPDLOG_TRACE("cycle: wait for new AT message: \"{}\"", msg);
        auto numberOfMsg = static_cast<int64_t>(receivedCommands.size());
        isNewMsgFromAt = false;
        cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
        if(!isNewMsgFromAt)
        {
            SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
            return false;
        }
        isNewMsgFromAt = false;
        heartBeatRefresh();
        SPDLOG_TRACE("new message was arrived");

        auto result =
                std::find_if(receivedCommands.begin() + numberOfMsg, receivedCommands.end(),
                             [&msg](const ATResponse& atReponse) { return atReponse.command.find(msg) != std::string::npos; });

        if(result != receivedCommands.end())
        {
            SPDLOG_DEBUG("\"{}\" new msg was confirmed", msg);
            receivedCommands.erase(result);
            return true;
        }
        SPDLOG_TRACE("New message is still not as expected");

        endPt = std::chrono::steady_clock::now();
    }
    SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
    return false;
}

void ATCommanderScheduler::atCommandManager(std::stop_token stopToken)
{
    if(!setConfigATE0())
    {
        SPDLOG_ERROR("failed to set ATE0");
        return;
    }
    heartBeatRefresh();
    while(!stopToken.stop_requested())
    {
        // Requests, status etc from GSM
        while(true)
        {
            std::string msg;
            if(!getOldestMessage(msg))
                break;
            SPDLOG_DEBUG("AT response/msg from receivedCommands: {}", msg);

            if(msg.find(SMS_RESPONSE) != std::string::npos and msg.find("\",,\"") != std::string::npos)
            {
                smsProcessing(msg);
                continue;
            }

            if(msg.find(RING) != std::string::npos)
            {
                SPDLOG_INFO("RING !!!");
                continue;
            }
            if(msg.find(CALLING) != std::string::npos)
            {
                callingProcessing(msg);
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
        if(hasAtRequests() && !hasReceivedCommands())
        {
            while(hasAtRequests())
            {
                configProcessing();
            }
        }

        // Request SMS to GSM
        if(hasSmsRequests() && !hasReceivedCommands())
        {
            while(hasSmsRequests())
            {
                smsRequestProcessing();
            }
        }

        if(!hasReceivedCommands())
        {
            if(!heartBeatTick())
            {
                SPDLOG_ERROR("Heart beat timeout. Stopping AT command manager gracefully.");
                shutdown();
                return;
            }
        }


        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    SPDLOG_DEBUG("AT comnand manager thread closed");
}

void ATCommanderScheduler::smsProcessing(const std::string &msg)
{
    const auto smsHeader = parseSmsHeader(msg);
    if(!smsHeader)
    {
        SPDLOG_ERROR("Invalid SMS header: {}", msg);
        return;
    }

    Sms sms;
    sms.number = smsHeader->number;
    sms.dateAndTime = smsHeader->dateAndTime;
    SPDLOG_INFO("new SMS info: {} {}", sms.dateAndTime, sms.number);

    // get next message from the queue (text of SMS)
    std::string msgSms;

    const bool result = getOldestMessageWithTimeout(k_waitForConfirmTimeout, msgSms);
    if(!result)
    {
        SPDLOG_ERROR("Failed to get SMS message");
        return;
    }
    SPDLOG_INFO("new SMS text: {}", msgSms);
    sms.msg = parseSmsBody(msgSms);
    {
        const std::lock_guard<std::mutex> lockSmsMutex(smsMutex);
        receivedSmses.push(std::move(sms));
    }
}

void ATCommanderScheduler::callingProcessing(const std::string &msg)
{
    // +CLIP: "+48123456789",145,,,"",0
    auto number = parseClipNumber(msg);
    if(!number)
    {
        SPDLOG_ERROR("Invalid call notification: {}", msg);
        return;
    }

    SPDLOG_INFO("Calling from {} !!! ", *number);

    ATRequest request = ATRequest();
    request.request = "ATH";
    request.responsexpected.emplace_back("NO CARRIER");
    request.responsexpected.emplace_back("OK");
    {
        const std::lock_guard lockRequestsMutex(atRequestsMutex);
        atRequestsQueue.push(ATRequestTask{std::move(request), nullptr});
    }
    {
        const std::lock_guard lockCalls(callsMutex);
        calls.emplace(*number);
    }
}

bool ATCommanderScheduler::configProcessing()
{
    ATRequestTask task;
    {
        const std::lock_guard lockRequestsMutex(atRequestsMutex);
        task = std::move(atRequestsQueue.front());
        atRequestsQueue.pop();
    }
    SPDLOG_TRACE("AT request: {}", task.request.request);
    auto now = std::chrono::steady_clock::now();
    serial.sendMessage(task.request.request);
    auto expectedResponses = task.request.responsexpected;
    bool success = true;
    for(const auto &expect : expectedResponses)
    {
        if(!waitForConfirm(expect, now))
        {
            success = false;
            SPDLOG_ERROR("Expected msg was not arrived: {}", expect);
            SPDLOG_ERROR("Failed to set config {}", task.request.request);
        }
    }
    if(task.completion)
        task.completion->set_value(success);
    return success;
}

bool ATCommanderScheduler::smsRequestProcessing()
{
    SmsRequestTask task;
    {
        const std::lock_guard lockRequestsMutex(atSmsRequestMutex);
        task = std::move(atSmsRequestQueue.front());
        atSmsRequestQueue.pop();
    }
    auto sms = std::move(task.request);
    SPDLOG_DEBUG("Sending SMS: \"{}\" to {}", sms.message, sms.number);
    const std::string sign = "=\"";
    const std::string command = std::string(AT_SMS_REQUEST) + sign + sms.number + "\"";

    auto now = std::chrono::steady_clock::now();
    serial.sendMessage(command);
    if(!waitForMessage(SMS_INPUT, now))
    {
        SPDLOG_ERROR("msg:> was not arrived");
        if(task.completion)
            task.completion->set_value(false);
        return false;
    }
    now = std::chrono::steady_clock::now();
    serial.sendMessage(sms.message);
    serial.sendChar(SUB);

    if(!waitForMessage(SMS_REQUEST, now))
    {
        SPDLOG_ERROR("msg:{} was not arrived", SMS_REQUEST);
        if(task.completion)
            task.completion->set_value(false);
        return false;
    }

    if(!waitForConfirm("OK", now))
    {
        SPDLOG_ERROR("msg:OK was not arrived");
        if(task.completion)
            task.completion->set_value(false);
        return false;
    }

    SPDLOG_INFO("message \"{}\" was send to {}", sms.message, sms.number);
    if(task.completion)
        task.completion->set_value(true);
    return true;
}

void ATCommanderScheduler::heartBeatRefresh()
{
    lastRefresh = std::chrono::steady_clock::now();
}

bool ATCommanderScheduler::heartBeatTick()
{
    if((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastRefresh).count()) > 10)
    {
        SPDLOG_TRACE("TIMEOUT");

        if(!sendSync())
        {
            SPDLOG_ERROR("Critical issue !!!");
            return false;
        }

        heartBeatRefresh();
    }
    return true;
}

ATCommanderScheduler::~ATCommanderScheduler()
{
    shutdown();
}
}// namespace AT
