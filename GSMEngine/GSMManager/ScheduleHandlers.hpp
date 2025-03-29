#ifndef SCHEDULEHANDLERS_HPP
#define SCHEDULEHANDLERS_HPP

#include "ATCommanderScheduler.hpp"

namespace AT
{
class ScheduleHandlers
{
public:
    ScheduleHandlers();
    void SMSInfoHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    void SMSTextHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    void CallingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
    void RingHandler(StatesSwitching &switching, SchedulerData &data, const std::string &command);
};
}// namespace AT

#endif// SCHEDULEHANDLERS_HPP
