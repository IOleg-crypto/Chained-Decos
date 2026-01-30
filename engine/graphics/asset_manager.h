#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/graphics/asset.h"
#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include "engine/core/log.h"
#include "engine/scene/project.h"

namespace CHEngine
{
    class AssetManager
    {
    public:
        static void Init()
        {
            CH_CORE_INFO("AssetManager: Initialized. Root: {}", PROJECT_ROOT_DIR);
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

            std::filesystem::path p(path);
            
            // Handle virtual prefixes
            if (path.starts_with("engine:"))
            {
                std::string sub = path.substr(7);
                return (std::filesystem::path(PROJECT_ROOT_DIR) / "engine/resources" / sub).string();
            }

            if (p.is_absolute()) return path;

            // Try relative to active project assets
            if (Project::GetActive())
            {
                std::filesystem::path assetPath = Project::GetAssetPath(p);
                if (std::filesystem::exists(assetPath))
                    return assetPath.string();
            }

            // Try relative to project root (system-wide resources)
            std::filesystem::path rootRel = std::filesystem::path(PROJECT_ROOT_DIR) / p;
            if (std::filesystem::exists(rootRel))
                return rootRel.string();

            CH_CORE_WARN("AssetManager: Could not resolve path: {}", path);
            return path;
        }

        template <typename T>
        static std::shared_ptr<T> Get(const std::string& path)
        {
            if (path.empty()) return nullptr;

            std::string resolved = ResolvePath(path);
            
            auto& cache = GetCache<T>();
            if (auto it = cache.find(resolved); it != cache.end())
            {
                return it->second;
            }

            CH_CORE_INFO("AssetManager: Loading {} from: {}", typeid(T).name(), resolved);
            
            auto asset = T::Load(resolved);
            if (asset)
            {
                cache[resolved] = asset;
                CH_CORE_INFO("AssetManager: Successfully loaded {}", resolved);
            }
            else
            {
                CH_CORE_ERROR("AssetManager: FAILED to load {}", resolved);
            }
            return asset;
        }

    private:
        template <typename T>
        static std::map<std::string, std::shared_ptr<T>>& GetCache()
        {
            static std::map<std::string, std::shared_ptr<T>> cache;
            return cache;
        }
    };
}

#endif // CH_ASSET_MANAGER_H
