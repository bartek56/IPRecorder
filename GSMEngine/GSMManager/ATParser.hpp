#ifndef ATPARSER_HPP
#define ATPARSER_HPP

#include <optional>
#include <string>
#include <string_view>

namespace AT
{
struct ParsedSmsHeader
{
    std::string number;
    std::string dateAndTime;
};

std::optional<ParsedSmsHeader> parseSmsHeader(std::string_view msg);
std::string parseSmsBody(std::string_view msg);
std::optional<std::string> parseClipNumber(std::string_view msg);
}// namespace AT

#endif// ATPARSER_HPP
