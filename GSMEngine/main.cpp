#include "Serial.hpp"
#include <iostream>
#include <csignal>


class ProgramState
{
public:
    static std::atomic<bool> running;
    static void handleSigterm(int signal)
    {
        std::cout << "SIGTERM. GSMSerial is closing ..." << std::endl;
        running = false;
        exit(EXIT_SUCCESS);
    }
    static void handleSigInt(int signal)
    {
        std::cout << "SIGINT (Ctrl+C). GSMSerial is closing ..." << std::endl;
        running = false;
    }
    static void handleSigTstp(int signal)
    {
        std::cout << "SIGTSTP (Ctrl+Z). GSMSerial is closing ..." << std::endl;
        running = false;
        exit(EXIT_SUCCESS);
    }
};

std::atomic<bool> ProgramState::running;

int main()
{
    std::cout << "start" << std::endl;
    ProgramState::running.store(true);
    ProgramState state;
    signal(SIGTERM, ProgramState::handleSigterm);// Sygnał zakończenia
    signal(SIGINT, ProgramState::handleSigInt);  // Sygnał przerwania (Ctrl+C)
    signal(SIGTSTP, ProgramState::handleSigTstp);// Sygnał zawieszenia (Ctrl+Z)

    // const std::string port = "/dev/ttyAMA0"; // GSM serial on NAS
    const std::string port = "/dev/pts/4";// virtual for testing

    Serial serial(port);
    while(ProgramState::running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        //        serial.sendMessage("test message");
        if(ProgramState::running == false)
        {
            break;
        }
    }
}
