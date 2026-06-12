#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include <BS_thread_pool.hpp>
#include <future>
#include <memory>

namespace Chained
{
class ThreadPool
{
public:
    static void Init()
    {
        unsigned int threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 1;
        unsigned int workerCount = (threads > 1) ? (threads - 1) : 1;
        s_Instance = std::make_unique<ThreadPool>(workerCount);
    }

    static void Shutdown()
    {
        if (s_Instance)
        {
            s_Instance->m_Pool.wait();
            s_Instance.reset();
        }
    }

    static ThreadPool& Get() { return *s_Instance; }

    // Enqueues a task and returns a future for the result.
    template <class F, class... Args>
    static auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        return s_Instance->m_Pool.submit_task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }

    // Queues a fire-and-forget task.
    static void QueueTask(std::function<void()> task)
    {
        s_Instance->m_Pool.detach_task(std::move(task));
    }

public:
    explicit ThreadPool(unsigned int workerCount)
    {
        m_Pool.reset(workerCount);
    }
    ~ThreadPool() = default;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    BS::thread_pool m_Pool;
    static std::unique_ptr<ThreadPool> s_Instance;
};
} // namespace Chained

#endif // CH_THREAD_POOL_H
