#ifndef CH_TASK_SYSTEM_H
#define CH_TASK_SYSTEM_H

#include "engine/core/base.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace CHEngine
{
/**
 * ChainedThread represents a worker thread in the task system.
 * It waits for tasks and executes them in the background.
 */
class ChainedThread
{
public:
    ChainedThread();
    ~ChainedThread();

    void Start();
    void Stop();

private:
    void ThreadLoop();

private:
    std::thread m_Thread;
    bool m_Running = false;
};

/**
 * TaskSystem manages a pool of ChainedThreads and distributes jobs.
 */
class TaskSystem
{
public:
    static void Init(uint32_t threadCount = 0);
    static void Shutdown();

    /**
     * Pushes a task to the background queue.
     * A future that will contain the result of the task.
     */
    template <typename F, typename... Args>
    static auto PushTask(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::lock_guard<std::mutex> lock(s_QueueMutex);
            if (s_Stopping)
                throw std::runtime_error("TaskSystem: Cannot push task on stopping system");

            s_Tasks.emplace([task]() { (*task)(); });
        }
        s_Condition.notify_one();
        return res;
    }

private:
    friend class ChainedThread;

    static std::queue<std::function<void()>> s_Tasks;
    static std::vector<std::unique_ptr<ChainedThread>> s_Workers;

    static std::mutex s_QueueMutex;
    static std::condition_variable s_Condition;
    static bool s_Stopping;
};
} // namespace CHEngine

#endif // CH_TASK_SYSTEM_H
