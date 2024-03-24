#include "Serial.hpp"

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <array>

Serial::Serial(const std::string serialPort) : fd(-1), m_messagesQueue()
{
    fd = open(serialPort.c_str(), O_RDWR);
    if(fd == -1)
    {
        std::cerr << "GSM serial is not connected on port: " << serialPort << std::endl;
        throw std::runtime_error("Initialization failed");
    }

    // serial port configuration
    struct termios options;
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
}

Serial::~Serial()
{
    close(fd);
    std::cout << "Serial was closed" << std::endl;
}

void Serial::start()
{
    //    char buffer[256];
    std::array<char, k_bufferSize> buffer;
    std::fill(buffer.begin(), buffer.end(), 0);

    fd_set read_fds;
    int bytesRead = 0;
    int totalBytesRead = 0;
    bool isBusy = false;
    while(true)
    {
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        // wait for data
        int result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if(result == -1)
        {
            std::cerr << "error with select()." << std::endl;
            break;
        }

        else if(result > 0)
        {
            isBusy = true;
            // check if select is for out device
            if(FD_ISSET(fd, &read_fds))
            {
                bytesRead = read(fd, buffer.data() + totalBytesRead, sizeof(buffer) - totalBytesRead - 1);

                if(bytesRead > 0)
                {
                    totalBytesRead += bytesRead;
                    if(static_cast<int>(buffer[totalBytesRead - 1]) == 10 && static_cast<int>(buffer[totalBytesRead - 2]) == 13)
                    {
                        std::cout << "new message: " << std::string(buffer.data(), totalBytesRead) << std::endl;
                        totalBytesRead = 0;
                        std::fill(buffer.begin(), buffer.end(), 0);
                        isBusy = false;
                    }
                }
            }
        }

        //        else if(result == 0)
        //        {
        //            std::cout << "timeout, No data" << std::endl;
        //        }

        // let's do other things
        if(!isBusy && m_messagesQueue.size() > 0)
        {
            auto newMessage = m_messagesQueue[0];

            int bytes_written = write(fd, newMessage.c_str(), newMessage.size());// Wysłanie wiadomości

            if(bytes_written < 0)
            {
                std::cerr << "Błąd podczas wysyłania danych." << std::endl;
            }

            std::cout << "Wysłano dane: " << newMessage << std::endl;

            m_messagesQueue.erase(m_messagesQueue.begin());
        }
    }
}

void Serial::sendMessage(std::string message)
{
    message.append("\r");
    m_messagesQueue.push_back(message);
}
