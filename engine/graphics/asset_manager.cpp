#include "engine/graphics/asset_manager.h"
#include "engine/core/log.h"
#include <algorithm>

namespace CHEngine
{
    AssetManager::AssetManager()
    {
    }

    AssetManager::~AssetManager()
    {
        Shutdown();
    }

    void AssetManager::Initialize(const std::filesystem::path& rootPath)
    {
        CH_CORE_INFO("AssetManager: Initializing...");
        
        if (rootPath.empty())
        {
            #ifdef PROJECT_ROOT_DIR
            m_RootPath = PROJECT_ROOT_DIR;
            #else
            m_RootPath = std::filesystem::current_path();
            #endif
        }
        else
        {
            m_RootPath = rootPath;
        }
        
        CH_CORE_INFO("AssetManager: Initialized. Root: {}", m_RootPath.string());
    }

    void AssetManager::Shutdown()
    {
        CH_CORE_INFO("AssetManager: Shutting down...");
        ClearSearchPaths();
        // Static caches are naturally preserved but could be explicitly cleared if needed.
        // In an instance-based world, these static caches in the template methods
        // should ideally be members, but since GetCache is static within a template,
        // it remains shared. Moving them to actual members requires a different approach
        // for template-based dispatching without a full Type-Erasure system.
    }

    void AssetManager::SetRootPath(const std::filesystem::path& path)
    {
        m_RootPath = path;
        CH_CORE_INFO("AssetManager: Root path changed to: {}", m_RootPath.string());
    }

    std::filesystem::path AssetManager::GetRootPath() const
    {
        return m_RootPath;
    }

    void AssetManager::AddSearchPath(const std::filesystem::path& path)
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        
        auto it = std::find(m_SearchPaths.begin(), m_SearchPaths.end(), path);
        if (it != m_SearchPaths.end()) return;
        
        m_SearchPaths.push_back(path);
        CH_CORE_INFO("AssetManager: Added search path: {}", path.string());
    }

    void AssetManager::ClearSearchPaths()
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        m_SearchPaths.clear();
        CH_CORE_INFO("AssetManager: Cleared search paths.");
    }

    std::string AssetManager::ResolvePath(const std::string& path) const
    {
        if (path.empty()) return "";
        
        std::filesystem::path p(path);
        if (p.is_absolute()) return path;

        std::string foundPath = "";

        // 1. Try registered search paths
        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            for (const auto& searchPath : m_SearchPaths)
            {
                 std::filesystem::path assetPath = searchPath / p;
                 if (std::filesystem::exists(assetPath))
                 {
                     foundPath = assetPath.string();
                     break;
                 }
            }
        }
        
        // 2. Try root relative path
        if (foundPath.empty())
        {
            std::filesystem::path rootRel = m_RootPath / p;
            if (std::filesystem::exists(rootRel))
                foundPath = rootRel.string();
        }

        // 3. Fallback
        if (foundPath.empty())
            foundPath = path;

        // Normalize
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
