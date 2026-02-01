#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/graphics/asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include <future>
#include <variant>
#include <unordered_map>
#include "engine/core/log.h"
#include "engine/scene/project.h"

// Forward declarations or includes for template specialization in Update()
namespace CHEngine {
    class TextureAsset;
    class ModelAsset;
    class SoundAsset;
    class ShaderAsset;
    class EnvironmentAsset;
    class FontAsset;
}

namespace CHEngine
{
    class AssetManager
    {
    public:
        static void Init()
        {
            if (s_RootPath.empty())
                s_RootPath = PROJECT_ROOT_DIR;
            CH_CORE_INFO("AssetManager: Initialized. Root: {}", s_RootPath.string());
        }

        static void SetRootPath(const std::filesystem::path& path)
        {
            s_RootPath = path;
            CH_CORE_INFO("AssetManager: Root path changed to: {}", s_RootPath.string());
        }

        static std::filesystem::path GetRootPath()
        {
            return s_RootPath;
        }

        static void Shutdown()
        {
            CH_CORE_INFO("AssetManager: Shutting down");
            // Caches will be cleared automatically when static members destroyed
        }

        static std::string ResolvePath(const std::string& path)
        {
            if (path.empty()) return "";
            if (path.starts_with(":")) return path; // Procedural

            // Handle virtual prefixes (engine: or engine/)
            if (path.starts_with("engine:") || path.starts_with("engine/"))
            {
                std::string sub = path.substr(7);
                std::filesystem::path res = s_RootPath / "engine" / "resources" / sub;
                
                // Fallback to non-resource location for legacy compatibility 
                if (!std::filesystem::exists(res))
                {
                    std::filesystem::path legacy = s_RootPath / "engine" / sub;
                    if (std::filesystem::exists(legacy)) return legacy.string();
                }
                return res.string();
            }

            std::filesystem::path p(path);
            if (p.is_absolute()) return path;

            // Try relative to active project assets
            if (Project::GetActive())
            {
                std::filesystem::path assetPath = Project::GetAssetPath(p);
                if (std::filesystem::exists(assetPath))
                    return assetPath.string();
            }

            // Try relative to project root (system-wide resources)
            std::filesystem::path rootRel = s_RootPath / p;
            if (std::filesystem::exists(rootRel))
                return rootRel.string();

            return path;
        }

        template <typename T>
        static std::shared_ptr<T> Get(const std::string& path)
        {
            if (path.empty()) return nullptr;

            std::string resolved = ResolvePath(path);
            
            auto& cache = GetCache<T>();
            
            // Deduplication: check cache first
            if (auto it = cache.find(resolved); it != cache.end())
            {
                return it->second;
            }

            // For ModelAsset and ShaderAsset: Load synchronously (Raylib uses GPU)
            if constexpr (std::is_same_v<T, ModelAsset> || std::is_same_v<T, ShaderAsset>)
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
        static void UpdateCache()
        {
            auto& loading = GetLoadingMap<T>();
            for (auto it = loading.begin(); it != loading.end();)
            {
                if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    auto asset = it->second.get();
                    if (asset)
                    {
                        asset->UploadToGPU(); 
                        CH_CORE_INFO("AssetManager: Async load finished for {}", it->first);
                    }
                    it = loading.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        static void Update();

    private:
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

        inline static std::filesystem::path s_RootPath;
    };
}

#endif // CH_ASSET_MANAGER_H
