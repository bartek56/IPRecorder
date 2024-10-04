#include "GSMManager.hpp"
#include "SerialConfig.hpp"
#include "spdlog/spdlog.h"
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
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#-%!()] %v");
    spdlog::set_level(spdlog::level::trace);

    ProgramState::running.store(true);
    ProgramState state;
    signal(SIGTERM, ProgramState::handleSigterm);// Sygnał zakończenia
    signal(SIGINT, ProgramState::handleSigInt);  // Sygnał przerwania (Ctrl+C)
    signal(SIGTSTP, ProgramState::handleSigTstp);// Sygnał zawieszenia (Ctrl+Z)

    GSMManager gsmManager(SERIAL_PORT);
    if(!gsmManager.initilize())
    {
        SPDLOG_ERROR("initialization failed");
        return 0;
    }
    SPDLOG_INFO("Initialization success");
    static uint32_t counter = 0;

    while(ProgramState::running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(ProgramState::running == false)
        {
            break;
        }
        counter++;

        if(counter == 5)
        {
            SPDLOG_INFO("sendSMS message \"hello world\" to 791942336");
            gsmManager.sendSms("+48791942336", "hello world");
            //SPDLOG_INFO("after async message request");
            //gsmManager.sendSmsSync("+48791942336", "test sync message");
            //SPDLOG_INFO("after sync message request");
        }

        if(gsmManager.isNewSms())
        {
            auto sms = gsmManager.getSms();
            SPDLOG_INFO("new SMS: {} {} {}", sms.dateAndTime, sms.number, sms.msg);
            gsmManager.sendSms(sms.number, "thanks for message");
        }
    }
}
