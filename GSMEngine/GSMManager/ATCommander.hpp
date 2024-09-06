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
    SmsRequest()
    {
    }
    SmsRequest(std::string num, std::string msg):
        number(num), message(msg)
    {
    }
    std::string number;
    std::string message;
};

struct ATRequest
{
    std::string request;
    std::vector<std::string> responsexpected;
};

class ATCommander
{
public:
    explicit ATCommander(const std::string &port);

    bool setConfig(const std::string& command);
    bool sendSms(const SmsRequest& sms);
    bool sendSmsSync(const SmsRequest& sms);
    bool isNewSms();
    Sms getLastSms();
    ~ATCommander();
private:
    bool waitForMessage(const std::string& msg);
    bool waitForConfirm(const std::string& msg);
    bool waitForMessageTimeout(const std::string& msg, const uint32_t& sec);
    bool getMessageWithTimeout(const uint32_t &miliSec, std::string& msg);
    bool setConfigATE0();
    std::vector<std::string> split(std::string &s, const std::string &delimiter);

    Serial serial;

    // received SMS
    std::queue<Sms> receivedSmses;
    std::mutex smsMutex;

    // SMS requests
    std::mutex atSmsRequestMutex;
    std::queue<SmsRequest> atSmsRequestQueue;
    std::condition_variable cvSmsRequests;

    // TODO callling

    // --------------------------------------------

    // received AT command
    std::queue<std::string> receivedCommands;
    std::mutex receivedCommandsMutex;
    std::condition_variable cvATReceiver;

    // requests AT command
    std::mutex atRequestsMutex;
    std::queue<ATRequest> atRequestsQueue;

    // AT command thread
    std::unique_ptr<std::thread> atThread;
    void atCommandManager();
    std::atomic<bool> atCommanManagerIsRunning;
};

#endif // ATCOMMANDER_HPP
