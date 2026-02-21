#include "thread_pool.h"
#include "engine/core/assert.h"
#include "engine/core/log.h"

namespace CHEngine
{
static ThreadPool* s_Instance = nullptr;

ThreadPool& ThreadPool::Get()
{
    CH_CORE_ASSERT(s_Instance, "ThreadPool instance is null!");
    return *s_Instance;
}

ThreadPool::ThreadPool(size_t threads)
{
    if (!s_Instance)
    {
        s_Instance = this;
    }
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
