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
    Tasks()
    {
   }
    void init()
    {
        task = [](T t){std::cout << "func from task" << std::endl;return true;};

        tasksThread = std::make_unique<std::thread>([this]() { this->tasksFunc(); });
        tasksRunning.store(true);
    }
    ~Tasks()
    {
        tasksRunning.store(false);
        tasksThread->join();
        //std::cout << "task was stopped" << std::endl;
    }
    bool addTask(const T &t)
    {
        {
            std::lock_guard<std::mutex> lc(tasksMutex);
            tasks.push(t);
            isTaskInQueue = true;
            std::cout << "task was added" << std::endl;
        }
        tasksCondition.notify_one();
        return true;
    }
    bool callTask(const T &t)
    {
        std::cout << "call task" << std::endl;
        std::unique_lock<std::mutex> lk(tasksMutex);
        cv.wait_for(lk, std::chrono::seconds(5), [this]() { return !isTaskInQueue; });
        if(isTaskInQueue)
        {
            std::cout << "can not call task, queue of tasks is loo long" << std::endl;
            return false;
        }
        std::cout << "before calling" << std::endl;
        return task(t);
    }


private:
    std::function<bool(T)> task;
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
                    std::cout << "get task" << std::endl;
                    T taskParameters = tasks.front();
                    std::cout << "call task from queue" << std::endl;
                    std::cout << "message 2 " << taskParameters.message << " number: " << taskParameters.number << std::endl;
                    auto result = task(taskParameters);
                    std::cout << "after caling task from wueue" << std::endl;
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
