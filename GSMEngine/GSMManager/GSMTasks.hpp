#ifndef GSMTASKS_HPP
#define GSMTASKS_HPP

#include <string>
#include <queue>
#include "Serial.hpp"


struct Sms
{
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


class GSMTasks
{
public:
    explicit GSMTasks(const std::string& port);
    GSMTasks(const GSMTasks&) = delete;
    GSMTasks& operator=(const GSMTasks&) = delete;
    GSMTasks(GSMTasks&&) = delete;
    GSMTasks& operator=(GSMTasks&&) = delete;
    ~GSMTasks();
    bool addSmsTask(const std::string& number, const std::string& message);
    bool sendSms(const std::string& number, const std::string& message);
    bool setConfig(const std::string& command);
    bool isNewSms();
    Sms getLastSms();
private:
    Serial serial;
    std::mutex receivedCommandsMutex;
    std::mutex smsMutex;
    std::condition_variable cv;
    bool isNewMessage;
    std::queue<std::string> receivedCommands;
    std::queue<Sms> receivedSmses;
    bool waitForMessage(const std::string& msg, const uint32_t& sec);

    std::atomic<bool> tasksRunning;
    std::unique_ptr<std::thread> tasksThread;
    std::queue<SmsRequest> tasks;
    std::condition_variable tasksCondition;
    std::mutex tasksMutex;
    bool isTaskInQueue;

    bool sendSmsSerial(const std::string& number, const std::string& message);
    void tasksFunc();
};

#endif // GSMTASKS_HPP
