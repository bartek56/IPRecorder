#ifndef ATCOMMANDERSCHEDULER_HPP
#define ATCOMMANDERSCHEDULER_HPP

#include "Serial.hpp"
#include <queue>

struct ATRequest
{
    std::string request;
    std::vector<std::string> responsexpected;
};

struct SmsRequest
{
    SmsRequest()
    {
    }
    SmsRequest(std::string num, std::string msg) : number(num), message(msg)
    {
    }
    std::string number;
    std::string message;
};

struct Sms
{
    Sms(const std::string &number, const std::string &msg) : number(number), dateAndTime(""), msg(msg)
    {
        if(number.find("+48") == std::string::npos)
            throw std::runtime_error("number doesn't contain polish national prefix");
    }
    Sms() : number(""), dateAndTime(""), msg("")
    {
    }

    std::string number;
    std::string dateAndTime;
    std::string msg;
};


class ATCommanderScheduler
{
public:
    ATCommanderScheduler(const std::string &port);
    ~ATCommanderScheduler();

protected:
    // requests AT command
    std::mutex atRequestsMutex;
    std::queue<ATRequest> atRequestsQueue;    
    std::condition_variable atRequestCv;

    // SMS requests
    std::mutex atSmsRequestMutex;
    std::queue<SmsRequest> atSmsRequestQueue;
    std::condition_variable atSmsRequestCv;

    // received SMS
    std::queue<Sms> receivedSmses;
    std::mutex smsMutex;
private:
    bool setConfigATE0();
    bool sendSync();

    bool getLastMessageWithTimeout(const uint32_t &miliSec, std::string &msg);
    std::string getOldestMessage();
    bool waitForMessage(const std::string &msg);
    bool waitForConfirm(const std::string &msg);
    bool waitForMessageTimeout(const std::string &msg, const uint32_t &sec);
    void heartBeatRefresh();

    std::vector<std::string> split(std::string &s, const std::string &delimiter);

    // TODO callling

    // --------------------------------------------
    Serial serial;
    bool isNewMsgFromAt=false;

    // received AT command
    std::vector<std::string> receivedCommands;
    std::mutex receivedCommandsMutex;
    std::condition_variable cvATReceiver;

    // AT command thread
    std::unique_ptr<std::thread> atThread;
    void atCommandManager();
    std::atomic<bool> atCommandManagerIsRunning;

    std::chrono::steady_clock::time_point lastRefresh;
};

#endif// ATCOMMANDERSCHEDULER_HPP
