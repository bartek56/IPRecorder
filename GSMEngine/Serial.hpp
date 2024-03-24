#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <string>
#include <vector>

class Serial
{
public:
    Serial(const std::string serialPort);
    ~Serial();
    void start();
    void sendMessage(std::string message);

private:
    static constexpr size_t k_bufferSize = 256;
    int fd;
    std::vector<std::string> m_messagesQueue;
};

#endif// SERIAL_HPP
