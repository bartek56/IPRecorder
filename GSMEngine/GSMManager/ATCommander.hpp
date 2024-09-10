#ifndef ATCOMMANDER_HPP
#define ATCOMMANDER_HPP

#include <string>
#include <queue>
#include "ATCommanderScheduler.hpp"



class ATCommander : public ATCommanderScheduler
{
public:
    explicit ATCommander(const std::string &port);

    bool setConfig(const std::string& command);
    bool sendSms(const SmsRequest& sms);
    bool sendSmsSync(const SmsRequest& sms);
    bool isNewSms();
    Sms getLastSms();
};

#endif // ATCOMMANDER_HPP
