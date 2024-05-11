#include "GSMManager.hpp"
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

    const std::string port = "/dev/ttyAMA0";// GSM serial on NAS
    //const std::string port = "/dev/pts/4";// virtual for testing
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
            gsmManager.SendSms("+48791942336", "test async message");
            std::cout << "after async message request" << std::endl;
            gsmManager.SendSmsSync("+48791942336", "test sync message");
            std::cout << "after sync message request" << std::endl;
        }

        if(gsmManager.isNewSms())
        {
            auto sms = gsmManager.getSms();
            std::cout << "new SMS:" << std::endl;
            std::cout << sms.dateAndTime << std::endl;
            std::cout << sms.number << ": " << sms.msg << std::endl;
            gsmManager.SendSms(sms.number, "thanks for message");
        }
    }
}
