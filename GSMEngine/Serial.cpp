#include "Serial.hpp"

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>
#include <iomanip>
#include <iostream>

Serial::Serial()
{
    std::cout << "ctor" << std::endl;
}

void Serial::start()
{
    std::cout << "start" << std::endl;
    //  const char *uartDevice = "/dev/pts/2";
    const char *uartDevice = "/dev/ttyAMA0";

    int fd = open(uartDevice, O_RDWR);
    if(fd == -1)
    {
        std::cerr << "Nie można otworzyć urządzenia UART." << std::endl;
        return;
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

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = 0;
    int totalBytesRead = 0;
    while(true)
    {
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        // wait for data
        int result = select(fd + 1, &read_fds, NULL, NULL, NULL);
        if(result == -1)
        {
            std::cerr << "error with select()." << std::endl;
            break;
        }
        else if(result == 0)
        {
            std::cout << "No data" << std::endl;
        }
        else
        {
            // check if select is for out device
            if(FD_ISSET(fd, &read_fds))
            {
                bytesRead = read(fd, buffer + totalBytesRead, sizeof(buffer) - totalBytesRead - 1);

                if(bytesRead > 0)
                {
                    totalBytesRead += bytesRead;
                    if(static_cast<int>(buffer[totalBytesRead - 1]) == 10 && static_cast<int>(buffer[totalBytesRead - 2]) == 13)
                    {
                        std::cout << "new message: " << std::string(buffer, totalBytesRead) << std::endl;
                        totalBytesRead = 0;
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
            }
        }

        // let's do other things
        // usleep(100000);
        // std::cout << "done" << std::endl;
    }

    close(fd);
    std::cout << "serial was closed" << std::endl;
}
