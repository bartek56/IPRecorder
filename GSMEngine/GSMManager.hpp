#ifndef GSMMANAGER_HPP
#define GSMMANAGER_HPP

#include "GSMTasks.hpp"

#include <string>

class GSMManager
{
public:
    GSMManager(const std::string& port);
    void Initilize();
private:
    GSMTasks tasks;
};

#endif // GSMMANAGER_HPP
