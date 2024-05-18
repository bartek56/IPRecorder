#ifndef TASKS_HPP
#define TASKS_HPP

#include "spdlog/spdlog.h"
#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <functional>
#include <iostream>

static constexpr uint32_t k_maxWaitingTimeoutForCallRequest = 8000;
static constexpr uint32_t k_waitingTimeoutForNewTask = 400;


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
        SPDLOG_DEBUG("Tasks thread was stopped");
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
        cv.wait_for(lk, std::chrono::milliseconds(k_maxWaitingTimeoutForCallRequest), [this]() { return !isTaskInQueue; });
        if(isTaskInQueue)
        {
            SPDLOG_ERROR("can not call task, queue of tasks is loo long");
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
                    cv.wait_for(lk, std::chrono::milliseconds(k_waitingTimeoutForNewTask), [this]() { return isTaskInQueue; });
                    if(!isTaskInQueue)
                    {
                        //SPDLOG_TRACE("wait for task timeout");
                        continue;
                    }
                }
                while(!tasks.empty())
                {
                    const T& taskParameters = tasks.front();
                    auto result = task(taskParameters);
                    if(!result)
                    {
                        SPDLOG_ERROR("Failed to execute task");
                    }
                    tasks.pop();
                }
                isTaskInQueue = false;
            }
        }
    }
};

#endif// TASKS_HPP
