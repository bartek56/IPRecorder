#include "ATParser.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <string>

namespace AT
{
std::optional<ParsedSmsHeader> parseSmsHeader(std::string_view msg)
{
    if(msg.size() < 2 || msg.compare(msg.size() - 2, 2, "\r\n") != 0)
        return std::nullopt;

    const auto msgWithoutCRLF = std::string(msg.substr(0, msg.size() - 2));
    auto splitted = utils::split(msgWithoutCRLF, ",,");
    if(splitted.size() < 2)
        return std::nullopt;

    splitted[0].erase(std::remove(splitted[0].begin(), splitted[0].end(), '"'), splitted[0].end());
    splitted[1].erase(std::remove(splitted[1].begin(), splitted[1].end(), '"'), splitted[1].end());

    const auto senderFields = utils::split(splitted[0], " ");
    if(senderFields.size() < 2 || senderFields[1].empty())
        return std::nullopt;

    return ParsedSmsHeader{senderFields[1], splitted[1]};
}

std::string parseSmsBody(std::string_view msg)
{
    if(msg.size() >= 2 && msg.compare(msg.size() - 2, 2, "\r\n") == 0)
        return std::string(msg.substr(0, msg.size() - 2));

    return std::string(msg);
}

std::optional<std::string> parseClipNumber(std::string_view msg)
{
    auto splitted = utils::split(std::string(msg), ": ");
    if(splitted.size() < 2)
        return std::nullopt;

    auto splitted2 = utils::split(splitted[1], ",");
    if(splitted2.empty() || splitted2[0].size() < 2 || splitted2[0].front() != '"' || splitted2[0].back() != '"')
        return std::nullopt;

    return splitted2[0].substr(1, splitted2[0].size() - 2);
}
}// namespace AT
