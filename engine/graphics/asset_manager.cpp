#include "engine/graphics/asset_manager.h"
#include "engine/core/log.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/shader_importer.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/environment_importer.h"
#include "engine/graphics/font_asset.h"
#include "engine/graphics/font_importer.h"
#include "engine/audio/sound_asset.h"
#include "engine/audio/audio_importer.h"
#include "engine/graphics/texture_importer.h"
#include "engine/graphics/mesh_importer.h"

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
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        m_AssetCaches.clear();
        m_AssetMetadata.clear();
    }

    void AssetManager::SetRootPath(const std::filesystem::path& path)
    {
        m_RootPath = path;
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
        std::string pStr = p.generic_string();
        if (!pStr.empty() && (pStr[0] == '/' || pStr[0] == '\\'))
            pStr = pStr.substr(1);

        std::filesystem::path normalizedP(pStr);

        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            for (const auto& searchPath : m_SearchPaths)
            {
                 std::filesystem::path assetPath = searchPath / normalizedP;
                 if (std::filesystem::exists(assetPath))
                 {
                     foundPath = assetPath.string();
                     break;
                 }
            }
        }
        
        if (foundPath.empty())
        {
            std::filesystem::path rootRel = m_RootPath / normalizedP;
            if (std::filesystem::exists(rootRel))
                foundPath = rootRel.string();
        }

        if (foundPath.empty())
            foundPath = path;

        std::string normalized = foundPath;
        #ifdef CH_PLATFORM_WINDOWS
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        #endif

        return normalized;
    }

    std::shared_ptr<Asset> AssetManager::GetAsset(const std::string& path, AssetType type)
    {
        if (path.empty() || type == AssetType::None) return nullptr;

        std::string resolved = ResolvePath(path);
        
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        auto& cache = m_AssetCaches[type];
        
        if (auto it = cache.find(resolved); it != cache.end())
        {
            return it->second;
        }

        std::shared_ptr<Asset> asset = nullptr;

        switch (type)
        {
            case AssetType::Texture:
                asset = std::static_pointer_cast<Asset>(TextureImporter::ImportTexture(resolved));
                break;
            case AssetType::Model:
                asset = std::static_pointer_cast<Asset>(MeshImporter::ImportMesh(resolved));
                break;
            case AssetType::Shader:
                asset = std::static_pointer_cast<Asset>(ShaderImporter::ImportShader(resolved));
                break;
            case AssetType::Font:
                asset = std::static_pointer_cast<Asset>(FontImporter::ImportFont(resolved));
                break;
            case AssetType::Environment:
                asset = std::static_pointer_cast<Asset>(EnvironmentImporter::ImportEnvironment(resolved));
                break;
            case AssetType::Audio:
                asset = std::static_pointer_cast<Asset>(AudioImporter::ImportSound(resolved));
                break;
            default:
                CH_CORE_ERROR("AssetManager: Unknown asset type for path: {}", resolved);
                return nullptr;
        }

        if (asset)
        {
            AssetMetadata metadata;
            metadata.Handle = asset->GetID();
            metadata.FilePath = resolved;
            metadata.Type = type;
            
            m_AssetMetadata[metadata.Handle] = metadata;
            cache[resolved] = asset;
        }

        return asset;
    }

    std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle, AssetType type)
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_AssetMetadata.find(handle); it != m_AssetMetadata.end())
        {
            return GetAsset(it->second.FilePath, type);
        }
        return nullptr;
    }

    void AssetManager::RemoveAsset(const std::string& path, AssetType type)
    {
        if (path.empty() || type == AssetType::None) return;
        std::string resolved = ResolvePath(path);
        
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (m_AssetCaches.count(type))
        {
            auto& cache = m_AssetCaches[type];
            if (auto it = cache.find(resolved); it != cache.end())
            {
                m_AssetMetadata.erase(it->second->GetID());
                cache.erase(it);
            }
        }
    }

    void AssetManager::Update()
    {
        // For now, mostly synchronous, but we can add async processing here
    }

    const AssetMetadata& AssetManager::GetMetadata(AssetHandle handle) const
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        auto it = m_AssetMetadata.find(handle);
        if (it == m_AssetMetadata.end())
        {
            static AssetMetadata s_EmptyMetadata;
            return s_EmptyMetadata;
        }
        return it->second;
    }

} // namespace CHEngine
