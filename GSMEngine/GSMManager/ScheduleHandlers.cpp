#include "ScheduleHandlers.hpp"
#include "Utils.hpp"
#include "spdlog/spdlog.h"


namespace AT
{

ScheduleHandlers::ScheduleHandlers()
{
}


void ScheduleHandlers::SMSInfoHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    const auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
    auto splitted = utils::split(msgWithoutCRLF, ",,");

    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

    auto number = utils::split(splitted[0], " ")[1];
    data.sms.number = number;
    auto date = splitted[1];
    data.sms.dateAndTime = date;
    SPDLOG_INFO("new SMS info: {} {}", date, number);
    switching.changeState(State::SMS_RECEIVING);
}

void ScheduleHandlers::SMSTextHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    SPDLOG_INFO("SMS text: {}", msg);

    data.sms.msg = msg.substr(0, msg.size() - 2);
    {
        const std::lock_guard<std::mutex> lockSmsMutex(smsMutex);
        receivedSmses.push(data.sms);
        data.sms = {};
    }
    /// TODO state SMS done
    switching.changeState(State::IDLE);
}


void ScheduleHandlers::CallingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    // +CLIP: "+48791942336",145,,,"",0
    auto splitted = utils::split(msg, ": ");
    auto callInfo = splitted[1];
    auto splitted2 = utils::split(callInfo, ",");
    auto number = splitted2[0].substr(1, splitted2[0].length() - 2);
    SPDLOG_INFO("Calling from {} !!! ", number);

    ATRequest request = ATRequest();
    request.request = "ATH";
    request.responsexpected.emplace_back("NO CARRIER");
    request.responsexpected.emplace_back("OK");
    {
        const std::lock_guard lockRequestsMutex(atRequestsMutex);
        atRequestsQueue.push(request);
    }
    calls.emplace(number);
}

void ScheduleHandlers::RingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
}


}// namespace AT
