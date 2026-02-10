#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/font_asset.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/thread_pool.h"

#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include <future>
#include <mutex>
#include <vector>

namespace CHEngine
{

    // Manages the lifecycle, loading, and caching of all engine assets.
    // Assets can be loaded synchronously or asynchronously.
    // Each instance maintains its own cache and search paths.
    class AssetManager
    {
    public: // Life Cycle
        AssetManager();
        ~AssetManager();

        // Initializes the manager with a root directory.
        void Initialize(const std::filesystem::path& rootPath = "");

        // Shuts down the manager and releases all cached assets.
        void Shutdown();

    public: // Configuration
        void SetRootPath(const std::filesystem::path& path);
        std::filesystem::path GetRootPath() const;
        
        void AddSearchPath(const std::filesystem::path& path);
        void ClearSearchPaths();

    public: // Asset Retrieval
        // Resolves a relative path to an absolute path based on search paths.
        std::string ResolvePath(const std::string& path) const;

        // Retrieves an asset of type T. If not cached, it starts loading.
        // For GPU-dependent assets (Textured, Models, Fonts), loading is synchronous.
        template <typename T>
        std::shared_ptr<T> Get(const std::string& path)
        {
            if (path.empty()) return nullptr;

            std::string resolved = ResolvePath(path);
            
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            auto& cache = GetCache<T>();
            
            // Deduplication: check cache first
            if (auto it = cache.find(resolved); it != cache.end())
            {
                return it->second;
            }

            // For assets that touch GPU: Load synchronously (Raylib uses OpenGL context)
            if constexpr (std::is_same_v<T, ShaderAsset> || 
                          std::is_same_v<T, FontAsset> || 
                          std::is_same_v<T, ModelAsset> || 
                          std::is_same_v<T, TextureAsset> ||
                          std::is_same_v<T, EnvironmentAsset>)
            {
                //CH_CORE_INFO("AssetManager: Loading {} synchronously: {}", typeid(T).name(), resolved);
                auto asset = T::Load(resolved);
                if (asset)
                {
                    // For assets that use deferred GPU upload (m_PendingData pattern)
                    if constexpr (std::is_same_v<T, ModelAsset> || std::is_same_v<T, TextureAsset>)
                    {
                        asset->UploadToGPU();
                    }
                    cache[resolved] = asset;
                }
                return asset;
            }
            else
            {
                // Check if already loading to prevent duplicate async tasks
                auto& loading = GetLoadingMap<T>();
                if (loading.contains(resolved))
                {
                    return cache[resolved]; 
                }

                //CH_CORE_INFO("AssetManager: Starting threadpool load for {} from: {}", typeid(T).name(), resolved);
                
                auto asset = std::make_shared<T>();
                asset->SetPath(resolved);
                asset->SetState(AssetState::Loading);
                cache[resolved] = asset;

                loading[resolved] = ThreadPool::Get().Enqueue([asset, resolved]() {
                    asset->LoadFromFile(resolved);
                    return asset;
                });

                return asset;
            }
        }

        // Removes an asset from the cache.
        template <typename T>
        void Remove(const std::string& path)
        {
            if (path.empty()) return;
            std::string resolved = ResolvePath(path);
            
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            GetCache<T>().erase(resolved);
            GetLoadingMap<T>().erase(resolved);
        }

        // Performs maintenance tasks like processing async load results.
        void Update();

    private: // Internal Processing
        template <typename T>
        void UpdateCache()
        {
            std::unique_lock<std::recursive_mutex> lock(m_AssetLock);
            auto& loading = GetLoadingMap<T>();
            
            std::vector<std::string> readyPaths;
            for (auto it = loading.begin(); it != loading.end(); ++it)
            {
                if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    readyPaths.push_back(it->first);
            }

            for (const auto& path : readyPaths)
            {
                auto it = loading.find(path);
                if (it == loading.end()) continue;

                auto asset = it->second.get();
                loading.erase(it);

                if (asset)
                {
                    lock.unlock();
                    asset->UploadToGPU(); 
                    lock.lock();
                    //CH_CORE_INFO("AssetManager: Async load finished for {}", path);
                }
            }
        }

