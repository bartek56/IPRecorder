#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <csignal>

#include <atomic>

class Serial
{
public:
    Serial(const std::string serialPort);
    ~Serial();
    static void readThread();
    static void sendThread();
    void sendMessage(std::string message);

private:
    static constexpr size_t k_bufferSize = 256;
    static int fd;
    static std::vector<std::string> m_messagesQueue;

    static std::mutex serialMutex;
    static std::mutex messageMutex;

    static std::atomic<bool> serialRunning;
    std::unique_ptr<std::thread> receiver;
    std::unique_ptr<std::thread> sender;
};

#endif// SERIAL_HPP
