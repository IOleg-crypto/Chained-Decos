#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include <condition_variable>
#include "engine/core/engine_service.h"
#include "engine/core/service_locator.h"
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace CHEngine
{
// Thread pool managing a fixed set of worker threads for parallel task execution.
class ThreadPool : public EngineService
{
public:
    ThreadPool() = default;
    ~ThreadPool() override = default;

    // Returns the global ThreadPool instance via ServiceLocator.
    static ThreadPool& Get()
    {
        return ServiceLocator::Get<ThreadPool>();
    }

    // Deleted copy/move to maintain singleton semantics
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Enqueues a task and returns a future for the result.
    template <class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop)
                throw std::runtime_error("Enqueue on stopped ThreadPool");

            m_Tasks.emplace([task]() { (*task)(); });
        }
        m_Condition.notify_one();
        return res;
    }

    // Queues a fire-and-forget task.
    void QueueTask(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Tasks.emplace(std::move(task));
        }
        m_Condition.notify_one();
    }

protected:
    void OnInit() override;
    void OnShutdown() override;

private:
    void StartWorkers();
    void StopWorkers();
    void WorkerThread(std::stop_token stopToken);

private:
    std::queue<std::function<void()>> m_Tasks;
    std::mutex m_QueueMutex;
    std::condition_variable_any m_Condition;
    bool m_Stop = false;
    std::vector<std::jthread> m_Workers;
};
} // namespace CHEngine

#endif // CH_THREAD_POOL_H
