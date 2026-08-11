#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "FileDescriptor.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <stop_token>
#include <thread>
#include <vector>

class Serial
{
public:
    explicit Serial(std::string_view serialPort);
    Serial(const Serial&) = delete;
    Serial &operator=(const Serial&) = delete;
    Serial(Serial&&) = delete;
    Serial &operator=(Serial&&) = delete;
    ~Serial();

    void setReadEvent(std::function<void(const std::string&)>&& readEventCb);

    void sendMessage(const std::string &message);
    void sendChar(const char &message);

private:
    enum class ReadEventState
    {
        unset,
        setting,
        set
    };

    static constexpr uint32_t k_bufferSize = 256;
    static constexpr size_t k_maxPendingMessageSize = 4096;
    static constexpr size_t k_activeTimems = 400;
    static constexpr size_t k_sleepTimems = 100;
    static constexpr size_t k_activeTimeus = k_activeTimems * 1000;


    FileDescriptor fd;
    std::vector<std::string> m_messagesWriteQueue;

    std::mutex serialMutex;
    std::condition_variable sendCondition;
    std::mutex messagesWriteMutex;
    bool isNewMessageToSend=false;

    std::jthread receiver;
    std::jthread sender;
    std::function<void(const std::string&)> readEvent;
    std::atomic<ReadEventState> readEventState{ReadEventState::unset};

    void sendThread(std::stop_token stopToken);
    void readThread(std::stop_token stopToken);
    void newMessageNotify(std::string message);
    bool writeAll(std::string_view message, std::stop_token stopToken);
};

#endif// SERIAL_HPP
