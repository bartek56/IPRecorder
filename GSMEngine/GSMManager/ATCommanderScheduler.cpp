#include "spdlog/spdlog.h"
#include "Utils.hpp"
#include "ATCommanderScheduler.hpp"

#include "ATConfig.hpp"
namespace AT
{
ATCommanderScheduler::ATCommanderScheduler(std::string_view port) : serial(port)
{
    receivedCommands.reserve(20);
    serial.setReadEvent(
            [&](const std::string &msg)
            {
                SPDLOG_TRACE("new AT message: {}", msg);
                std::lock_guard<std::mutex> lk(receivedCommandsMutex);
                isNewMsgFromAt = true;
                receivedCommands.push_back(ATResponse(std::chrono::steady_clock::now(), msg));
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
    serial.sendMessage(AT_SYNC);
    if(!waitForSyncConfirm("OK"))
    {
        SPDLOG_ERROR("OK message was not arrived! SendSync failed!");
        return false;
    }
    if(!receivedCommands.empty())
    {
        SPDLOG_ERROR("Unexpected message was came! SendSync failed!");
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
        auto receivedCommand = receivedCommands.back();
        msg = receivedCommand.command;
        receivedCommands.pop_back();
        return true;
    }
    SPDLOG_TRACE("Message queue is empty, waiting for new meesage");

    isNewMsgFromAt = false;
    cvATReceiver.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
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

std::string ATCommanderScheduler::getOldestMessage()
{
    SPDLOG_TRACE("getOldestMessage");
    std::lock_guard lk(receivedCommandsMutex);
    auto receivedCommand = receivedCommands.front();
    auto msg = receivedCommand.command;
    receivedCommands.erase(receivedCommands.begin());
    return msg;
}

bool ATCommanderScheduler::getOldestMessageWithTimeout(const uint32_t &miliSec, std::string &msg)
{
    SPDLOG_TRACE("getOldestMessageWithTimeout");
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);
    if(receivedCommands.size() > 0)
    {
        msg = receivedCommands.front().command;
        receivedCommands.erase(receivedCommands.begin());
        SPDLOG_DEBUG("\"{}\" - last message", msg);
        return true;
    }
    SPDLOG_TRACE("wait for new AT message");
    isNewMsgFromAt = false;
    cvATReceiver.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
    if(!isNewMsgFromAt)
    {
        SPDLOG_ERROR("wait for the oldest message timeout: {}ms", miliSec);
        return false;
    }
    isNewMsgFromAt = false;
    heartBeatRefresh();
    SPDLOG_TRACE("new message was arrived");

    if(receivedCommands.size() > 0)
    {
        msg = receivedCommands.front().command;
        receivedCommands.erase(receivedCommands.begin());
        SPDLOG_DEBUG("\"{}\" - new message", msg);
        return true;
    }
    return false;
}

bool ATCommanderScheduler::waitForMessage(const std::string &msg,
                                          const std::chrono::steady_clock::time_point &timePoint)
{
    return waitForMessageTimeout(msg, timePoint, k_waitForMessageTimeout);
}

bool ATCommanderScheduler::waitForConfirm(const std::string &msg,
                                          const std::chrono::steady_clock::time_point &timePoint)
{
    return waitForMessageTimeout(msg, timePoint, k_waitForConfirmTimeout);
}

bool ATCommanderScheduler::waitForSyncConfirm(const std::string &msg)
{
    SPDLOG_TRACE("waitForSyncConfirm: msg: {}", msg);
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);

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
    cvATReceiver.wait_for(lk, std::chrono::milliseconds(k_waitForConfirmTimeout), [this]() { return isNewMsgFromAt; });
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

bool ATCommanderScheduler::waitForMessageTimeout(const std::string &msg,
                                                 const std::chrono::steady_clock::time_point &timePoint,
                                                 const uint32_t &miliSec)
{
    SPDLOG_TRACE("waitForLastMessageTimeout: msg: {}, timeout: {}ms", msg, miliSec);
    std::unique_lock<std::mutex> lk(receivedCommandsMutex);

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
        auto numberOfMsg = receivedCommands.size();
        isNewMsgFromAt = false;
        cvATReceiver.wait_for(lk, std::chrono::milliseconds(miliSec), [this]() { return isNewMsgFromAt; });
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
                             [&msg](ATResponse atReponse) { return atReponse.command.find(msg) != std::string::npos; });

        if(result != receivedCommands.end())
        {
            SPDLOG_DEBUG("\"{}\" new msg was confirmed", msg);
            receivedCommands.erase(result);
            return true;
        }
        else
        {
            SPDLOG_TRACE("New message is still not as expected");
        }

        endPt = std::chrono::steady_clock::now();
    }
    SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
    return false;
}

void ATCommanderScheduler::atCommandManager()
{
    if(!setConfigATE0())
    {
        SPDLOG_ERROR("failed to set ATE0");
        std::exit(0);
    }
    heartBeatRefresh();
    while(atCommandManagerIsRunning.load())
    {
        // Requests, status etc from GSM
        while(!receivedCommands.empty())
        {
            std::string msg = getOldestMessage();
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
        if(atRequestsQueue.size() > 0 && receivedCommands.empty())
        {
            while(atRequestsQueue.size() > 0)
            {
                configProcessing();
            }
            atRequestCv.notify_one();
        }

        // Request SMS to GSM
        if(atSmsRequestQueue.size() > 0 && receivedCommands.empty())
        {
            while(atSmsRequestQueue.size() > 0)
            {
                smsRequestProcessing();
            }
            atSmsRequestCv.notify_one();
        }

        if(receivedCommands.empty())
        {
            heartBeatTick();
        }


        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    SPDLOG_DEBUG("AT comnand manager thread closed");
}

void ATCommanderScheduler::smsProcessing(const std::string &msg)
{
    const auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
    auto splitted = utils::split(msgWithoutCRLF, ",,");

    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

    auto number = utils::split(splitted[0], " ")[1];
    Sms sms;
    sms.number = number;
    auto date = splitted[1];
    sms.dateAndTime = date;
    SPDLOG_INFO("new SMS info: {} {}", date, number);

    // get next message from the queue (text of SMS)
    std::string msgSms;

    bool result = getOldestMessageWithTimeout(k_waitForConfirmTimeout, msgSms);
    if(!result)
    {
        SPDLOG_ERROR("Failed to get SMS message");
        return;
    }
    SPDLOG_INFO("new SMS text: {}", msgSms);
    sms.msg = msgSms.substr(0, msgSms.size() - 2);
    {
        std::lock_guard<std::mutex> lc(smsMutex);
        receivedSmses.push(std::move(sms));
    }
}

void ATCommanderScheduler::callingProcessing(const std::string &msg)
{
    // +CLIP: "+48791942336",145,,,"",0
    auto splitted = utils::split(msg, ": ");
    auto callInfo = splitted[1];
    auto splitted2 = utils::split(callInfo, ",");
    auto number = splitted2[0].substr(1, splitted2[0].length() - 2);
    SPDLOG_INFO("Calling from {} !!! ", number);

    ATRequest request = ATRequest();
    request.request = "ATH";
    request.responsexpected.push_back("NO CARRIER");
    request.responsexpected.push_back("OK");
    {
        std::lock_guard lock(atRequestsMutex);
        atRequestsQueue.push(request);
    }
    calls.push(Call(number));
}

void ATCommanderScheduler::configProcessing()
{
    ATRequest lastTask;
    {
        std::lock_guard lock(atRequestsMutex);
        lastTask = atRequestsQueue.front();
        atRequestsQueue.pop();
    }
    SPDLOG_TRACE("AT request: {}", lastTask.request);
    auto now = std::chrono::steady_clock::now();
    serial.sendMessage(lastTask.request);
    auto expectedResponses = lastTask.responsexpected;
    for(const auto &expect : expectedResponses)
    {
        if(!waitForConfirm(expect, now))
        {
            SPDLOG_ERROR("Expected msg was not arrived: {}", expect);
            SPDLOG_ERROR("Failed to set config {}", lastTask.request);
        }
    }
}

void ATCommanderScheduler::smsRequestProcessing()
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

    auto now = std::chrono::steady_clock::now();
    serial.sendMessage(command);
    if(!waitForMessage(">", now))
    {
        SPDLOG_ERROR("msg:> was not arrived");
        return;
    }
    now = std::chrono::steady_clock::now();
    serial.sendMessage(sms.message);
    serial.sendChar(SUB);

    if(!waitForMessage(SMS_REQUEST, now))
    {
        SPDLOG_ERROR("msg:{} was not arrived", SMS_REQUEST);
        return;
    }

    if(!waitForConfirm("OK", now))
    {
        SPDLOG_ERROR("msg:OK was not arrived");
        return;
    }

    SPDLOG_INFO("message \"{}\" was send to {}", sms.message, sms.number);
}

void ATCommanderScheduler::heartBeatRefresh()
{
    lastRefresh = std::chrono::steady_clock::now();
}

void ATCommanderScheduler::heartBeatTick()
{
    if((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastRefresh).count()) > 10)
    {
        SPDLOG_TRACE("TIMEOUT");

        if(!sendSync())
        {
            SPDLOG_ERROR("Critical issue !!!");
            std::exit(0);
        }

        heartBeatRefresh();
    }
}

ATCommanderScheduler::~ATCommanderScheduler()
{
    atCommandManagerIsRunning.store(false);
    atThread->join();
}
}// namespace AT
