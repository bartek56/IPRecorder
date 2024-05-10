#include "GSMManager.hpp"

#include <iostream>

GSMManager::GSMManager(const std::string &port) : tasks(port)
{
}

bool GSMManager::Initilize()
{
    auto setConfig = [&](const std::string &command)
    {
        auto result = tasks.setConfig(command);
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

bool GSMManager::SendSms(const std::string &message, const std::string &number)
{
    return tasks.sendSms(message, number);
}

bool GSMManager::isNewSms()
{
    return tasks.isNewSms();
}

Sms GSMManager::getLastSms()
{
    return tasks.getLastSms();
}
