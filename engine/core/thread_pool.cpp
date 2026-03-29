#include "thread_pool.h"
#include "engine/core/log.h"

namespace CHEngine
{
ThreadPool::ThreadPool()
{
    size_t threads = std::thread::hardware_concurrency();
    CH_CORE_INFO("ThreadPool: Initializing with {} threads", threads);

    for (size_t i = 0; i < threads; ++i)
    {
        m_Workers.emplace_back([this](std::stop_token st) { WorkerThread(st); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Stop = true;
    }
    m_Condition.notify_all();
    // jthreads will automatically join here
}

void ThreadPool::WorkerThread(std::stop_token stopToken)
{
    while (!stopToken.stop_requested())
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            
            // Wait until there's a task or the entire pool is stopping
            m_Condition.wait(lock, stopToken, [this] { 
                return m_Stop || !m_Tasks.empty(); 
            });

            if (m_Stop && m_Tasks.empty())
                return;

            if (m_Tasks.empty())
                continue;

            task = std::move(m_Tasks.front());
            m_Tasks.pop();
        }
        
        if (task)
            task();
    }
}
} // namespace CHEngine
