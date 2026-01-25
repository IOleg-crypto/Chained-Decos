#include "task_system.h"
#include "engine/core/log.h"

namespace CHEngine
{
// Static Initializations
std::queue<std::function<void()>> TaskSystem::s_Tasks;
std::vector<std::unique_ptr<ChainedThread>> TaskSystem::s_Workers;
std::mutex TaskSystem::s_QueueMutex;
std::condition_variable TaskSystem::s_Condition;
bool TaskSystem::s_Stopping = false;

// --- ChainedThread Implementation ---

ChainedThread::ChainedThread()
{
}

ChainedThread::~ChainedThread()
{
    Stop();
}

void ChainedThread::Start()
{
    m_Running = true;
    m_Thread = std::thread(&ChainedThread::ThreadLoop, this);
}

void ChainedThread::Stop()
{
    m_Running = false;
    if (m_Thread.joinable())
        m_Thread.join();
}

void ChainedThread::ThreadLoop()
{
    while (m_Running)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(TaskSystem::s_QueueMutex);
            TaskSystem::s_Condition.wait(
                lock, [] { return TaskSystem::s_Stopping || !TaskSystem::s_Tasks.empty(); });

            if (TaskSystem::s_Stopping && TaskSystem::s_Tasks.empty())
                return;

            task = std::move(TaskSystem::s_Tasks.front());
            TaskSystem::s_Tasks.pop();
        }

        // Execute the work
        try
        {
            task();
        }
        catch (const std::exception &e)
        {
            CH_CORE_ERROR("TaskSystem: Exception in background thread: {}", e.what());
        }
    }
}

// --- TaskSystem Implementation ---

void TaskSystem::Init(uint32_t threadCount)
{
    if (threadCount == 0)
    {
        threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0)
            threadCount = 2; // Fallback
    }

    CH_CORE_INFO("TaskSystem: Initializing with {} ChainedThreads", threadCount);

    s_Stopping = false;
    for (uint32_t i = 0; i < threadCount; ++i)
    {
        auto worker = std::make_unique<ChainedThread>();
        worker->Start();
        s_Workers.push_back(std::move(worker));
    }
}

void TaskSystem::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(s_QueueMutex);
        s_Stopping = true;
    }
    s_Condition.notify_all();

    CH_CORE_INFO("TaskSystem: Stopping workers...");
    s_Workers.clear(); // Scope handles thread joining
    CH_CORE_INFO("TaskSystem: Shutdown complete.");
}
} // namespace CHEngine
