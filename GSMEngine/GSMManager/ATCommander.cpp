#include "ATCommander.hpp"
#include "ATCommanderScheduler.hpp"
#include "ATConfig.hpp"
#include "spdlog/spdlog.h"
#include <chrono>
#include <mutex>
#include <string_view>

namespace AT
{
ATCommander::ATCommander(std::string_view port) noexcept : ATCommanderScheduler(port)
{
}

bool ATCommander::setConfig(std::string_view command)
{
    SPDLOG_DEBUG("setConfig: {}", command);
    ATRequest request = ATRequest();
    request.request = command;
    request.responsexpected.emplace_back("OK");

    std::unique_lock lockRequestsQueue(atRequestsMutex);
    atRequestsQueue.push(request);

    atRequestCv.wait_for(lockRequestsQueue, std::chrono::milliseconds(k_waitForMessageTimeout),
                         [this]() { return atRequestsQueue.empty(); });
    if(!atRequestsQueue.empty())
    {
        SPDLOG_ERROR("wait for setConfig timeout!");
        return false;
    }
    SPDLOG_DEBUG("Setting \"{}\" was successful", command);
    return true;
}

bool ATCommander::sendSms(const SmsRequest &sms)
{
    SPDLOG_DEBUG("add SMS to queue: Text: \"{}\" number: {}", sms.message, sms.number);
    const std::lock_guard lock(atSmsRequestMutex);
    atSmsRequestQueue.push(sms);
    return true;
}

bool ATCommander::sendSmsSync(const SmsRequest &sms)
{
    SPDLOG_DEBUG("sending SMS: Text: \"{}\" number: {}", sms.message, sms.number);
    std::unique_lock lockRequestsQueue(atSmsRequestMutex);
    atSmsRequestQueue.push(sms);
    atSmsRequestCv.wait_for(lockRequestsQueue, std::chrono::milliseconds(k_waitForMessageTimeout),
                            [this]() { return atSmsRequestQueue.empty(); });
    if(!atSmsRequestQueue.empty())
    {
        SPDLOG_ERROR("wait for sending SMS timeout!");
        return false;
    }
    return true;
}

bool ATCommander::isNewSms()
{
    const std::lock_guard<std::mutex> lockReceivedSmses(smsMutex);
    return !receivedSmses.empty();
}

Sms ATCommander::getLastSms()
{
    const std::lock_guard<std::mutex> lockReceivedSmses(smsMutex);
    auto lastSms = receivedSmses.front();
    receivedSmses.pop();
    return lastSms;
}

bool ATCommander::isNewCall()
{
    const std::lock_guard<std::mutex> lockCalls(callsMutex);
    return !calls.empty();
}

Call ATCommander::getLastCall()
{
    const std::lock_guard<std::mutex> lockCalls(callsMutex);
    auto lastCall = calls.front();
    calls.pop();
    return lastCall;
}
}// namespace AT
