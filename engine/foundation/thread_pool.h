#ifndef CH_THREAD_POOL_H
#define CH_THREAD_POOL_H

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>

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
        s_Instance = new ThreadPool(workerCount);
    }

    static void Shutdown()
    {
        if (s_Instance)
        {
            s_Instance->Stop();
            delete s_Instance;
        }
    }

    static ThreadPool& Get() { return *s_Instance; }

    // Enqueues a task and returns a future for the result.
    template <class F, class... Args>
    static auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        // Пакуємо завдання у shared_ptr, щоб його можна було скопіювати всередину std::function
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(s_Instance->m_QueueMutex);

            // Не дозволяємо додавати завдання, якщо пул зупиняється
            if (s_Instance->m_Stop)
                throw std::runtime_error("Enqueue on stopped ThreadPool");

            s_Instance->m_Tasks.emplace([task]() { (*task)(); });
        }

        s_Instance->m_Condition.notify_one();
        return res;
    }

    // Queues a fire-and-forget task (без повернення результату).
    static void QueueTask(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(s_Instance->m_QueueMutex);
            if (s_Instance->m_Stop)
                throw std::runtime_error("QueueTask on stopped ThreadPool");

            s_Instance->m_Tasks.emplace(std::move(task));
        }
        s_Instance->m_Condition.notify_one();
    }

public:
    explicit ThreadPool(unsigned int workerCount) : m_Stop(false)
    {
        m_Workers.reserve(workerCount);
        for (unsigned int i = 0; i < workerCount; ++i)
        {
            m_Workers.emplace_back([this]() {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->m_QueueMutex);

                        // Чекаємо, поки з'явиться завдання АБО пул почнуть зупиняти
                        this->m_Condition.wait(lock, [this]() {
                            return this->m_Stop || !this->m_Tasks.empty();
                        });

                        // Якщо пул зупиняють і завдань більше немає — виходимо з потоку
                        if (this->m_Stop && this->m_Tasks.empty())
                            return;

                        // Забираємо завдання з черги
                        task = std::move(this->m_Tasks.front());
                        this->m_Tasks.pop();
                    }

                    // Виконуємо завдання поза м'ютексом, щоб не блокувати інші потоки
                    task();
                }
            });
        }
    }

    ~ThreadPool()
    {
        Stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    void Stop()
    {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop) return;
            m_Stop = true;
        }

        // Будимо всі потоки, щоб вони побачили m_Stop == true і завершили роботу
        m_Condition.notify_all();

        // Чекаємо завершення всіх потоків (join)
        for (std::thread& worker : m_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        m_Workers.clear();
    }

private:
    // Потоки-воркери
    std::vector<std::thread> m_Workers;

    // Черга завдань
    std::queue<std::function<void()>> m_Tasks;

    // Синхронізація
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    bool m_Stop;
private:
    static inline ThreadPool* s_Instance = nullptr;
};


} // namespace Chained

#endif // CH_THREAD_POOL_H