#include "GSMManager.hpp"
#include "spdlog/spdlog.h"
#include "ATConfig.hpp"

#include <iostream>

GSMManager::GSMManager(const std::string &port)
    : atCommander(port, receivedSmses, smsMutex), tasks(
                                                          [&](const SmsRequest &sms)
                                                          {
                                                              SPDLOG_TRACE("task is calling!!. number:{}, message:{} ", sms.number, sms.message);
                                                              atCommander.sendSms(sms);
                                                              return true;
                                                          })
{
}

bool GSMManager::initilize()
{
    if(!setDefaultConfig())
    {
        SPDLOG_ERROR("Failed to set default configuration");
        return false;
    }
    return true;
}

bool GSMManager::sendSms(const std::string &number, const std::string &message)
{
    return tasks.addTask(SmsRequest(number, message));
}

bool GSMManager::sendSmsSync(const std::string &number, const std::string &message)
{
    return tasks.callTask(SmsRequest(number, message));
}

bool GSMManager::isNewSms()
{
    std::lock_guard<std::mutex> lc(smsMutex);
    return !receivedSmses.empty();
}

Sms GSMManager::getSms()
{
    std::lock_guard<std::mutex> lc(smsMutex);
    auto lastSms = receivedSmses.front();
    receivedSmses.pop();
    return lastSms;
}

bool GSMManager::setDefaultConfig()
{
    auto setConfig = [&](const std::string &command)
    {
        auto result = atCommander.setConfig(command);
        if(!result)
        {
            SPDLOG_ERROR("Failed to set config: {}", command);
            return false;
        }
        return true;
    };

    bool result = true;
    for(const auto &it : k_defaultConfig)
    {
        result &= setConfig(it);
    }

    return result;
}
