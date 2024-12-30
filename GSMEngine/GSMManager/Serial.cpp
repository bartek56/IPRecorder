#include "Serial.hpp"
#include "spdlog/spdlog.h"

#include "Utils.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <utility>


Serial::Serial(std::string_view serialPort) : fd(-1), serialRunning(false), buffer()
{
    fd = open(serialPort.begin(), O_RDWR);
    if(fd == -1)
    {
        SPDLOG_ERROR("GSM serial is not connected on port: {}", serialPort);
        throw std::runtime_error("Initialization failed");
    }

    // serial port configuration
    struct termios options
    {
    };
    tcgetattr(fd, &options);
    cfsetispeed(&options, B19200);
    cfsetospeed(&options, B19200);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;// disable parity
    options.c_cflag &= ~CSTOPB;// one bit of stop
    options.c_cflag &= ~CSIZE; // disable bits of size
    options.c_cflag |= CS8;    // 8 bits of data
    options.c_lflag = 0;       //~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag = 0;       // ~(IXON | IXOFF | IXANY);
    tcsetattr(fd, TCSANOW, &options);

    // non-blocking mode
    fcntl(fd, F_SETFL, O_NONBLOCK);
    serialRunning.store(true);

    std::fill(buffer.begin(), buffer.end(), 0);
    receiver = std::make_unique<std::thread>([this]() { this->readThread(); });
    sender = std::make_unique<std::thread>([this]() { this->sendThread(); });
}

Serial::~Serial()
{
    serialRunning.store(false);
    sender->join();
    receiver->join();
    close(fd);
    SPDLOG_INFO("Serial was closed");
}

void Serial::readThread()
{
    fd_set read_fds;

    // number of read bytes from serial on one sequence
    ssize_t bytesRead = 0;

    // number of read bytes from serial
    uint32_t totalBytesRead = 0;

    // size of message with end CRLF
    uint32_t sizeOfMessage = 0;
    char *startOfMessage = nullptr;
    struct timeval timeout
    {
    };
    timeout.tv_sec = 0;
    timeout.tv_usec = k_activeTimeus;

    while(serialRunning.load())
    {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        // wait for data
        const int result = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if(result == -1)
        {
            SPDLOG_ERROR("error with select()");
            break;
        }
        // timeout but some data was saved in buffer
        if(result == 0 && sizeOfMessage > 0)
        {
            SPDLOG_TRACE("timeout");
            newMessageNotify(startOfMessage, sizeOfMessage);
            totalBytesRead = 0;
            sizeOfMessage = 0;
        }

        else if(result > 0)
        {
            // check if select is for dedicated device
            if(FD_ISSET(fd, &read_fds))
            {
                {
                    const std::lock_guard<std::mutex> lock(serialMutex);
                    bytesRead = read(fd, buffer.data() + totalBytesRead, k_bufferSize - totalBytesRead - 1);
                }

                if(bytesRead > 0)
                {
                    totalBytesRead += bytesRead;
                    startOfMessage = buffer.data();
                    uint32_t sizeOfPreviousMessage = totalBytesRead - bytesRead;
                    if(sizeOfPreviousMessage > 0)
                    {
                        sizeOfPreviousMessage -= 1;
                    }

                    for(uint32_t i = sizeOfPreviousMessage; i < (totalBytesRead - 1); i++)
                    {
                        sizeOfMessage++;

                        const auto asciValue1 = utils::charToInt(buffer[i]);
                        const auto asciValue2 = utils::charToInt(buffer[i + 1]);
                        SPDLOG_TRACE("byte {} {}", i, asciValue1);


                        if(asciValue1 == k_CR && asciValue2 == k_LF)
                        {
                            SPDLOG_TRACE("byte {} {} it is CRLF", i, asciValue2);
                            if(i == 0)
                            {
                                SPDLOG_TRACE("skip first CRLF");
                                i += 1;
                                sizeOfMessage += 1;
                                continue;
                            }
                            i++;
                            sizeOfMessage++;
                            if(utils::charToInt(buffer[i + 1]) == k_CR && utils::charToInt(buffer[i + 2]) == k_LF)
                            {
                                SPDLOG_TRACE("skip CRLF duplicate");
                                // skip CRLF duplicate
                                i += 2;
                            }
                            newMessageNotify(startOfMessage, sizeOfMessage);
                            sizeOfMessage = 0;
                            startOfMessage = buffer.data() + i + 1;
                        }
                    }
                    if(sizeOfMessage == 0)
                    {
                        // no more data - next data put to start of buffer
                        SPDLOG_TRACE("no more data");
                        totalBytesRead = 0;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(k_sleepTimems));
    }
    SPDLOG_DEBUG("receiver closed");
}

void Serial::setReadEvent(std::function<void(std::string &)> &&readEventCb)
{
    readEvent = std::move(readEventCb);
}

void Serial::newMessageNotify(char *buffer, const uint32_t &sizeOfMessage)
{
    auto newMessage = std::string(buffer, sizeOfMessage);
    SPDLOG_TRACE("new message {}", newMessage);
    if(readEvent)
    {
        readEvent(newMessage);
    }
}

void Serial::sendThread()
{
    while(serialRunning.load())
    {
        {
            std::unique_lock<std::mutex> lockMessageWrite(messagesWriteMutex);
            sendCondition.wait_for(lockMessageWrite, std::chrono::milliseconds(k_activeTimems),
                                   [this]() { return isNewMessageToSend; });

            if(m_messagesWriteQueue.empty())
            {
                isNewMessageToSend = false;
                continue;
            }
            auto newMessage = m_messagesWriteQueue.begin();
            ssize_t bytesWritten = 0;
            {
                const std::lock_guard<std::mutex> lockSerial(serialMutex);
                // send char
                if(newMessage->size() == 2)
                {
                    auto ptr = static_cast<char>(std::stoi(newMessage->c_str()));
                    bytesWritten = write(fd, &ptr, 1);
                }
                else
                {
                    bytesWritten = write(fd, newMessage->c_str(), newMessage->size());
                }
            }
            if(bytesWritten < 0)
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
    m_messagesWriteQueue.push_back(std::to_string(message));
    isNewMessageToSend = true;
    sendCondition.notify_one();
}
