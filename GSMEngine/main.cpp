#include "Serial.hpp"
#include <iostream>
#include <csignal>

std::atomic<bool> running(true);

void handle_signal(int signal)
{
    if(signal == SIGTERM)
    {
        std::cout << "SIGTERM. GSMSerial is closing ..." << std::endl;
        exit(EXIT_SUCCESS);
    }
    else if(signal == SIGINT)
    {
        std::cout << "SIGINT (Ctrl+C). GSMSerial is closing ..." << std::endl;
    }
    else if(signal == SIGTSTP)
    {
        std::cout << "SIGTSTP (Ctrl+Z). GSMSerial is closing ..." << std::endl;
        exit(EXIT_SUCCESS);
    }
    running = false;
}

int main()
{
    std::cout << "start" << std::endl;
    signal(SIGTERM, handle_signal);// Sygnał zakończenia
    signal(SIGINT, handle_signal); // Sygnał przerwania (Ctrl+C)
    signal(SIGTSTP, handle_signal);// Sygnał zawieszenia (Ctrl+Z)

    // const std::string port = "/dev/ttyAMA0"; // GSM serial on NAS
    const std::string port = "/dev/pts/3";// virtual for testing

    Serial serial(port);
    //    std::this_thread::sleep_for(std::chrono::seconds(4));
    //    serial.sendMessage("hello");
    //    serial.sendMessage("world!");
    while(running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(running == false)
        {
            break;
        }
    }
}
