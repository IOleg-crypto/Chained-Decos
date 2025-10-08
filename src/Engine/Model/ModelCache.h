#ifndef MODELCACHE_H
#define MODELCACHE_H

#include <chrono>
#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>


// Information about cached model
struct CachedModelInfo
{
    std::unique_ptr<Model> model;
    std::chrono::steady_clock::time_point lastAccessed;
    int accessCount = 0;
    size_t memorySize = 0; // Approximate memory size
    bool isFrequentlyUsed = false;
    bool isLoading = false; // For async loading
    std::future<std::unique_ptr<Model>> loadingFuture; // Async loading future

    void UpdateAccess()
    {
        lastAccessed = std::chrono::steady_clock::now();
        accessCount++;
        isFrequentlyUsed = accessCount > 5; // Frequent use threshold
    }
};

// Async loading task
struct ModelLoadingTask
{
    std::string modelName;
    std::string filePath;
    std::promise<std::unique_ptr<Model>> promise;
    std::chrono::steady_clock::time_point requestTime;
    int priority = 0; // Higher number = higher priority

    // Comparator for priority queue (higher priority first)
    bool operator<(const ModelLoadingTask& other) const
    {
        return priority < other.priority;
    }
};

// Model cache with automatic memory management and async loading
class ModelCache
{
public:
    ModelCache() = default;
    ~ModelCache();

    // Basic cache operations
    Model *GetModel(const std::string &name);
    bool AddModel(const std::string &name, Model &&model);
    bool RemoveModel(const std::string &name);
    void Clear();

    // Async loading operations
    std::future<Model*> GetModelAsync(const std::string &name, const std::string &filePath, int priority = 0);
    bool IsModelLoading(const std::string &name) const;
    void CancelLoading(const std::string &name);

    // Preloading strategies
    void PreloadModels(const std::vector<std::string>& modelNames, const std::vector<std::string>& filePaths);
    void PreloadFrequentModels();
    void PreloadNearbyModels(const std::vector<std::string>& nearbyModelNames);

    // Cache statistics
    size_t GetCacheSize() const { return m_cache.size(); }
    size_t GetTotalMemoryUsage() const;
    float GetHitRate() const;
    size_t GetLoadingQueueSize() const { return m_loadingQueue.size(); }
    size_t GetActiveLoadingTasks() const { return m_activeTasks; }

    // Memory management
    void CleanupUnusedModels(int maxAgeSeconds = 300); // 5 minutes
    void SetMaxCacheSize(size_t maxSize) { m_maxCacheSize = maxSize; }
    void SetMaxConcurrentLoads(size_t maxConcurrent) { m_maxConcurrentLoads = maxConcurrent; }

    // Debug info
    void PrintCacheStats() const;
    void PrintLoadingStats() const;

private:
    std::unordered_map<std::string, CachedModelInfo> m_cache;
    size_t m_maxCacheSize = 50; // Maximum models in cache
    size_t m_maxConcurrentLoads = 4; // Maximum concurrent async loads

    // Async loading system
    std::vector<std::thread> m_loadingThreads;
    std::priority_queue<ModelLoadingTask> m_loadingQueue;
    std::unordered_map<std::string, std::future<std::unique_ptr<Model>>> m_activeLoadingTasks;
    std::mutex m_loadingMutex;
    std::condition_variable m_loadingCondition;
    std::atomic<bool> m_stopLoadingThreads{false};
    std::atomic<size_t> m_activeTasks{0};

    // Statistics
    mutable int m_hitCount = 0;
    mutable int m_missCount = 0;
    mutable int m_asyncLoadCount = 0;
    mutable int m_failedLoadCount = 0;

    // Private methods
    size_t EstimateModelSize(const Model &model) const;
    void EvictLeastRecentlyUsed();
    void LoadingThreadFunction();
    std::unique_ptr<Model> LoadModelFromFile(const std::string& filePath);
    void ProcessLoadingQueue();
    void UpdateLoadingTask(const std::string& name, std::future<std::unique_ptr<Model>>&& future);
};

#endif /* MODELCACHE_H */
