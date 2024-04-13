#include "GSMManager.hpp"

#include <iostream>

GSMManager::GSMManager(const std::string &port) : tasks(port)
{
}

void GSMManager::Initilize()
{

    bool status = false;
    status = tasks.setConfig("AT+AAAA=1");

    if(!status)
    {
        std::cout << "Failed 1" << std::endl;
    }
    tasks.setConfig("AT+BBBB=1");
    tasks.setConfig("AT+CCCC=1");
    tasks.setConfig("AT+DDDD=1");
}
