#include "Serial.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

Serial::Serial() { std::cout << "ctor" << std::endl; }

void Serial::start() {
  std::cout << "start" << std::endl;
//  const char *uartDevice = "/dev/pts/2";
  const char *uartDevice = "/dev/ttyAMA0";

  int fd = open(uartDevice, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    std::cerr << "Nie można otworzyć urządzenia UART." << std::endl;
    return;
  }

  // serial port configuration
  struct termios options;
  tcgetattr(fd, &options);
  cfsetispeed(&options, B19200);
  cfsetospeed(&options, B19200);
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB; // disable parity
  options.c_cflag &= ~CSTOPB; // one bit of stop
  options.c_cflag &= ~CSIZE; // disable bits of size
  options.c_cflag |= CS8; // 8 bits of data
  options.c_lflag = 0; //~(ICANON | ECHO | ECHOE | ISIG);
  options.c_iflag = 0; // ~(IXON | IXOFF | IXANY);
  tcsetattr(fd, TCSANOW, &options);

  // non-blocking mode
  fcntl(fd, F_SETFL, O_NONBLOCK);

  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  while (true) {

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    // wait for data
    int result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result == -1) {
      std::cerr << "error with select()." << std::endl;
      break;
    } else if (result == 0) {
      std::cout << "No data" << std::endl;
    } else {

      // check if select is for out device
      if (FD_ISSET(fd, &read_fds)) {
        // read data
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
				usleep(100000); // wait to get whole message from serial
        int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
          std::cout << "New data: " << buffer << std::endl;
        }
      }
    }

    // let's do other things
    usleep(100000);
    std::cout << "wait" << std::endl;
  }

  close(fd);
	std::cout << "serial was closed" << std::endl;
}
