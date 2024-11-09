#pragma once
//#ifndef ATCOMMANDER_HPP
//#define ATCOMMANDER_HPP

#include "ATCommanderScheduler.hpp"

#include <string_view>

namespace AT
{
    class ATCommander : public ATCommanderScheduler
    {
    public:
        explicit ATCommander(std::string_view port) noexcept;

        bool setConfig(std::string_view command);
        bool sendSms(const SmsRequest &sms);
        bool sendSmsSync(const SmsRequest &sms);
        bool isNewSms();
        Sms getLastSms();
        bool isNewCall();
        Call getLastCall();
    };
}// namespace AT
//#endif// ATCOMMANDER_HPP
