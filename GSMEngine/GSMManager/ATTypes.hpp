#ifndef ATTYPES_HPP
#define ATTYPES_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <unordered_map>

namespace AT
{
enum class State : uint8_t
{
    IDLE = 0,
    SMS_RECEIVING,
    SMS_RECEIVING_CONFIRM,
    SMS_SENDING,
    SMS_SENDING_CONFIRM,
    SMS_SENDING_OK,
    CALL,
    SYNC,
    UNKNOWN
};


enum class AT_Command : uint8_t
{
    SMS_RECEIVING_INFO = 0,
    SMS_RECEIVING_TEXT,
    SMS_SENDING_CONFIRM,
    CALLING,
    RING,
    OK,
    UNKNOWN
};

enum class ResultState : uint8_t
{
    NONE = 0,
    SMS_RECEIVED,
    CALLING,
    WAIT_FOR_OK
};


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


class StatesSwitching
{
public:
    void changeState(State state);
    State getState();
    std::string getStateStr();

private:
    State state{};
    std::unordered_map<State, std::string> stateStr{{State::IDLE, "IDLE"},
                                                    {State::SMS_RECEIVING, "SMS_RECEIVING"},
                                                    {State::SMS_RECEIVING_CONFIRM, "SMS_RECEIVING_CONFIRM"},
                                                    {State::SMS_SENDING, "SMS_SENDING"},
                                                    {State::SMS_SENDING_CONFIRM, "SMS_SENDING_CONFIRM"},
                                                    {State::SMS_SENDING_OK, "SMS_SENDING_OK"},
                                                    {State::CALL, "CALL"},
                                                    {State::SYNC, "SYNC"},
                                                    {State::UNKNOWN, "UNKNOWN"}};
};


struct SchedulerData
{
    Sms smsReceiving;
    Sms smsSending;
};
}// namespace AT


#endif// ATTYPES_HPP
