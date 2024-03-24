#include "Serial.hpp"
#include <iostream>

int main()
{
    std::cout << "Hello world" << std::endl;

    // const std::string port = "/dev/pts/3"; // GSM serial on NAS
    const std::string port = "/dev/pts/3";// virtual for testing

    Serial serial(port);
    serial.sendMessage("Hello \n");
    serial.sendMessage("World!\n");
    serial.start();
}
