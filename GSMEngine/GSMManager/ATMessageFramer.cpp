#include "ATMessageFramer.hpp"

namespace AT
{
ATMessageFramer::ATMessageFramer(size_t maxPendingMessageSize) : maxPendingMessageSize(maxPendingMessageSize)
{
}

FramedMessages ATMessageFramer::push(std::string_view data)
{
    FramedMessages result;
    pendingMessage.append(data);
    if(pendingMessage.size() > maxPendingMessageSize)
    {
        pendingMessage.clear();
        result.overflow = true;
        return result;
    }

    while(true)
    {
        const auto messageEnd = pendingMessage.find("\r\n");
        if(messageEnd == std::string::npos)
            break;

        if(messageEnd == 0)
        {
            pendingMessage.erase(0, 2);
            continue;
        }

        const auto messageSize = messageEnd + 2;
        result.messages.push_back(pendingMessage.substr(0, messageSize));
        pendingMessage.erase(0, messageSize);
    }

    return result;
}

std::optional<std::string> ATMessageFramer::flushPending()
{
    if(pendingMessage.empty())
        return std::nullopt;

    auto message = std::move(pendingMessage);
    pendingMessage.clear();
    return message;
}
}// namespace AT
