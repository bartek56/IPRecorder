#include "ATMessageFramer.hpp"
#include "Serial.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <utility>


Serial::Serial(std::string_view serialPort)
{
    const std::string portPath{serialPort};
    const int descriptor = ::open(portPath.c_str(), O_RDWR | O_NOCTTY);
    if(descriptor == -1)
    {
        SPDLOG_ERROR("GSM serial is not connected on port: {}", serialPort);
        throw std::system_error(errno, std::generic_category(), "Failed to open serial port " + portPath);
    }
    fd.reset(descriptor);

    termios options{};
    if(tcgetattr(fd.get(), &options) == -1)
        throw std::system_error(errno, std::generic_category(), "Failed to read serial port settings");

    cfmakeraw(&options);
    options.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    options.c_cflag |= CLOCAL | CREAD | CS8;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    if(cfsetispeed(&options, B19200) == -1 || cfsetospeed(&options, B19200) == -1)
        throw std::system_error(errno, std::generic_category(), "Failed to set serial port speed");
    if(tcsetattr(fd.get(), TCSANOW, &options) == -1)
        throw std::system_error(errno, std::generic_category(), "Failed to apply serial port settings");

    const int flags = fcntl(fd.get(), F_GETFL);
    if(flags == -1 || fcntl(fd.get(), F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::system_error(errno, std::generic_category(), "Failed to enable non-blocking serial port mode");

    receiver = std::jthread([this](std::stop_token stopToken) { readThread(stopToken); });
    sender = std::jthread([this](std::stop_token stopToken) { sendThread(stopToken); });
}

Serial::~Serial()
{
    sender.request_stop();
    sendCondition.notify_all();
    receiver.request_stop();
    SPDLOG_INFO("Serial is stopping");
}

void Serial::readThread(std::stop_token stopToken)
{
    fd_set read_fds;
    std::array<char, k_bufferSize> readBuffer{};
    AT::ATMessageFramer messageFramer(k_maxPendingMessageSize);

    while(!stopToken.stop_requested())
    {
        FD_ZERO(&read_fds);
        FD_SET(fd.get(), &read_fds);
        struct timeval timeout
        {
        };
        timeout.tv_sec = 0;
        timeout.tv_usec = k_activeTimeus;

        // wait for data
        const int result = select(fd.get() + 1, &read_fds, nullptr, nullptr, &timeout);
        if(result == -1)
        {
            if(errno == EINTR)
                continue;

            SPDLOG_ERROR("error with select()");
            break;
        }

        if(result == 0)
        {
            auto pendingMessage = messageFramer.flushPending();
            if(pendingMessage)
            {
                SPDLOG_TRACE("message delimiter timeout");
                newMessageNotify(std::move(*pendingMessage));
            }
            continue;
        }

        if(!FD_ISSET(fd.get(), &read_fds))
            continue;

        ssize_t bytesRead = 0;
        {
            const std::lock_guard<std::mutex> lock(serialMutex);
            bytesRead = ::read(fd.get(), readBuffer.data(), readBuffer.size());
        }

        if(bytesRead == -1)
        {
            if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            SPDLOG_ERROR("error with read()");
            break;
        }

        if(bytesRead == 0)
            continue;

        auto framedMessages = messageFramer.push(std::string_view(readBuffer.data(), static_cast<size_t>(bytesRead)));
        if(framedMessages.overflow)
        {
            SPDLOG_ERROR("received AT message exceeds {} bytes", k_maxPendingMessageSize);
            continue;
        }

        for(auto &message : framedMessages.messages)
            newMessageNotify(std::move(message));
    }
    SPDLOG_DEBUG("receiver closed");
}

void Serial::setReadEvent(std::function<void(const std::string &)> &&readEventCb)
{
    auto expectedState = ReadEventState::unset;
    if(!readEventState.compare_exchange_strong(expectedState, ReadEventState::setting))
        throw std::logic_error("Serial read callback is already set");

    try
    {
        readEvent = std::move(readEventCb);
        readEventState.store(ReadEventState::set, std::memory_order_release);
    }
    catch(...)
    {
        readEventState.store(ReadEventState::unset, std::memory_order_release);
        throw;
    }
}

void Serial::newMessageNotify(std::string newMessage)
{
    SPDLOG_TRACE("new message {}", newMessage);
    if(readEventState.load(std::memory_order_acquire) == ReadEventState::set)
        readEvent(newMessage);
    else
        SPDLOG_WARN("AT message received before read callback was set: {}", newMessage);
}

void Serial::sendThread(std::stop_token stopToken)
{
    while(!stopToken.stop_requested())
    {
        {
            std::unique_lock<std::mutex> lockMessageWrite(messagesWriteMutex);
            sendCondition.wait_for(lockMessageWrite, std::chrono::milliseconds(k_activeTimems),
                                   [this, &stopToken]() {
                                       return stopToken.stop_requested() || isNewMessageToSend;
                                   });

            if(stopToken.stop_requested())
                break;

            if(m_messagesWriteQueue.empty())
            {
                isNewMessageToSend = false;
                continue;
            }
            auto newMessage = m_messagesWriteQueue.begin();
            bool messageWasWritten = false;
            {
                const std::lock_guard<std::mutex> lockSerial(serialMutex);
                messageWasWritten = writeAll(*newMessage, stopToken);
            }
            if(!messageWasWritten)
            {
                SPDLOG_ERROR("Error to send data");
            }
            else
            {
                SPDLOG_TRACE("Message was send {}", newMessage->data());
            }

            m_messagesWriteQueue.erase(newMessage);
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(k_sleepTimems));
    }
    SPDLOG_DEBUG("sender closed");
}

bool Serial::writeAll(std::string_view message, std::stop_token stopToken)
{
    size_t bytesWritten = 0;

    while(bytesWritten < message.size() && !stopToken.stop_requested())
    {
        const auto result = ::write(fd.get(), message.data() + bytesWritten, message.size() - bytesWritten);

        if(result > 0)
        {
            bytesWritten += static_cast<size_t>(result);
            continue;
        }

        if(result == -1 && errno == EINTR)
            continue;

        if(result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(k_sleepTimems));
            continue;
        }

        return false;
    }

    return bytesWritten == message.size();
}


void Serial::sendMessage(const std::string &message)
{
    const std::lock_guard<std::mutex> lock(messagesWriteMutex);
    m_messagesWriteQueue.push_back(message + "\r");
    isNewMessageToSend = true;
    sendCondition.notify_one();
}

void Serial::sendChar(const char &message)
{
    const std::lock_guard<std::mutex> lock(messagesWriteMutex);
    m_messagesWriteQueue.emplace_back(1, message);
    isNewMessageToSend = true;
    sendCondition.notify_one();
}
