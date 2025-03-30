#ifndef ATCOMMANDERSCHEDULER_HPP
#define ATCOMMANDERSCHEDULER_HPP

#include "Serial.hpp"
#include "ATTypes.hpp"
#include "ATConfig.hpp"
#include "ATHandlers.hpp"

#include <queue>

namespace AT
{


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
    bool waitForMessage(std::string_view msg, const std::chrono::steady_clock::time_point &timePoint);
    bool waitForConfirm(std::string_view msg, const std::chrono::steady_clock::time_point &timePoint);
    bool waitForSyncConfirm(const std::string &msg);
    /// TODO refactor it/ printLog and timePoint should be get from another place - constexpr if
    bool waitForMessageTimeout(std::string_view msg, const std::chrono::steady_clock::time_point &timePoint,
                               const uint32_t &sec);

    AT_Command translateCommand(const std::string &command);


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
    void configProcessing();
    void smsRequestProcessing();

    // Heart beat
    std::chrono::steady_clock::time_point lastRefresh;
    void heartBeatRefresh();
    void heartBeatTick();

    ATHandlers handlers;
    StatesSwitching statesSwitching;
    SchedulerData data;

    std::unordered_map<AT_Command, std::string_view> atResponsesDict{
            {AT_Command::SMS_RECEIVING_INFO, SMS_RESPONSE},
            {AT_Command::SMS_SENDING_CONFIRM, SMS_SENDING_CONFIRM},
            {AT_Command::CALLING, CALLING},
            {AT_Command::RING, RING},
            {AT_Command::OK, OK},
    };

    std::unordered_map<State, std::vector<AT_Command>> atCommandOfState{
            {State::SMS_RECEIVING, {AT_Command::SMS_RECEIVING_INFO}},
            {State::SMS_RECEIVING_CONFIRM, {AT_Command::SMS_RECEIVING_INFO}},
            {State::SMS_SENDING_CONFIRM, {AT_Command::OK}},
            {State::SMS_SENDING, {AT_Command::SMS_SENDING_CONFIRM}},
            {State::SMS_SENDING_OK, {AT_Command::OK}},
    };

    std::unordered_map<AT_Command, std::function<ResultState(const std::string &)>> atRequestHandlerMap;
    std::queue<std::string> requestsQueue;
};


}// namespace AT

#endif// ATCOMMANDERSCHEDULER_HPP
