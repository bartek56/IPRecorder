#include "ATCommander.hpp"
#include "spdlog/spdlog.h"
#include "ATConfig.hpp"

ATCommander::ATCommander(const std::string &port) : ATCommanderScheduler(port)
{
}

bool ATCommander::setConfig(const std::string &command)
{
    SPDLOG_DEBUG("setConfig");
    ATRequest request = ATRequest();
    request.request = command + EOL;
    request.responsexpected.push_back("OK");

    std::unique_lock lk(atRequestsMutex);
    atRequestsQueue.push(request);

    atRequestCv.wait_for(lk, std::chrono::milliseconds(k_waitForMessageTimeout),
                         [this]() { return atRequestsQueue.empty(); });
    if(!atRequestsQueue.empty())
    {
        SPDLOG_ERROR("wait for setConfig timeout!");
        return false;
    }
    return true;
}

bool ATCommander::sendSms(const SmsRequest &sms)
{
    SPDLOG_DEBUG("add SMS to queue: Text: \"{}\" number: {}", sms.message, sms.number);
    std::lock_guard lock(atSmsRequestMutex);
    atSmsRequestQueue.push(sms);
    return true;
}

bool ATCommander::sendSmsSync(const SmsRequest &sms)
{
    SPDLOG_DEBUG("sending SMS: Text: \"{}\" number: {}", sms.message, sms.number);
    std::unique_lock lk(atSmsRequestMutex);
    atSmsRequestQueue.push(sms);
    atSmsRequestCv.wait_for(lk, std::chrono::milliseconds(k_waitForMessageTimeout),
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
    std::lock_guard<std::mutex> lc(smsMutex);
    return !receivedSmses.empty();
}

Sms ATCommander::getLastSms()
{
    std::lock_guard<std::mutex> lc(smsMutex);
    auto lastSms = receivedSmses.front();
    receivedSmses.pop();
    return lastSms;
}

bool ATCommander::isNewCall()
{
    std::lock_guard<std::mutex> lc(callsMutex);
    return !calls.empty();
}

Call ATCommander::getLastCall()
{
    std::lock_guard<std::mutex> lc(callsMutex);
    auto lastCall = calls.front();
    calls.pop();
    return lastCall;
}
