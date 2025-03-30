#include "ATCommanderScheduler.hpp"
#include "ATConfig.hpp"
#include "Utils.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include "ATHandlers.hpp"


namespace AT
{
ATCommanderScheduler::ATCommanderScheduler(std::string_view port) : serial(port), atCommandManagerIsRunning(false)
{
    receivedCommands.reserve(maxReceivedCommands);

    atRequestHandlerMap.insert(
            {{AT_Command::SMS_RECEIVING_INFO,
              std::bind(&ATHandlers::SMSReceivingInfoHandler, handlers, std::ref(statesSwitching), std::ref(data),
                        std::placeholders::_1)},
             {AT_Command::SMS_SENDING_CONFIRM,
              std::bind(&ATHandlers::SMSSendingConfirmHandler, handlers, std::ref(statesSwitching), std::ref(data),
                        std::placeholders::_1)},
             {AT_Command::SMS_RECEIVING_TEXT,
              std::bind(&ATHandlers::SMSReceivingTextHandler, handlers, std::ref(statesSwitching), std::ref(data),
                        std::placeholders::_1)},
             {AT_Command::CALLING, std::bind(&ATHandlers::CallingHandler, handlers, std::ref(statesSwitching),
                                             std::ref(data), std::placeholders::_1)},
             {AT_Command::RING, std::bind(&ATHandlers::RingHandler, handlers, std::ref(statesSwitching), std::ref(data),
                                          std::placeholders::_1)}});

    serial.setReadEvent(
            [&](const std::string &msg)
            {
                SPDLOG_TRACE("new AT message: {}", msg);
                const std::lock_guard<std::mutex> lockReceivedCommads(receivedCommandsMutex);
                isNewMsgFromAt = true;
                receivedCommands.emplace_back(std::chrono::steady_clock::now(), msg);
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
    {
        SPDLOG_ERROR("Error 1");
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
            SPDLOG_ERROR("Error 2");
            return false;
        }

        if(lastMessage.find("OK") != std::string::npos)
        {
            return true;
        }
    }

    SPDLOG_ERROR("setConfig ATE0 failed!");
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
    cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(miliSec),
                          [this]() { return isNewMsgFromAt; });
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
    const std::lock_guard lockReceivedCommands(receivedCommandsMutex);
    auto receivedCommand = receivedCommands.front();
    auto msg = receivedCommand.command;
    receivedCommands.erase(receivedCommands.begin());
    return msg;
}

bool ATCommanderScheduler::getOldestMessageWithTimeout(const uint32_t &miliSec, std::string &msg)
{
    SPDLOG_TRACE("getOldestMessageWithTimeout");
    std::unique_lock<std::mutex> lockReceivedCommands(receivedCommandsMutex);
    if(!receivedCommands.empty())
    {
        msg = receivedCommands.front().command;
        receivedCommands.erase(receivedCommands.begin());
        SPDLOG_DEBUG("\"{}\" - last message", msg);
        return true;
    }
    SPDLOG_TRACE("wait for new AT message");
    isNewMsgFromAt = false;
    cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(miliSec),
                          [this]() { return isNewMsgFromAt; });
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
        receivedCommands.erase(receivedCommands.begin());
        SPDLOG_DEBUG("\"{}\" - new message", msg);
        return true;
    }
    return false;
}

bool ATCommanderScheduler::waitForMessage(std::string_view msg, const std::chrono::steady_clock::time_point &timePoint)
{
    return waitForMessageTimeout(msg, timePoint, k_waitForMessageTimeout);
}

bool ATCommanderScheduler::waitForConfirm(std::string_view msg, const std::chrono::steady_clock::time_point &timePoint)
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
    cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(k_waitForConfirmTimeout),
                          [this]() { return isNewMsgFromAt; });
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
        cvATReceiver.wait_for(lockReceivedCommands, std::chrono::milliseconds(miliSec),
                              [this]() { return isNewMsgFromAt; });
        if(!isNewMsgFromAt)
        {
            SPDLOG_ERROR("wait for AT message: {} timeout: {}ms", msg, miliSec);
            return false;
        }
        isNewMsgFromAt = false;
        heartBeatRefresh();
        SPDLOG_TRACE("new message was arrived");

        auto result = std::find_if(receivedCommands.begin() + numberOfMsg, receivedCommands.end(),
                                   [&msg](const ATResponse &atReponse)
                                   { return atReponse.command.find(msg) != std::string::npos; });

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

AT_Command ATCommanderScheduler::translateCommand(const std::string &command)
{
    for(const auto &it : atResponsesDict)
    {
        if(command.find(it.second) != std::string::npos)
        {
            return it.first;
        }
    }
    return AT_Command::UNKNOWN;
}


