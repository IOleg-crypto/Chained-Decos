#include "thread_pool.h"
#include "engine/core/log.h"

namespace CHEngine
{
ThreadPool::ThreadPool(size_t numThreads)
{
    if (numThreads == 0)
        numThreads = 1;

    CH_CORE_INFO("ThreadPool initialized with %d worker threads", numThreads);

    for (size_t i = 0; i < numThreads; ++i)
    {
        m_Workers.emplace_back(
            [this]
            {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_QueueMutex);
                        m_Condition.wait(lock, [this] { return m_Stop || !m_Tasks.empty(); });

                        if (m_Stop && m_Tasks.empty())
                            return;

                        task = std::move(m_Tasks.front());
                        m_Tasks.pop();
                    }

                    m_ActiveTasks++;
                    task();
                    m_ActiveTasks--;
                }
            });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Stop = true;
    }
    m_Condition.notify_all();

    for (std::thread &worker : m_Workers)
    {
        if (worker.joinable())
            worker.join();
    }

    CH_CORE_INFO("ThreadPool shut down");
}

void ThreadPool::WaitAll()
{
    while (m_ActiveTasks.load() > 0 || !m_Tasks.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
} // namespace CHEngine
