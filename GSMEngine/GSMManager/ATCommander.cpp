#include "ATCommander.hpp"
#include "ATCommanderScheduler.hpp"
#include "ATConfig.hpp"
#include "spdlog/spdlog.h"
#include <chrono>
#include <mutex>
#include <string_view>
#include <utility>

namespace AT
{
ATCommander::ATCommander(std::string_view port) : ATCommanderScheduler(port)
{
}

bool ATCommander::setConfig(std::string_view command)
{
    SPDLOG_DEBUG("setConfig: {}", command);
    ATRequest request = ATRequest();
    request.request = command;
    request.responsexpected.emplace_back("OK");
    auto completion = std::make_shared<std::promise<bool>>();
    auto result = completion->get_future();

    {
        const std::lock_guard lockRequestsQueue(atRequestsMutex);
        atRequestsQueue.push(ATRequestTask{std::move(request), completion});
    }

    if(result.wait_for(std::chrono::milliseconds(k_waitForMessageTimeout)) != std::future_status::ready)
    {
        SPDLOG_ERROR("wait for setConfig timeout!");
        return false;
    }

    const bool success = result.get();
    if(success)
        SPDLOG_DEBUG("Setting \"{}\" was successful", command);
    return success;
}

bool ATCommander::sendSms(const SmsRequest &sms)
{
    SPDLOG_DEBUG("add SMS to queue: Text: \"{}\" number: {}", sms.message, sms.number);
    const std::lock_guard lock(atSmsRequestMutex);
    atSmsRequestQueue.push(SmsRequestTask{sms, nullptr});
    return true;
}

bool ATCommander::sendSmsSync(const SmsRequest &sms)
{
    SPDLOG_DEBUG("sending SMS: Text: \"{}\" number: {}", sms.message, sms.number);
    auto completion = std::make_shared<std::promise<bool>>();
    auto result = completion->get_future();
    {
        const std::lock_guard lockRequestsQueue(atSmsRequestMutex);
        atSmsRequestQueue.push(SmsRequestTask{sms, completion});
    }

    if(result.wait_for(std::chrono::milliseconds(k_waitForMessageTimeout)) != std::future_status::ready)
    {
        SPDLOG_ERROR("wait for sending SMS timeout!");
        return false;
    }
    return result.get();
}

std::optional<Sms> ATCommander::getLastSms()
{
    const std::lock_guard<std::mutex> lockReceivedSmses(smsMutex);
    if(receivedSmses.empty())
        return std::nullopt;

    auto lastSms = std::move(receivedSmses.front());
    receivedSmses.pop();
    return lastSms;
}

std::optional<Call> ATCommander::getLastCall()
{
    const std::lock_guard<std::mutex> lockCalls(callsMutex);
    if(calls.empty())
        return std::nullopt;

    auto lastCall = std::move(calls.front());
    calls.pop();
    return lastCall;
}
}// namespace AT
