#include "engine/core/application.h"

namespace CHEngine
{
ThreadPool& ThreadPool::Get()
{
    return Application::Get().GetThreadPool();
}

ThreadPool::ThreadPool(size_t threads)
{
    CH_CORE_INFO("ThreadPool: Initializing with {} threads", threads);

    for (size_t i = 0; i < threads; ++i)
    {
        m_Workers.emplace_back([this] { WorkerThread(); });
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
}

void ThreadPool::Shutdown()
{
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        if (m_Stop)
        {
            return;
        }
        m_Stop = true;
    }
    m_Condition.notify_all();
    for (std::thread& worker : m_Workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ThreadPool::WorkerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Condition.wait(lock, [this] { return m_Stop || !m_Tasks.empty(); });

            if (m_Stop && m_Tasks.empty())
            {
                return;
            }

            task = std::move(m_Tasks.front());
            m_Tasks.pop();
        }
        task();
    }
}
} // namespace CHEngine
