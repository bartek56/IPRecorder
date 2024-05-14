#ifndef TASKS_HPP
#define TASKS_HPP

#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <functional>
#include <iostream>

template<typename T>
class Tasks
{
public:
    Tasks(std::function<bool(const T&)> func)
    {
        task = func;
        tasksThread = std::make_unique<std::thread>([this]() { this->tasksFunc(); });
        tasksRunning.store(true);
   }

    ~Tasks()
    {
        tasksRunning.store(false);
        tasksThread->join();
        std::cout << "task was stopped" << std::endl;
    }

    bool addTask(const T &t)
    {
        {
            std::lock_guard<std::mutex> lc(tasksMutex);
            tasks.push(t);
            isTaskInQueue = true;
        }
        tasksCondition.notify_one();
        return true;
    }

    bool callTask(const T &t)
    {
        std::unique_lock<std::mutex> lk(tasksMutex);
        cv.wait_for(lk, std::chrono::seconds(5), [this]() { return !isTaskInQueue; });
        if(isTaskInQueue)
        {
            std::cout << "can not call task, queue of tasks is loo long" << std::endl;
            return false;
        }
        return task(t);
    }

private:
    std::function<bool(const T&)> task;
    std::atomic<bool> tasksRunning;
    std::unique_ptr<std::thread> tasksThread;
    std::queue<T> tasks;
    std::condition_variable tasksCondition;
    std::mutex tasksMutex;
    bool isTaskInQueue = false;
    std::condition_variable cv;
    void tasksFunc()
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
                    const T& taskParameters = tasks.front();
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
};

#endif// TASKS_HPP
