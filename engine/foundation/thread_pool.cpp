#include "thread_pool.h"

namespace Chained {
    std::unique_ptr<ThreadPool> ThreadPool::s_Instance = nullptr;
}
