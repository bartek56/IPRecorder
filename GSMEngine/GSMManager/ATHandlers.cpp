#include "ATHandlers.hpp"

#include "Utils.hpp"
#include "spdlog/spdlog.h"

namespace AT
{
ATHandlers::ATHandlers()
{
}

ResultState ATHandlers::SMSReceivingInfoHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    const auto msgWithoutCRLF = msg.substr(0, msg.size() - 2);
    auto splitted = utils::split(msgWithoutCRLF, ",,");

    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

    auto number = utils::split(splitted[0], " ")[1];
    data.smsReceiving.number = number;
    auto date = splitted[1];
    data.smsReceiving.dateAndTime = date;
    SPDLOG_INFO("new SMS info: {} {}", date, number);
    switching.changeState(State::SMS_RECEIVING);
    return ResultState::NONE;
}

ResultState ATHandlers::SMSSendingConfirmHandler(StatesSwitching &switching, SchedulerData &data,
                                                 const std::string &command)
{

    /*
    auto now = std::chrono::steady_clock::now();
    if(!waitForConfirm("OK", now))
    {
        SPDLOG_ERROR("msg: 'OK' was not arrived");
        return;
    }

    SPDLOG_INFO("message \"{}\" was send to {}", data.sms.msg, data.sms.number);
*/
    switching.changeState(State::SMS_SENDING_OK);
    return ResultState::NONE;
}

ResultState ATHandlers::SMSReceivingTextHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    SPDLOG_INFO("SMS text: {}", msg);
    data.smsReceiving.msg = msg.substr(0, msg.size() - 2);
    switching.changeState(State::IDLE);
    return ResultState::SMS_RECEIVED;
}


ResultState ATHandlers::CallingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    // +CLIP: "+48791942336",145,,,"",0
    auto splitted = utils::split(msg, ": ");
    auto callInfo = splitted[1];
    auto splitted2 = utils::split(callInfo, ",");
    auto number = splitted2[0].substr(1, splitted2[0].length() - 2);
    SPDLOG_INFO("Calling from {} !!! ", number);

    /// TODO inform python about calling
    /*
    ATRequest request = ATRequest();
    request.request = "ATH";
    request.responsexpected.emplace_back("NO CARRIER");
    request.responsexpected.emplace_back("OK");
    {
        const std::lock_guard lockRequestsMutex(atRequestsMutex);
        atRequestsQueue.push(request);
    }
    calls.emplace(number);
    */
    return ResultState::NONE;
}

ResultState ATHandlers::RingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &msg)
{
    return ResultState::NONE;
}

}// namespace AT
