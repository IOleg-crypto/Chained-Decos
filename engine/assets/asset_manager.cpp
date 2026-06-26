#include "engine/assets/asset_manager.h"
#include "engine/assets/loaders/asset_importer.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/log.h"

namespace Chained
{
    void AssetManager::Initialize()
    {
    }

    void AssetManager::Shutdown()
    {
    }

    void AssetManager::SetEngineRoot(const std::filesystem::path& path)
    {
        m_EngineRoot = path;
        m_FailedImports.clear();
        CH_CORE_INFO("AssetManager: Engine root set to '{}'", path.string());
    }

    void AssetManager::SetProjectDirectory(const std::filesystem::path& path)
    {
        m_ProjectDirectory = path;
        m_FailedImports.clear();
        CH_CORE_INFO("AssetManager: Project directory set to '{}'", path.string());
    }

    void AssetManager::SetAssetDirectory(const std::filesystem::path& path)
    {
        m_AssetDirectory = path;
        m_FailedImports.clear();
        CH_CORE_INFO("AssetManager: Asset directory set to '{}'", path.string());
    }

    std::filesystem::path AssetManager::ResolveFilePath(const std::filesystem::path& relativePath) const
    {
        // If it's already absolute and exists, use it directly
        if (relativePath.is_absolute() && std::filesystem::exists(relativePath))
            return relativePath;

        // Tier 1: EngineRoot / relativePath
        if (!m_EngineRoot.empty())
        {
            auto p = m_EngineRoot / relativePath;
            if (std::filesystem::exists(p)) return p;
        }

        // Tier 2: AssetDirectory / relativePath
        if (!m_AssetDirectory.empty())
        {
            auto p = m_AssetDirectory / relativePath;
            if (std::filesystem::exists(p)) return p;
        }

        // Tier 3: ProjectDirectory / relativePath
        if (!m_ProjectDirectory.empty())
        {
            auto p = m_ProjectDirectory / relativePath;
            if (std::filesystem::exists(p)) return p;
        }

        // Fallback: CWD-relative (original behavior)
        if (std::filesystem::exists(relativePath))
            return std::filesystem::absolute(relativePath);

        return {}; // Not found
    }

    const AssetMetadata& AssetManager::GetMetadata(AssetHandle handle) const
    {
        return m_Registry.GetMetadata(handle);
    }

    void AssetManager::SetMetadata(AssetHandle handle, const AssetMetadata& metadata)
    {
        m_Registry.SetMetadata(handle, metadata);
    }

    static AssetType DeduceAssetTypeFromExtension(const std::filesystem::path& path)
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".hdr") return AssetType::Texture;
        if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx") return AssetType::Model;
        if (ext == ".ttf" || ext == ".otf") return AssetType::Font;
        if (ext == ".chshader") return AssetType::Shader;
        if (ext == ".chenv") return AssetType::Environment;
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") return AssetType::Audio;
        
        return AssetType::None;
    }

    AssetHandle AssetManager::ImportAsset(const std::filesystem::path& filepath)
    {
        std::string key = filepath.string();

        {
            std::lock_guard<std::mutex> lock(m_AssetMutex);
            // Skip if we already know this path doesn't resolve
            if (m_FailedImports.count(key))
                return AssetHandle(0);

            // Check if already imported by key
            for (const auto& [handle, metadata] : m_Registry.GetRegistryMap())
            {
                if (metadata.FilePath.string() == key)
                    return handle;
            }
        }

        // Resolve relative path through 3-tier search (no lock needed — reads only)
        std::filesystem::path resolved = ResolveFilePath(filepath);

        std::lock_guard<std::mutex> lock(m_AssetMutex);

        if (resolved.empty())
        {
            CH_CORE_WARN("AssetManager: Cannot find file at path: {}", key);
            m_FailedImports.insert(key);
            return AssetHandle(0);
        }

        // Double-check after re-acquiring lock (another thread might have imported meanwhile)
        for (const auto& [handle, metadata] : m_Registry.GetRegistryMap())
        {
            if (metadata.FilePath == resolved || metadata.FilePath.string() == key)
                return handle;
        }

        AssetHandle handle = UUID();
        AssetMetadata metadata;
        metadata.Handle = handle;
        metadata.FilePath = resolved;
        metadata.Type = DeduceAssetTypeFromExtension(filepath);

        m_Registry.SetMetadata(handle, metadata);
        return handle;
    }


    std::shared_ptr<Asset> AssetManager::GetAssetRaw(AssetHandle handle)
    {
        {
            std::lock_guard<std::mutex> lock(m_AssetMutex);
            auto it = m_LoadedAssets.find(handle);
            if (it != m_LoadedAssets.end())
            {
                return it->second;
            }
        }

        const AssetMetadata& metadata = m_Registry.GetMetadata(handle);
        if (!metadata.IsValid())
        {
            return nullptr;
        }

        std::shared_ptr<Asset> asset = nullptr;

        switch (metadata.Type)
        {
            case AssetType::Texture:
                asset = AssetImporter::ImportTexture(handle, metadata);
                break;
            case AssetType::Model:
                asset = AssetImporter::ImportModel(handle, metadata);
                break;
            case AssetType::Font:
                asset = AssetImporter::ImportFont(handle, metadata);
                break;
            case AssetType::Shader:
                asset = AssetImporter::ImportShader(handle, metadata);
                break;
            case AssetType::Environment:
                asset = AssetImporter::ImportEnvironment(handle, metadata);
                break;
            default:
                break;
        }

        if (asset)
        {
            {
                std::lock_guard<std::mutex> lock(m_AssetMutex);
                m_LoadedAssets[handle] = asset;
            }
            // If asset has pending CPU data (e.g. texture decoded on worker thread),
            // queue it for GPU finalization on the main thread via Update().
            if (asset->GetState() == AssetState::Loading)
            {
                std::lock_guard<std::mutex> pendingLock(m_PendingMutex);
                m_PendingFinalize.push_back(asset);
            }
        }
        return asset;
    }

    AssetHandle AssetManager::ResolveToHandle(const std::filesystem::path& path, AssetType type)
    {
        // Path deduplication lookup
        for (const auto& [handle, metadata] : m_Registry.GetRegistryMap())
        {
            if (metadata.FilePath == path)
            {
                return handle;
            }
        }

        // If not found, import it
        return ImportAsset(path);
    }

    bool AssetManager::HasBackgroundWork() const
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        return !m_PendingFinalize.empty();
    }

    uint32_t AssetManager::GetPendingFinalizeCount() const
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        return static_cast<uint32_t>(m_PendingFinalize.size());
    }

    void AssetManager::Update(Timestep /*ts*/)
    {
        // Drain pending GPU finalization queue — MUST run on main thread.
        // Budget: finalize up to 8 textures per frame to avoid hitching.
        constexpr int kMaxPerFrame = 8;
        int count = 0;

        while (count < kMaxPerFrame)
        {
            std::shared_ptr<Asset> asset;
            {
                std::lock_guard<std::mutex> lock(m_PendingMutex);
                if (m_PendingFinalize.empty()) break;
                asset = m_PendingFinalize.front();
                m_PendingFinalize.pop_front();
            }

            // Texture finalization (GPU upload)
            if (auto texAsset = std::dynamic_pointer_cast<TextureAsset>(asset))
            {
                if (texAsset->HasPending())
                    texAsset->Finalize();
            }
            else
            {
                // Generic: mark as ready if state is still Loading
                if (asset->GetState() == AssetState::Loading)
                    asset->SetState(AssetState::Ready);
            }
            ++count;
        }
    }
}
