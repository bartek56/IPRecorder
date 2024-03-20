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
  // Ścieżka do urządzenia UART (np. /dev/ttyS0, /dev/ttyUSB0 itp.)
  const char *uartDevice = "/dev/pts/2";

  // Otwarcie urządzenia UART
  int fd = open(uartDevice, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    std::cerr << "Nie można otworzyć urządzenia UART." << std::endl;
    return;
  }

  // Konfiguracja parametrów portu szeregowego
  struct termios options;
  tcgetattr(fd, &options);
  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_lflag = 0; //~(ICANON | ECHO | ECHOE | ISIG);
  options.c_iflag = 0; // ~(IXON | IXOFF | IXANY);
  tcsetattr(fd, TCSANOW, &options);

  // Ustawienie non-blocking mode
  fcntl(fd, F_SETFL, O_NONBLOCK);

  // Inicjalizacja zbioru deskryptorów
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  while (true) {

    // Ustawienie czasu oczekiwania na zero
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    // Czekanie na dane przy użyciu select
    int result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result == -1) {
      std::cerr << "Błąd select()." << std::endl;
      break;
    } else if (result == 0) {
      std::cout << "Brak danych w danym przedziale czasu." << std::endl;
    } else {

      // Sprawdzanie, czy zdarzenie dotyczy naszego deskryptora
      if (FD_ISSET(fd, &read_fds)) {
        // Odczytywanie danych z urządzenia UART
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
          std::cout << "Otrzymane dane: " << buffer << std::endl;
        }
      }
    }

    // Czekanie na nowe zdarzenia
    // usleep(100000); // Oczekiwanie 0.1 sekundy (opcjonalne)
    std::cout << "wait" << std::endl;
  }

  // Zamykanie urządzenia UART
  close(fd);
}
