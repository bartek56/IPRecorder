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

    while(1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
