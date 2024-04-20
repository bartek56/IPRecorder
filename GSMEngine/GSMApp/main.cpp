#include "GSMManager.hpp"
#include <iostream>
#include <csignal>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"

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
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%!:%#] %v");
    spdlog::set_level(spdlog::level::trace);

    SPDLOG_WARN("warn");
    SPDLOG_INFO("info");
    SPDLOG_DEBUG("debug");
    SPDLOG_TRACE("trace");

    std::cout << "start" << std::endl;
    ProgramState::running.store(true);
    ProgramState state;
    signal(SIGTERM, ProgramState::handleSigterm);// Sygnał zakończenia
    signal(SIGINT, ProgramState::handleSigInt);  // Sygnał przerwania (Ctrl+C)
    signal(SIGTSTP, ProgramState::handleSigTstp);// Sygnał zawieszenia (Ctrl+Z)

    //const std::string port = "/dev/ttyAMA0";// GSM serial on NAS
    const std::string port = "/dev/pts/4";// virtual for testing
    GSMManager gsmManager(port);
    if(!gsmManager.Initilize())
    {
        std::cout << "initialization failed" << std::endl;
        return 0;
    }
    std::cout << "Initialization success" << std::endl;
    static uint32_t counter = 0;

    while(ProgramState::running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(ProgramState::running == false)
        {
            break;
        }
        counter++;

        if(counter == 15)
        {
            std::cout << "sendSMS test message to 791942336" << std::endl;
            gsmManager.SendSms("test message", 791942336);
        }
    }
}