void ATCommanderScheduler::atCommandManager()
{
    if(!setConfigATE0())
    {
        SPDLOG_ERROR("failed to set ATE0");
        std::exit(0);
    }
    statesSwitching.changeState(State::IDLE);
    heartBeatRefresh();
    while(atCommandManagerIsRunning.load())
    {
        // Requests, status etc from GSM
        while(!receivedCommands.empty())
        {
            auto msg = getOldestMessage();
            SPDLOG_DEBUG("AT response/msg from receivedCommands: {}", msg);
            auto typeOfRequest = translateCommand(msg);

            auto supportedCommandsInCurrentState = atCommandOfState[statesSwitching.getState()];

            auto supportedCommandResult =
                    std::find_if(supportedCommandsInCurrentState.begin(), supportedCommandsInCurrentState.end(),
                                 [&typeOfRequest](AT_Command command) { return command == typeOfRequest; });

            if(statesSwitching.getState() != State::IDLE and
               supportedCommandResult == supportedCommandsInCurrentState.end())
            {
                SPDLOG_WARN("Command '{}' is not supported on the current state: {}", msg,
                            statesSwitching.getStateStr());
                if(typeOfRequest != AT_Command::UNKNOWN)
                {
                    requestsQueue.push(msg);
                }
                else
                {
                    SPDLOG_ERROR("AT command: '{}' is unknown and ncannot be added to queue", msg);
                }
                continue;
            }


            if((statesSwitching.getState() == State::SMS_RECEIVING) and (typeOfRequest == AT_Command::UNKNOWN))
            {
                atRequestHandlerMap[AT_Command::SMS_RECEIVING_TEXT](msg);
            }

            if(typeOfRequest == AT_Command::OK)
            {
                if(statesSwitching.getState() == State::SMS_SENDING_OK)
                {
                    /// OK confirm on the SMS STATE
                    statesSwitching.changeState(State::IDLE);
                    continue;
                }
                else
                {
                    SPDLOG_WARN("New message 'OK' was received, but I don't know what confirm");
                }
            }

            if(typeOfRequest == AT_Command::UNKNOWN)
            {
                SPDLOG_WARN("Unknown type of message! - {}", msg);
                continue;
            }

            auto result = atRequestHandlerMap[typeOfRequest](msg);
            if(result == ResultState::SMS_RECEIVED)
            {
                const std::lock_guard<std::mutex> lockSmsMutex(smsMutex);
                receivedSmses.push(data.smsReceiving);
            }
        }

        if(statesSwitching.getState() == State::IDLE and !requestsQueue.empty())
        {
            auto lastMsg = requestsQueue.front();
            requestsQueue.pop();
            auto typeOfRequest = translateCommand(lastMsg);
            atRequestHandlerMap[typeOfRequest](lastMsg);
        }
        // Request config to GSM
        if(statesSwitching.getState() == State::IDLE && receivedCommands.empty() && !atRequestsQueue.empty())
        {
            while(!atRequestsQueue.empty())
            {
                // state config
                configProcessing();
            }
            atRequestCv.notify_one();
        }

        // Request SMS to GSM
        if(statesSwitching.getState() == State::IDLE && !atSmsRequestQueue.empty() && receivedCommands.empty())
        {
            // state SMS request
            statesSwitching.changeState(State::SMS_SENDING);
            smsRequestProcessing();
            /// TODO sendSyncMsg
            //atSmsRequestCv.notify_one();
        }

        if(receivedCommands.empty() && atSmsRequestQueue.empty())
        {
            /// TODO just send AT message and change state to SYNC
            /// do not use waitForMessage
            heartBeatTick();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    SPDLOG_DEBUG("AT comnand manager thread closed");
}

void ATCommanderScheduler::configProcessing()
{
    //    statesSwitching.changeState()
    ATRequest lastTask;
    {
        const std::lock_guard lockRequestsMutex(atRequestsMutex);
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
        const std::lock_guard lockRequestsMutex(atSmsRequestMutex);
        sms = atSmsRequestQueue.front();
        atSmsRequestQueue.pop();
    }
    data.smsSending.msg = sms.message;
    data.smsSending.number = sms.number;
    SPDLOG_DEBUG("Sending SMS: \"{}\" to {}", sms.message, sms.number);
    /// TODO move it to ATHandlers
    const std::string sign = "=\"";
    const std::string command = std::string(AT_SMS_REQUEST) + sign + sms.number + "\"";

    auto now = std::chrono::steady_clock::now();
    serial.sendMessage(command);
    if(!waitForMessage(SMS_INPUT, now))
    {
        SPDLOG_ERROR("msg: '>' was not arrived");
        return;
    }
    now = std::chrono::steady_clock::now();
    serial.sendMessage(sms.message);
    serial.sendChar(SUB);
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


void StatesSwitching::changeState(const State newState)
{
    SPDLOG_DEBUG("Change state from {} to {}", stateStr[state], stateStr[newState]);
    state = newState;
}

State StatesSwitching::getState()
{
    return state;
}

std::string StatesSwitching::getStateStr()
{
    return stateStr[state];
}


}// namespace AT
