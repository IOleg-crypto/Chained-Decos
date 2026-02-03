#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/audio/sound_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/font_asset.h"

namespace CHEngine
{
    std::filesystem::path AssetManager::s_RootPath;
    std::vector<std::filesystem::path> AssetManager::s_SearchPaths;
    std::recursive_mutex AssetManager::s_AssetLock;

    void AssetManager::Init()
    {
        if (s_RootPath.empty())
            s_RootPath = PROJECT_ROOT_DIR;
        CH_CORE_INFO("AssetManager: Initialized. Root: {}", s_RootPath.string());
    }

    void AssetManager::SetRootPath(const std::filesystem::path& path)
    {
        s_RootPath = path;
        CH_CORE_INFO("AssetManager: Root path changed to: {}", s_RootPath.string());
    }

    std::filesystem::path AssetManager::GetRootPath()
    {
        return s_RootPath;
    }

    void AssetManager::AddSearchPath(const std::filesystem::path& path)
    {
        std::lock_guard<std::recursive_mutex> lock(s_AssetLock);
        // Avoid duplicates
        for (const auto& p : s_SearchPaths)
        {
            if (p == path) return;
        }
        s_SearchPaths.push_back(path);
        CH_CORE_INFO("AssetManager: Added search path: {}", path.string());
    }

    void AssetManager::ClearSearchPaths()
    {
        std::lock_guard<std::recursive_mutex> lock(s_AssetLock);
        s_SearchPaths.clear();
        CH_CORE_INFO("AssetManager: Cleared search paths.");
    }

    void AssetManager::Shutdown()
    {
        CH_CORE_INFO("AssetManager: Shutting down");
        ClearSearchPaths();
        // Caches will be cleared automatically when static members destroyed
    }

    std::string AssetManager::ResolvePath(const std::string& path)
    {
        if (path.empty()) return "";
        
        std::filesystem::path p(path);
        if (p.is_absolute()) return path;

        std::string foundPath = "";

        // 1. Try registered search paths (e.g. Project Assets)
        {
            std::lock_guard<std::recursive_mutex> lock(s_AssetLock);
            for (const auto& searchPath : s_SearchPaths)
            {
                 std::filesystem::path assetPath = searchPath / p;
                 if (std::filesystem::exists(assetPath))
                 {
                     foundPath = assetPath.string();
                     break;
                 }
            }
        }
        
        // 2. Try root relative path if not found in search paths
        if (foundPath.empty())
        {
            std::filesystem::path rootRel = s_RootPath / p;
            if (std::filesystem::exists(rootRel))
                foundPath = rootRel.string();
        }

        // 3. Fallback to original path if still not found
        if (foundPath.empty())
            foundPath = path;

        // Normalize for consistency (especially on Windows)
        std::string normalized = foundPath;
#ifdef CH_PLATFORM_WINDOWS
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
#endif
        return normalized;
    }

    void AssetManager::Update()
    {
        UpdateCache<TextureAsset>();
        UpdateCache<ModelAsset>();
        UpdateCache<SoundAsset>();
        UpdateCache<ShaderAsset>();
        UpdateCache<EnvironmentAsset>();
    }

} // namespace CHEngine
