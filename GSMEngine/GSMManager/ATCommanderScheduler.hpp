#ifndef ATCOMMANDERSCHEDULER_HPP
#define ATCOMMANDERSCHEDULER_HPP

#include "Serial.hpp"
#include <stdexcept>
#include <queue>
namespace AT
{
struct ATResponse
{
    ATResponse(std::chrono::steady_clock::time_point _timestamp, std::string _command)
        : timestamp(_timestamp), command(_command)
    {
    }
    std::chrono::steady_clock::time_point timestamp;
    std::string command;
};

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

struct Call
{
    Call(std::string _number) : number(_number)
    {
    }
    Call() : number("")
    {
    }

    // TODO time point of call std::chrono::system_clock::time_point timePoint;
    std::string number;
};

class ATCommanderScheduler
{
public:
    ATCommanderScheduler(std::string_view port);
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

    // calls
    std::queue<Call> calls;
    std::mutex callsMutex;

private:
    bool setConfigATE0();
    bool sendSync();

    bool getLastMessageWithTimeout(const uint32_t &miliSec, std::string &msg);
    std::string getOldestMessage();
    bool getOldestMessageWithTimeout(const uint32_t &miliSec, std::string &msg);
    bool waitForMessage(const std::string &msg, const std::chrono::steady_clock::time_point &timePoint);
    bool waitForConfirm(const std::string &msg, const std::chrono::steady_clock::time_point &timePoint);
    bool waitForSyncConfirm(const std::string &msg);
    /// TODO refactor it/ printLog and timePoint should be get from another place - constexpr if
    bool waitForMessageTimeout(const std::string &msg, const std::chrono::steady_clock::time_point &timePoint,
                               const uint32_t &sec);

    // --------------------------------------------
    Serial serial;
    bool isNewMsgFromAt = false;

    // received AT command
    static constexpr int maxReceivedCommands = 20;
    std::vector<ATResponse> receivedCommands;
    std::mutex receivedCommandsMutex;
    std::condition_variable cvATReceiver;

    // AT command thread
    std::unique_ptr<std::thread> atThread;
    void atCommandManager();
    std::atomic<bool> atCommandManagerIsRunning;
    void smsProcessing(const std::string &msg);
    void callingProcessing(const std::string &msg);
    void configProcessing();
    void smsRequestProcessing();

    // Heart beat
    std::chrono::steady_clock::time_point lastRefresh;
    void heartBeatRefresh();
    void heartBeatTick();
};
}// namespace AT

#endif// ATCOMMANDERSCHEDULER_HPP
