#ifndef GSMMANAGER_HPP
#define GSMMANAGER_HPP

#include "GSMTasks.hpp"

#include <string>

class GSMManager
{
public:
    GSMManager(const std::string& port);
    bool initilize();
    bool sendSms(const std::string& number, const std::string& message);
    bool sendSmsSync(const std::string& number, const std::string& message);
    bool isNewSms();
    Sms getSms();
private:
    GSMTasks tasks;
};

#endif // GSMMANAGER_HPP
