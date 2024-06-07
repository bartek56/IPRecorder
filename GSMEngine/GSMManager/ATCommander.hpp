#ifndef ATCOMMANDER_HPP
#define ATCOMMANDER_HPP

#include <string>
#include <queue>
#include "Serial.hpp"

struct Sms
{
    Sms(const std::string& number, const std::string& msg) : number(number), dateAndTime(""), msg(msg)
    {
        if(number.find("+48") == std::string::npos)
            throw std::runtime_error("number doesn't contain polish national prefix");
    }
    Sms() : number(""), dateAndTime(""),msg("")
    {
    }

    std::string number;
    std::string dateAndTime;
    std::string msg;
};

struct SmsRequest
{
    SmsRequest(std::string num, std::string msg):
        number(num), message(msg)
    {
    }
    std::string number;
    std::string message;
};

class ATCommander
{
public:
    explicit ATCommander(const std::string &port, std::queue<Sms>& receivedSms, std::mutex& smsMutex);

    bool sendSms(const SmsRequest& sms);
    bool setConfig(const std::string& command);
private:
    Serial serial;
    std::queue<Sms>& receivedSmses;
    std::mutex& smsMutex;

    std::mutex receivedCommandsMutex;
    std::queue<std::string> receivedCommands;

    bool isNewMessage=false;
    std::condition_variable cv;
    bool waitForMessage(const std::string& msg);
    bool waitForConfirm(const std::string& msg);
    bool waitForMessageTimeout(const std::string& msg, const uint32_t& sec);
};

#endif // ATCOMMANDER_HPP
