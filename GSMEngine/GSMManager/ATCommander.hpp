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

struct ATRequest
{
    std::string request;
    std::vector<std::string> responsexpected;
};

class ATCommander
{
public:
    explicit ATCommander(const std::string &port, std::queue<Sms>& receivedSms, std::mutex& smsMutex);

    bool sendSms(const SmsRequest& sms);
    bool sendSmsSync(const SmsRequest& sms);
    bool setConfig(const std::string& command);
    ~ATCommander();
private:
    Serial serial;
    std::queue<Sms>& receivedSmses;
    std::mutex& smsMutex;

    std::mutex receivedCommandsMutex;
    std::queue<std::string> receivedCommands;

    std::condition_variable cv;

    std::vector<std::string> split(std::string &s, const std::string &delimiter);
    bool waitForMessage(const std::string& msg);
    bool waitForConfirm(const std::string& msg);
    bool waitForMessageTimeout(const std::string& msg, const uint32_t& sec);
    bool getMessageWithTimeout(const uint32_t &miliSec, std::string& msg);
    bool setConfigATE0();

    std::unique_ptr<std::thread> atThread;
    void atCommandManager();
    std::atomic<bool> atCommanManagerIsRunning;

    std::condition_variable atRequestsCv;
    std::mutex atRequestsMutex;
    std::queue<ATRequest> atRequestsQueue;

    std::mutex atSmsRequestMutex;
    std::queue<SmsRequest> atSmsRequestQueue;

    bool isNewSMS = false;
    std::chrono::time_point<std::chrono::steady_clock> newSmsTimestamp;
};

#endif // ATCOMMANDER_HPP
