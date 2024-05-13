#include "GSMTasks.hpp"
#include <string>
#include <iostream>
#include <algorithm>


GSMTasks::GSMTasks(const std::string &port) : atCommander(port, receivedSmses, smsMutex)
{
    tasksThread = std::make_unique<std::thread>([this]() { this->tasksFunc(); });
    tasksRunning.store(true);
}

GSMTasks::~GSMTasks()
{
    tasksRunning.store(false);
    tasksThread->join();
    std::cout << "task was stopped" << std::endl;
}

void GSMTasks::tasksFunc()
{
    while(tasksRunning.load())
    {
        {
            std::unique_lock<std::mutex> lk(tasksMutex);
            if(tasks.empty())
            {
                cv.wait_for(lk, std::chrono::milliseconds(400), [this]() { return isTaskInQueue; });
                if(!isTaskInQueue)
                {
                    //std::cout << "wait for task timeout" << std::endl;
                    continue;
                }
            }
            while(!tasks.empty())
            {
                auto task = tasks.front();
                auto result = atCommander.sendSms(Sms(task.number, task.message));
                if(!result)
                {
                    std::cout << "Failed to send sms" << std::endl;
                }
                tasks.pop();
            }
            isTaskInQueue = false;
        }
    }
}

bool GSMTasks::addSmsTask(const std::string &number, const std::string &message)
{
    {
        std::lock_guard<std::mutex> lc(tasksMutex);
        tasks.push(SmsRequest(number, message));
        isTaskInQueue = true;
    }
    tasksCondition.notify_one();
    return true;
}

bool GSMTasks::sendSms(const std::string &number, const std::string &message)
{
    //std::cout << "sendSms" << std::endl;
    std::unique_lock<std::mutex> lk(tasksMutex);
    cv.wait_for(lk, std::chrono::seconds(5), [this]() { return !isTaskInQueue; });
    if(isTaskInQueue)
    {
        std::cout << "can not send message, queue is loo long" << std::endl;
        return false;
    }
    return atCommander.sendSms(Sms(number, message));
}

bool GSMTasks::isNewSms()
{
    std::lock_guard<std::mutex> lc(smsMutex);
    return !receivedSmses.empty();
}

Sms GSMTasks::getLastSms()
{
    std::lock_guard<std::mutex> lc(smsMutex);
    auto lastSms = receivedSmses.front();
    receivedSmses.pop();
    return lastSms;
}

bool GSMTasks::setConfig(const std::string &command)
{
    std::unique_lock<std::mutex> lk(tasksMutex);
    cv.wait_for(lk, std::chrono::seconds(5), [this]() { return !isTaskInQueue; });
    if(isTaskInQueue)
    {
        std::cout << "can not set config, serial is busy" << std::endl;
        return false;
    }
    return atCommander.setConfig(command);
}
