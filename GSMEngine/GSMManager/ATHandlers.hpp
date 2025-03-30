#ifndef ATHANDLERS_HPP
#define ATHANDLERS_HPP

#include <string>
#include "ATTypes.hpp"

namespace AT{
class ATHandlers
{
public:
    ATHandlers();
        // ATHandlers
    ResultState SMSReceivingInfoHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    ResultState SMSSendingConfirmHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    ResultState SMSReceivingTextHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    ResultState CallingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    ResultState RingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);

};
}
#endif// ATHANDLERS_HPP
