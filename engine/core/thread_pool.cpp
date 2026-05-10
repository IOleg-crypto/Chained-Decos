#include "thread_pool.h"
#include "engine/core/log.h"

namespace CHEngine
{

void ThreadPool::OnInit()
{
    StartWorkers();
}

void ThreadPool::OnShutdown()
{
    StopWorkers();
}

void ThreadPool::StartWorkers()
{
    size_t threads = std::thread::hardware_concurrency();
    if (threads == 0)
    {
        threads = 1;
    }

    // Leave headroom for the main thread and OS scheduling so asset work does not saturate the machine.
    size_t workerCount = (threads > 1) ? (threads - 1) : 1;

    CH_CORE_INFO("ThreadPool: Initializing with {} worker threads", workerCount);

    for (size_t i = 0; i < workerCount; ++i)
    {
        m_Workers.emplace_back([this](std::stop_token st) { WorkerThread(st); });
    }
}

void ThreadPool::StopWorkers()
{
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Stop = true;
    }
    
    // Trigger the condition variable to wake up any threads waiting for tasks.
    m_Condition.notify_all();

    // Explicitly destroy the threads (joining them) while the mutex and condition variable members are still alive.
    // Member destructors run after the body of this destructor, but it's safer to join them explicitly here.
    m_Workers.clear();
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
        {
            try
            {
                task();
            }
            catch (const std::exception& e)
            {
                CH_CORE_ERROR("ThreadPool: Unhandled exception in queued task: {}", e.what());
            }
            catch (...)
            {
                CH_CORE_ERROR("ThreadPool: Unhandled unknown exception in queued task");
            }
        }
    }
}
} // namespace CHEngine
