#ifndef ADVANCED_ASSET_MANAGER_H
#define ADVANCED_ASSET_MANAGER_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <raylib.h>

struct AssetMetadata {
    std::string filePath;
    std::string assetType;
    size_t fileSize;
    std::chrono::system_clock::time_point lastModified;
    std::chrono::system_clock::time_point loadTime;
    std::unordered_map<std::string, std::string> properties;
    std::vector<std::string> dependencies;
    bool compressed;
    size_t originalSize;
    size_t compressedSize;

    AssetMetadata();
    void UpdateFileInfo(const std::string& path);
    bool IsOutdated() const;
};

struct AssetDependency {
    std::string assetName;
    std::string dependencyName;
    std::string dependencyType; // "hard" or "soft"

    AssetDependency(const std::string& asset, const std::string& dep, const std::string& type = "hard");
};

class AssetLoadJob {
public:
    enum class Status {
        PENDING,
        LOADING,
        COMPLETED,
        FAILED,
        CANCELLED
    };

    AssetLoadJob(const std::string& name, const std::string& path, std::function<bool()> loadFunc);

    void Execute();
    bool IsCompleted() const { return m_status == Status::COMPLETED; }
    bool IsFailed() const { return m_status == Status::FAILED; }
    std::string GetError() const { return m_error; }

    std::string m_name;
    std::string m_path;
    Status m_status;
    std::function<bool()> m_loadFunction;
    std::string m_error;
    float m_progress;
    std::chrono::steady_clock::time_point m_startTime;
};

class AdvancedAssetManager {
public:
    static AdvancedAssetManager& GetInstance();

    // Asset registration and management
    bool RegisterAsset(const std::string& name, const std::string& filePath,
                      const std::string& assetType, const std::vector<std::string>& dependencies = {});
    bool UnregisterAsset(const std::string& name);
    bool IsAssetRegistered(const std::string& name) const;

    // Asset loading with advanced features
    bool LoadAsset(const std::string& name, bool async = false);
    bool LoadAssets(const std::vector<std::string>& names, bool async = false);
    bool UnloadAsset(const std::string& name);
    bool ReloadAsset(const std::string& name);

    // Dependency management
    void AddDependency(const std::string& assetName, const std::string& dependencyName,
                      const std::string& depType = "hard");
    void RemoveDependency(const std::string& assetName, const std::string& dependencyName);
    std::vector<std::string> GetDependencies(const std::string& assetName) const;
    std::vector<std::string> GetDependentAssets(const std::string& assetName) const;

    // Asset validation and optimization
    bool ValidateAsset(const std::string& name);
    bool OptimizeAsset(const std::string& name);
    bool CompressAsset(const std::string& name);

    // Hot-reloading system
    void EnableHotReloading(bool enable);
    bool IsHotReloadingEnabled() const { return m_hotReloading; }
    void CheckForAssetChanges();
    std::vector<std::string> GetModifiedAssets() const;

    // Asynchronous loading
    void UpdateAsyncLoading();
    bool IsAsyncLoading() const { return !m_loadJobs.empty(); }
    float GetOverallLoadingProgress() const;
    void CancelAllLoading();

    // Asset caching and memory management
    void SetCacheSize(size_t maxSize);
    size_t GetCacheSize() const { return m_maxCacheSize; }
    size_t GetCurrentCacheSize() const;
    void ClearCache();
    void OptimizeCache();

    // Asset search and querying
    std::vector<std::string> FindAssetsByType(const std::string& assetType) const;
    std::vector<std::string> FindAssetsByProperty(const std::string& property, const std::string& value) const;
    std::vector<std::string> FindAssetsBySize(size_t minSize, size_t maxSize) const;

    // Asset statistics and reporting
    struct AssetStats {
        int totalAssets;
        int loadedAssets;
        int failedAssets;
        size_t totalSize;
        size_t cacheSize;
        float averageLoadTime;
        int dependencyCount;
        std::unordered_map<std::string, int> assetsByType;
    };

    AssetStats GetAssetStatistics() const { return m_stats; }
    void ExportAssetReport(const std::string& filename) const;

    // Asset lifecycle callbacks
    void SetAssetLoadCallback(std::function<void(const std::string&)> callback);
    void SetAssetUnloadCallback(std::function<void(const std::string&)> callback);
    void SetAssetErrorCallback(std::function<void(const std::string&, const std::string&)> callback);

private:
    AdvancedAssetManager();
    ~AdvancedAssetManager() = default;
    AdvancedAssetManager(const AdvancedAssetManager&) = delete;
    AdvancedAssetManager& operator=(const AdvancedAssetManager&) = delete;

    // Internal methods
    bool LoadAssetInternal(const std::string& name);
    void ProcessDependencies(const std::string& assetName);
    void UpdateAssetMetadata(const std::string& name);
    AssetLoadJob* CreateLoadJob(const std::string& name);
    void ExecuteLoadJob(AssetLoadJob* job);

    // Asset storage
    std::unordered_map<std::string, AssetMetadata> m_assetMetadata;
    std::unordered_map<std::string, std::shared_ptr<void>> m_loadedAssets;
    std::vector<std::string> m_assetList;

    // Dependency tracking
    std::unordered_map<std::string, std::vector<AssetDependency>> m_dependencies;
    std::unordered_map<std::string, std::vector<std::string>> m_reverseDependencies;

    // Asynchronous loading
    std::vector<std::unique_ptr<AssetLoadJob>> m_loadJobs;
    std::unordered_map<std::string, std::unique_ptr<AssetLoadJob>> m_activeJobs;
    int m_maxConcurrentJobs;

    // Hot-reloading
    bool m_hotReloading;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> m_lastCheckTimes;

    // Caching
    size_t m_maxCacheSize;
    size_t m_currentCacheSize;
    std::unordered_map<std::string, size_t> m_assetSizes;

    // Statistics
    AssetStats m_stats;
    std::vector<float> m_loadTimes;

    // Callbacks
    std::function<void(const std::string&)> m_loadCallback;
    std::function<void(const std::string&)> m_unloadCallback;
    std::function<void(const std::string&, const std::string&)> m_errorCallback;

    // Constants
    static const int DEFAULT_MAX_CONCURRENT_JOBS = 4;
    static const size_t DEFAULT_CACHE_SIZE = 512 * 1024 * 1024; // 512 MB
};

#endif // ADVANCED_ASSET_MANAGER_H