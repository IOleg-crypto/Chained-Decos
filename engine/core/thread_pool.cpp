#include "engine/core/application.h"

namespace CHEngine
{
static ThreadPool* s_ThreadPoolInstance = nullptr;

ThreadPool& ThreadPool::Get()
{
    CH_CORE_ASSERT(s_ThreadPoolInstance, "ThreadPool not initialized!");
    return *s_ThreadPoolInstance;
}

ThreadPool::ThreadPool(size_t threads)
{
    CH_CORE_ASSERT(!s_ThreadPoolInstance, "ThreadPool already exists!");
    s_ThreadPoolInstance = this;

    CH_CORE_INFO("ThreadPool: Initializing with {} threads", threads);

    for (size_t i = 0; i < threads; ++i)
    {
        m_Workers.emplace_back([this] { WorkerThread(); });
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
    s_ThreadPoolInstance = nullptr;
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
