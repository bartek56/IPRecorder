#ifndef ATCOMMANDER_HPP
#define ATCOMMANDER_HPP

#include <string>
#include "ATCommanderScheduler.hpp"


class ATCommander : public ATCommanderScheduler
{
public:
    explicit ATCommander(std::string_view port) noexcept;

    bool setConfig(std::string_view command);
    bool sendSms(const SmsRequest& sms);
    bool sendSmsSync(const SmsRequest& sms);
    bool isNewSms();
    Sms getLastSms();
    bool isNewCall();
    Call getLastCall();
};

#endif // ATCOMMANDER_HPP
