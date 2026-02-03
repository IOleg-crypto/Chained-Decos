#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/graphics/asset.h"
#include "engine/graphics/asset.h"
#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include <future>
#include <variant>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include "engine/core/log.h"

#include "engine/graphics/texture_asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/font_asset.h"
#include "engine/audio/sound_asset.h"

namespace CHEngine
{
    class AssetManager
    {
    public:
        static void Init();
        static void SetRootPath(const std::filesystem::path& path);
        static std::filesystem::path GetRootPath();
        
        static void AddSearchPath(const std::filesystem::path& path);
        static void ClearSearchPaths();
        
        static void Shutdown();

        static std::string ResolvePath(const std::string& path);

        template <typename T>
        static std::shared_ptr<T> Get(const std::string& path)
        {
            if (path.empty()) return nullptr;

            std::string resolved = ResolvePath(path);
            
            std::lock_guard<std::recursive_mutex> lock(s_AssetLock);
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
                CH_CORE_INFO("AssetManager: Loading {} synchronously: {}", typeid(T).name(), resolved);
                auto asset = T::Load(resolved);
                if (asset)
                {
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
                    // Asset is being loaded, return the placeholder from cache
                    return cache[resolved]; 
                }

                CH_CORE_INFO("AssetManager: Starting async load for {} from: {}", typeid(T).name(), resolved);
                
                // Create asset object first (in Loading state)
                auto asset = std::make_shared<T>();
                asset->SetPath(resolved);
                asset->SetState(AssetState::Loading);
                cache[resolved] = asset;

                // Start async load using all available cores (std::launch::async)
                loading[resolved] = std::async(std::launch::async, [asset, resolved]() {
                    asset->LoadFromFile(resolved); // Custom method for background work
                    return asset;
                });

                return asset;
            }
        }

        template <typename T>
        static void Clear(const std::string& path)
        {
            if (path.empty()) return;
            std::string resolved = ResolvePath(path);
            
            std::lock_guard<std::recursive_mutex> lock(s_AssetLock);
            GetCache<T>().erase(resolved);
            GetLoadingMap<T>().erase(resolved);
        }

        template <typename T>
        static void UpdateCache()
        {
            std::unique_lock<std::recursive_mutex> lock(s_AssetLock);
            auto& loading = GetLoadingMap<T>();
            
            // Collect ready items first to avoid iterator invalidation when releasing lock
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
                    // Release lock before GPU upload to allow other threads/nested Get() calls
                    lock.unlock();
                    asset->UploadToGPU(); 
                    lock.lock();
                    CH_CORE_INFO("AssetManager: Async load finished for {}", path);
                }
            }
        }

        static void Update();

    private:
        static std::filesystem::path s_RootPath;
        static std::vector<std::filesystem::path> s_SearchPaths;
        static std::recursive_mutex s_AssetLock;

        template <typename T>
        static std::map<std::string, std::shared_ptr<T>>& GetCache()
        {
            static std::map<std::string, std::shared_ptr<T>> cache;
            return cache;
        }

        template <typename T>
        static std::map<std::string, std::future<std::shared_ptr<T>>>& GetLoadingMap()
        {
            static std::map<std::string, std::future<std::shared_ptr<T>>> loading;
            return loading;
        }

    };
}

#endif // CH_ASSET_MANAGER_H
