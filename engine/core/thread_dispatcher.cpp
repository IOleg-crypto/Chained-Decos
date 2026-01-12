#include "thread_dispatcher.h"
#include "engine/core/log.h"

namespace CHEngine
{
std::vector<std::thread> ThreadDispatcher::s_Workers;
std::queue<std::function<void()>> ThreadDispatcher::s_BackgroundTasks;
std::mutex ThreadDispatcher::s_BackgroundMutex;
std::condition_variable ThreadDispatcher::s_BackgroundCondition;
bool ThreadDispatcher::s_Running = false;

std::queue<std::function<void()>> ThreadDispatcher::s_MainThreadQueue;
std::mutex ThreadDispatcher::s_MainThreadMutex;

void ThreadDispatcher::Init()
{
    s_Running = true;
    uint32_t threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0)
        threadCount = 1;

    CH_CORE_INFO("ThreadDispatcher: Initializing with {0} worker threads", threadCount);

    for (uint32_t i = 0; i < threadCount; ++i)
    {
        s_Workers.emplace_back(WorkerThread);
    }
}

void ThreadDispatcher::Shutdown()
{
    {
        std::unique_lock<std::mutex> lock(s_BackgroundMutex);
        s_Running = false;
    }
    s_BackgroundCondition.notify_all();

    for (auto &worker : s_Workers)
    {
        if (worker.joinable())
            worker.join();
    }
    s_Workers.clear();

    CH_CORE_INFO("ThreadDispatcher: Shutdown complete");
}

void ThreadDispatcher::DispatchMain(std::function<void()> func)
{
    std::unique_lock<std::mutex> lock(s_MainThreadMutex);
    s_MainThreadQueue.push(std::move(func));
}

void ThreadDispatcher::ExecuteMainThreadQueue()
{
    std::unique_lock<std::mutex> lock(s_MainThreadMutex);
    while (!s_MainThreadQueue.empty())
    {
        auto func = std::move(s_MainThreadQueue.front());
        s_MainThreadQueue.pop();

        // Unlock while executing to allow nested dispatches or avoid blocking main thread work
        lock.unlock();
        func();
        lock.lock();
    }
}

void ThreadDispatcher::WorkerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(s_BackgroundMutex);
            s_BackgroundCondition.wait(lock,
                                       [] { return !s_Running || !s_BackgroundTasks.empty(); });

            if (!s_Running && s_BackgroundTasks.empty())
                return;

            task = std::move(s_BackgroundTasks.front());
            s_BackgroundTasks.pop();
        }

        if (task)
            task();
    }
}
} // namespace CHEngine
