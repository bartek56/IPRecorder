#ifndef GSMMANAGER_HPP
#define GSMMANAGER_HPP

#include "ATCommander.hpp"

#include <string>
#include <optional>
using namespace AT;

class GSMManager
{
public:
    GSMManager(std::string_view port);
    bool initilize();
    bool sendSms(const std::string& number, const std::string& message);
    bool sendSmsSync(const std::string& number, const std::string& message);
    std::optional<Sms> getSms();
    std::optional<Call> getCall();
private:
    ATCommander atCommander;
    bool setDefaultConfig();
};

#endif // GSMMANAGER_HPP
