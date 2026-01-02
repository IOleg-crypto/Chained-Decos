#include "core/log.h"
#include "model_cache.h"
#include "core/log.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <raylib.h>


// CachedModelInfo struct implementation
void CachedModelInfo::UpdateAccess()
{
    lastAccessed = std::chrono::steady_clock::now();
    accessCount++;
    isFrequentlyUsed = accessCount > 5; // Frequent use threshold
}

// ModelLoadingTask struct implementation
bool ModelLoadingTask::operator<(const ModelLoadingTask &other) const
{
    return priority < other.priority;
}

ModelCache::~ModelCache()
{
    // Stop loading threads
    m_stopLoadingThreads = true;
    m_loadingCondition.notify_all();

    // Wait for threads to finish
    for (auto &thread : m_loadingThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    Clear();
}

Model *ModelCache::GetModel(const std::string &name)
{
    auto it = m_cache.find(name);
    if (it != m_cache.end())
    {
        it->second.UpdateAccess();
        m_hitCount++;
        return it->second.model.get();
    }

    m_missCount++;
    return nullptr;
}

bool ModelCache::AddModel(const std::string &name, Model &&model)
{
    // Check if we exceed the limit
    if (m_cache.size() >= m_maxCacheSize)
    {
        EvictLeastRecentlyUsed();
    }

    auto info = std::make_unique<CachedModelInfo>();
    info->model = std::make_unique<Model>(std::move(model));
    info->memorySize = EstimateModelSize(*info->model);
    info->UpdateAccess();

    m_cache[name] = std::move(*info);

    CD_CORE_INFO("Added model '%s' to cache (size: %zu KB)", name.c_str(), info->memorySize / 1024);

    return true;
}

bool ModelCache::RemoveModel(const std::string &name)
{
    auto it = m_cache.find(name);
    if (it != m_cache.end())
    {
        // Free Raylib resources
        if (it->second.model)
        {
            UnloadModel(*it->second.model);
        }
        m_cache.erase(it);
        CD_CORE_INFO("Removed model '%s' from cache", name.c_str());
        return true;
    }
    return false;
}

void ModelCache::Clear()
{
    for (auto &[name, info] : m_cache)
    {
        if (info.model)
        {
            UnloadModel(*info.model);
        }
    }
    m_cache.clear();
    CD_CORE_INFO("Model cache cleared");
}

size_t ModelCache::GetTotalMemoryUsage() const
{
    size_t total = 0;
    for (const auto &[name, info] : m_cache)
    {
        total += info.memorySize;
    }
    return total;
}

float ModelCache::GetHitRate() const
{
    int total = m_hitCount + m_missCount;
    return total > 0 ? (float)m_hitCount / total : 0.0f;
}

// ==================== ASYNC LOADING METHODS ====================

std::future<Model *> ModelCache::GetModelAsync(const std::string &name, const std::string &filePath,
                                               int priority)
{
    // Check if already cached
    auto it = m_cache.find(name);
    if (it != m_cache.end() && !it->second.isLoading)
    {
        it->second.UpdateAccess();
        m_hitCount++;
        std::promise<Model *> promise;
        promise.set_value(it->second.model.get());
        return promise.get_future();
    }

    // Check if already loading
    auto loadingIt = m_activeLoadingTasks.find(name);
    if (loadingIt != m_activeLoadingTasks.end())
    {
        // Return existing future
        std::promise<Model *> promise;
        auto future = promise.get_future();

        // Chain the existing loading future
        std::thread(
            [existingFuture = std::move(loadingIt->second), promise = std::move(promise)]() mutable
            {
                try
                {
                    auto model = existingFuture.get();
                    promise.set_value(model.get());
                }
                catch (const std::exception &e)
                {
                    promise.set_exception(std::current_exception());
                }
            })
            .detach();

        return promise.get_future();
    }

    // Start new async loading
    m_missCount++;
    m_asyncLoadCount++;

    std::promise<std::unique_ptr<Model>> loadingPromise;
    auto loadingFuture = loadingPromise.get_future();

    // Create loading task
    ModelLoadingTask task;
    task.modelName = name;
    task.filePath = filePath;
    task.promise = std::move(loadingPromise);
    task.requestTime = std::chrono::steady_clock::now();
    task.priority = priority;

    {
        std::lock_guard<std::mutex> lock(m_loadingMutex);
        m_loadingQueue.push(std::move(task));
        m_activeLoadingTasks[name] = std::move(loadingFuture);

        // Mark as loading in cache
        auto cacheIt = m_cache.find(name);
        if (cacheIt != m_cache.end())
        {
            cacheIt->second.isLoading = true;
        }
        else
        {
            CachedModelInfo info;
            info.isLoading = true;
            m_cache[name] = std::move(info);
        }
    }

    m_loadingCondition.notify_one();

    // Return future that resolves to Model*
    std::promise<Model *> resultPromise;
    auto resultFuture = resultPromise.get_future();

    std::thread(
        [loadingFuture = std::move(loadingFuture), resultPromise = std::move(resultPromise), name,
         this]() mutable
        {
            try
            {
                auto model = loadingFuture.get();
                resultPromise.set_value(model.get());

                // Update cache with loaded model
                std::lock_guard<std::mutex> lock(m_loadingMutex);
                auto it = m_cache.find(name);
                if (it != m_cache.end())
                {
                    it->second.model = std::move(model);
                    it->second.isLoading = false;
                    it->second.memorySize = EstimateModelSize(*it->second.model);
                    it->second.UpdateAccess();
                }
            }
            catch (const std::exception &e)
            {
                std::lock_guard<std::mutex> lock(m_loadingMutex);
                m_failedLoadCount++;
                auto it = m_cache.find(name);
                if (it != m_cache.end())
                {
                    it->second.isLoading = false;
                }
                m_activeLoadingTasks.erase(name);
                resultPromise.set_exception(std::current_exception());
            }
        })
        .detach();

    return resultFuture;
}

bool ModelCache::IsModelLoading(const std::string &name) const
{
    auto it = m_cache.find(name);
    return (it != m_cache.end()) && it->second.isLoading;
}

void ModelCache::CancelLoading(const std::string &name)
{
    std::lock_guard<std::mutex> lock(m_loadingMutex);
    m_activeLoadingTasks.erase(name);

    auto it = m_cache.find(name);
    if (it != m_cache.end())
    {
        it->second.isLoading = false;
    }
}

void ModelCache::PreloadModels(const std::vector<std::string> &modelNames,
                               const std::vector<std::string> &filePaths)
{
    if (modelNames.size() != filePaths.size())
    {
        CD_CORE_ERROR("PreloadModels: modelNames and filePaths size mismatch");
        return;
    }

    for (size_t i = 0; i < modelNames.size(); ++i)
    {
        // High priority for preloading
        GetModelAsync(modelNames[i], filePaths[i], 10);
    }

    CD_CORE_INFO("Started preloading %zu models", modelNames.size());
}

void ModelCache::PreloadFrequentModels()
{
    std::vector<std::pair<std::string, int>> usageStats;

    for (const auto &[name, info] : m_cache)
    {
        if (!info.isFrequentlyUsed && info.model)
        {
            usageStats.push_back({name, info.accessCount});
        }
    }

    // Sort by usage count (descending)
    std::sort(usageStats.begin(), usageStats.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    // Preload top 5 most used models that aren't frequently used yet
    size_t preloadCount = std::min(size_t(5), usageStats.size());
    for (size_t i = 0; i < preloadCount; ++i)
    {
        const auto &[name, count] = usageStats[i];
        CD_CORE_INFO("Preloading frequently used model: %s (accessed %d times)", name.c_str(),
                     count);

        // We don't have file paths here, so we'll need to get them from somewhere else
        // For now, just mark as frequently used
        auto it = m_cache.find(name);
        if (it != m_cache.end())
        {
            it->second.isFrequentlyUsed = true;
        }
    }
}

void ModelCache::PreloadNearbyModels(const std::vector<std::string> &nearbyModelNames)
{
    for (const auto &name : nearbyModelNames)
    {
        // Medium priority for nearby models
        GetModelAsync(name, "", 5);
    }

    CD_CORE_INFO("Started preloading %zu nearby models", nearbyModelNames.size());
}

void ModelCache::PrintLoadingStats() const
{
    CD_CORE_INFO("=== Model Loading Statistics ===");
    CD_CORE_INFO("Active loading tasks: %zu", m_activeTasks.load());
    CD_CORE_INFO("Loading queue size: %zu", m_loadingQueue.size());
    CD_CORE_INFO("Async loads requested: %d", m_asyncLoadCount);
    CD_CORE_INFO("Failed loads: %d", m_failedLoadCount);
    CD_CORE_INFO("Cache hit rate: %.1f%%", GetHitRate() * 100);
}

void ModelCache::CleanupUnusedModels(int maxAgeSeconds)
{
    auto now = std::chrono::steady_clock::now();
    auto maxAge = std::chrono::seconds(maxAgeSeconds);

    auto it = m_cache.begin();
    while (it != m_cache.end())
    {
        auto age = now - it->second.lastAccessed;

        // Don't delete frequently used models
        if (!it->second.isFrequentlyUsed && age > maxAge)
        {
            CD_CORE_INFO("Evicting unused model '%s' (age: %d seconds)", it->first.c_str(),
                         (int)std::chrono::duration_cast<std::chrono::seconds>(age).count());

            if (it->second.model)
            {
                UnloadModel(*it->second.model);
            }
            it = m_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ModelCache::PrintCacheStats() const
{
    CD_CORE_INFO("=== Model Cache Statistics ===");
    CD_CORE_INFO("Cache size: %zu/%zu models", m_cache.size(), m_maxCacheSize);
    CD_CORE_INFO("Memory usage: %.2f MB", GetTotalMemoryUsage() / (1024.0f * 1024.0f));
    CD_CORE_INFO("Hit rate: %.1f%% (%d hits, %d misses)", GetHitRate() * 100, m_hitCount,
                 m_missCount);

    // Top 5 most frequently used models
    std::vector<std::pair<std::string, int>> usage;
    for (const auto &[name, info] : m_cache)
    {
        usage.push_back({name, info.accessCount});
    }

    std::sort(usage.begin(), usage.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    CD_CORE_INFO("Top models by usage:");
    for (int i = 0; i < std::min(5, (int)usage.size()); i++)
    {
        CD_CORE_INFO("  %d. %s (%d accesses)", i + 1, usage[i].first.c_str(), usage[i].second);
    }
}

// Private methods
size_t ModelCache::EstimateModelSize(const Model &model) const
{
    size_t size = 0;

    // Approximate model size estimation
    for (int i = 0; i < model.meshCount; i++)
    {
        const Mesh &mesh = model.meshes[i];
        size += mesh.vertexCount * 3 * sizeof(float);            // vertices
        size += mesh.vertexCount * 3 * sizeof(float);            // normals
        size += mesh.vertexCount * 2 * sizeof(float);            // texcoords
        size += mesh.triangleCount * 3 * sizeof(unsigned short); // indices
    }

    return size;
}

void ModelCache::EvictLeastRecentlyUsed()
{
    if (m_cache.empty())
        return;

    // Find the least recently used model
    auto oldest = std::min_element(m_cache.begin(), m_cache.end(), [](const auto &a, const auto &b)
                                   { return a.second.lastAccessed < b.second.lastAccessed; });

    if (oldest != m_cache.end())
    {
        CD_CORE_INFO("Evicting LRU model: %s", oldest->first.c_str());
        RemoveModel(oldest->first);
    }
}

// ==================== PRIVATE ASYNC METHODS ====================

std::unique_ptr<Model> ModelCache::LoadModelFromFile(const std::string &filePath)
{
    if (filePath.empty())
    {
        throw std::runtime_error("Empty file path");
    }

    // Check if file exists
    std::ifstream file(filePath);
    if (!file.good())
    {
        throw std::runtime_error("Model file not found: " + filePath);
    }

    // Load model using Raylib
    auto model = std::make_unique<Model>();
    *model = LoadModel(filePath.c_str());

    if (model->meshCount == 0)
    {
        throw std::runtime_error("Failed to load model: " + filePath);
    }

    CD_CORE_INFO("Successfully loaded model from: %s", filePath.c_str());
    return model;
}

void ModelCache::ProcessLoadingQueue()
{
    while (!m_stopLoadingThreads)
    {
        ModelLoadingTask task;

        {
            std::unique_lock<std::mutex> lock(m_loadingMutex);
            m_loadingCondition.wait(lock, [this]()
                                    { return !m_loadingQueue.empty() || m_stopLoadingThreads; });

            if (m_stopLoadingThreads)
                break;

            if (m_loadingQueue.empty())
                continue;

            // Check if we have too many active tasks
            if (m_activeTasks >= m_maxConcurrentLoads)
            {
                continue;
            }

            // Get highest priority task
            task = std::move(const_cast<ModelLoadingTask &>(m_loadingQueue.top()));
            m_loadingQueue.pop();
            m_activeTasks++;
        }

        try
        {
            // Load the model
            auto model = LoadModelFromFile(task.filePath);

            // Set the promise value
            task.promise.set_value(std::move(model));

            CD_CORE_INFO("Async loaded model: %s", task.modelName.c_str());
        }
        catch (const std::exception &e)
        {
            CD_CORE_ERROR("Failed to async load model %s: %s", task.modelName.c_str(), e.what());
            task.promise.set_exception(std::current_exception());
        }

        {
            std::lock_guard<std::mutex> lock(m_loadingMutex);
            m_activeTasks--;
        }
    }
}

void ModelCache::UpdateLoadingTask(const std::string &name,
                                   std::future<std::unique_ptr<Model>> &&future)
{
    std::lock_guard<std::mutex> lock(m_loadingMutex);
    m_activeLoadingTasks[name] = std::move(future);
}
#include "core/log.h"