    private: // Internal State Accessors (Specialized per type)
        template <typename T> std::map<std::string, std::shared_ptr<T>>& GetCache();
        template <typename T> std::map<std::string, std::future<std::shared_ptr<T>>>& GetLoadingMap();

    private: // Members
        std::filesystem::path m_RootPath;
        std::vector<std::filesystem::path> m_SearchPaths;
        mutable std::recursive_mutex m_AssetLock;

        // Per-type caches (Instance-based)
        std::map<std::string, std::shared_ptr<ModelAsset>> m_ModelCache;
        std::map<std::string, std::shared_ptr<TextureAsset>> m_TextureCache;
        std::map<std::string, std::shared_ptr<ShaderAsset>> m_ShaderCache;
        std::map<std::string, std::shared_ptr<SoundAsset>> m_SoundCache;
        std::map<std::string, std::shared_ptr<FontAsset>> m_FontCache;
        std::map<std::string, std::shared_ptr<EnvironmentAsset>> m_EnvironmentCache;

        // Loading maps
        std::map<std::string, std::future<std::shared_ptr<ModelAsset>>> m_ModelLoading;
        std::map<std::string, std::future<std::shared_ptr<TextureAsset>>> m_TextureLoading;
        std::map<std::string, std::future<std::shared_ptr<ShaderAsset>>> m_ShaderLoading;
        std::map<std::string, std::future<std::shared_ptr<SoundAsset>>> m_SoundLoading;
        std::map<std::string, std::future<std::shared_ptr<FontAsset>>> m_FontLoading;
        std::map<std::string, std::future<std::shared_ptr<EnvironmentAsset>>> m_EnvironmentLoading;
    };

    // Specializations for GetCache and GetLoadingMap
    template<> inline std::map<std::string, std::shared_ptr<ModelAsset>>& AssetManager::GetCache() { return m_ModelCache; }
    template<> inline std::map<std::string, std::shared_ptr<TextureAsset>>& AssetManager::GetCache() { return m_TextureCache; }
    template<> inline std::map<std::string, std::shared_ptr<ShaderAsset>>& AssetManager::GetCache() { return m_ShaderCache; }
    template<> inline std::map<std::string, std::shared_ptr<SoundAsset>>& AssetManager::GetCache() { return m_SoundCache; }
    template<> inline std::map<std::string, std::shared_ptr<FontAsset>>& AssetManager::GetCache() { return m_FontCache; }
    template<> inline std::map<std::string, std::shared_ptr<EnvironmentAsset>>& AssetManager::GetCache() { return m_EnvironmentCache; }

    template<> inline std::map<std::string, std::future<std::shared_ptr<ModelAsset>>>& AssetManager::GetLoadingMap() { return m_ModelLoading; }
    template<> inline std::map<std::string, std::future<std::shared_ptr<TextureAsset>>>& AssetManager::GetLoadingMap() { return m_TextureLoading; }
    template<> inline std::map<std::string, std::future<std::shared_ptr<ShaderAsset>>>& AssetManager::GetLoadingMap() { return m_ShaderLoading; }
    template<> inline std::map<std::string, std::future<std::shared_ptr<SoundAsset>>>& AssetManager::GetLoadingMap() { return m_SoundLoading; }
    template<> inline std::map<std::string, std::future<std::shared_ptr<FontAsset>>>& AssetManager::GetLoadingMap() { return m_FontLoading; }
    template<> inline std::map<std::string, std::future<std::shared_ptr<EnvironmentAsset>>>& AssetManager::GetLoadingMap() { return m_EnvironmentLoading; }
}

#endif // CH_ASSET_MANAGER_H
