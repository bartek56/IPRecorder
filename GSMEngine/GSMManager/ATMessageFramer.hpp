#ifndef ATMESSAGEFRAMER_HPP
#define ATMESSAGEFRAMER_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AT
{
struct FramedMessages
{
    std::vector<std::string> messages;
    bool overflow = false;
};

class ATMessageFramer
{
public:
    explicit ATMessageFramer(size_t maxPendingMessageSize);

    FramedMessages push(std::string_view data);
    std::optional<std::string> flushPending();

private:
    size_t maxPendingMessageSize;
    std::string pendingMessage;
};
}// namespace AT

#endif// ATMESSAGEFRAMER_HPP
