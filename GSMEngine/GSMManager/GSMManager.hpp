#ifndef GSMMANAGER_HPP
#define GSMMANAGER_HPP

#include "GSMTasks.hpp"

#include <string>

class GSMManager
{
public:
    GSMManager(const std::string& port);
    bool Initilize();
    bool SendSms(const std::string& message, const int& number);
    bool isNewSms();
    Sms getLastSms();
private:
    GSMTasks tasks;
};

#endif // GSMMANAGER_HPP
