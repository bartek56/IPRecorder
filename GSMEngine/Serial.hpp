#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>

#include <atomic>

class Serial
{
public:
    Serial(const std::string serialPort);
    Serial(const Serial& serial) = delete;
    Serial& operator=(const Serial&) = delete;
    Serial(Serial&& serial) = delete;
    Serial& operator=(const Serial&&) = delete;
    ~Serial();

    void readThread();
    void sendThread();
    void sendMessage(std::string message);

private:
    static constexpr size_t k_bufferSize = 256;
    int fd;
    std::vector<std::string> m_messagesQueue;

    std::mutex serialMutex;
    std::condition_variable cv;
    std::mutex messageMutex;
    bool isNewMessage;

    std::atomic<bool> serialRunning;
    std::unique_ptr<std::thread> receiver;
    std::unique_ptr<std::thread> sender;
};

#endif// SERIAL_HPP
