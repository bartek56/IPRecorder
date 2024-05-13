#include "GSMManager.hpp"

#include <iostream>

GSMManager::GSMManager(const std::string &port) : atCommander(port, receivedSmses, smsMutex)
{
}

bool GSMManager::initilize()
{
    if(!setDefaultConfig())
    {
        std::cout << "Failed to set default configuration" << std::endl;
        return false;
    }
    tasks.init();
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
            std::cout << "failed to set config: " << command << std::endl;
            return false;
        }
        return true;
    };

    bool result = true;
    result &= setConfig("AT");
    result &= setConfig("AT+CMGF=1");
    result &= setConfig("AT+CNMI=1,2,0,0");
    result &= setConfig("AT+CLIP=1");

    return result;
}
