#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <functional>

#include <atomic>

class Serial
{
public:
    Serial(const std::string serialPort);
    Serial(const Serial &serial) = delete;
    Serial &operator=(const Serial &) = delete;
    Serial(Serial &&serial) = delete;
    Serial &operator=(const Serial &&) = delete;
    ~Serial();

    void readThread();
    void sendThread();
    void sendMessage(std::string message);
    void sendChar(char message);

    void setReadEvent(std::function<void(std::string&)> cb);

private:
    static constexpr size_t k_bufferSize = 256;
    static constexpr size_t k_activeTimems = 400;
    static constexpr size_t k_sleepTimems = 100;
    static constexpr size_t k_activeTimeus = k_activeTimems * 1000;
    static constexpr size_t k_sleepTimeus = k_sleepTimems * 1000;
    int fd;
    std::vector<std::string> m_messagesQueue;

    std::mutex serialMutex;
    std::condition_variable cv;
    std::mutex messageMutex;
    bool isNewMessage;

    std::atomic<bool> serialRunning;
    std::unique_ptr<std::thread> receiver;
    std::unique_ptr<std::thread> sender;
    std::function<void(std::string&)> readEvent;

    void newMessageNotify(std::array<char, k_bufferSize>& buffer, uint32_t& sizeOfMessage);
};

#endif// SERIAL_HPP
