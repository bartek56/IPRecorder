#include "Tasks.hpp"

#include <iostream>

#include <algorithm>

template<typename T>
Tasks<T>::Tasks(std::function<bool(const T &)> func) : task(func)
{
    tasksThread = std::make_unique<std::thread>([this]() { this->tasksFunc(); });
    tasksRunning.store(true);
}

template<typename T>
Tasks<T>::~Tasks()
{
    tasksRunning.store(false);
    tasksThread->join();
    std::cout << "task was stopped" << std::endl;
}

template<typename T>
void Tasks<T>::tasksFunc()
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
                T taskParameters = tasks.front();

                auto result = task(taskParameters);
                if(!result)
                {
                    std::cout << "Failed to execute task" << std::endl;
                }
                tasks.pop();
            }
            isTaskInQueue = false;
        }
    }
}
template<typename T>
bool Tasks<T>::addTask(const T &t)
{
    {
        std::lock_guard<std::mutex> lc(tasksMutex);
        tasks.push(t);
        isTaskInQueue = true;
    }
    tasksCondition.notify_one();
    return true;
}

template<typename T>
bool Tasks<T>::callTask(const T &t)
{
    //std::cout << "sendSms" << std::endl;
    std::unique_lock<std::mutex> lk(tasksMutex);
    cv.wait_for(lk, std::chrono::seconds(5), [this]() { return !isTaskInQueue; });
    if(isTaskInQueue)
    {
        std::cout << "can not call task, queue of tasks is loo long" << std::endl;
        return false;
    }

    return task(t);
}
