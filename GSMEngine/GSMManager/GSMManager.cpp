#include "ATCommanderScheduler.hpp"
#include "ATConfig.hpp"
#include "GSMManager.hpp"
#include "spdlog/spdlog.h"
#include <string>
#include <string_view>

GSMManager::GSMManager(std::string_view port) : atCommander(port)
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
    return atCommander.sendSms(SmsRequest(number, message));
}

bool GSMManager::sendSmsSync(const std::string &number, const std::string &message)
{
    return atCommander.sendSmsSync(SmsRequest(number, message));
}

std::optional<Sms> GSMManager::getSms()
{
    return atCommander.getLastSms();
}

std::optional<Call> GSMManager::getCall()
{
    return atCommander.getLastCall();
}

void GSMManager::shutdown()
{
    atCommander.shutdown();
}

bool GSMManager::isAlive() const
{
    return atCommander.isAlive();
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
    for(const auto &config : k_defaultConfig)
    {
        result &= setConfig(config);
        if (result == false)
        {
            SPDLOG_ERROR("Failed to set {} ", config);
            break;
        }
    }

    return result;
}
