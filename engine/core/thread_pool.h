#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include "engine/core/base.h"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

namespace CHEngine
{
    // A simple thread pool for executing generic tasks.
    // Useful for physics, asset loading, and other parallel operations.
    class ThreadPool
    {
    public:
        ThreadPool(size_t threads = std::thread::hardware_concurrency());
        ~ThreadPool();

        static ThreadPool& Get();

        // Enqueues a task for execution. Returns a future for the result.
        template<class F, class... Args>
        auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
        {
            using return_type = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                if (m_Stop)
                    throw std::runtime_error("enqueue on stopped ThreadPool");

                m_Tasks.emplace([task](){ (*task)(); });
            }
            m_Condition.notify_one();
            return res;
        }

        void Shutdown();

    private:
        // Worker thread function
        void WorkerThread();

    private:
        std::vector<std::thread> m_Workers;
        std::queue<std::function<void()>> m_Tasks;
        
        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        bool m_Stop = false;
    };
}

#endif // CH_THREAD_POOL_H
