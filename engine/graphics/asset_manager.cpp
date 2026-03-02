#include "engine/graphics/asset_manager.h"
#include "engine/audio/audio_importer.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/constants.h"
#include "engine/core/log.h"
#include "engine/core/thread_pool.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/environment_importer.h"
#include "engine/graphics/font_asset.h"
#include "engine/graphics/font_importer.h"
#include "engine/graphics/mesh_importer.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/shader_importer.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/texture_importer.h"
#include "engine/scene/project.h"


#include <algorithm>
#include <cmath>
#include <filesystem>
#include <future>

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
    if (it != m_SearchPaths.end())
    {
        return;
    }
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
    if (path.empty())
    {
        return "";
    }

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_PathCache.find(path); it != m_PathCache.end())
        {
            return it->second;
        }
    }

    std::filesystem::path p(path);
    if (p.is_absolute())
    {
        return Project::NormalizePath(p).generic_string();
    }

    std::string foundPath = "";

    // 1. Handle engine/ prefix for engine-specific assets
    using namespace CHEngine::Constants;

    if (path.starts_with(Paths::EnginePrefix))
    {
        std::filesystem::path engineRoot = Project::GetEngineRoot();
        if (!engineRoot.empty())
        {
            std::string sub = path.substr(Paths::EnginePrefixSize);
            // Try relative to engine root
            std::filesystem::path p1 = engineRoot / sub;
            if (std::filesystem::exists(p1))
            {
                foundPath = p1.string();
            }
            else
            {
                // Try in engine/resources (standard layout)
                std::filesystem::path p2 = engineRoot / "engine" / "resources" / sub;
                if (std::filesystem::exists(p2))
                {
                    foundPath = p2.string();
                }
            }
        }
    }

    // 2. Try Search Paths (provided by the user or editor)
    if (foundPath.empty())
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        for (const auto& searchPath : m_SearchPaths)
        {
            std::filesystem::path assetPath = searchPath / path;
            if (std::filesystem::exists(assetPath))
            {
                foundPath = assetPath.string();
                break;
            }
        }
    }

    // 3. Try relative to Project Asset Directory or Project Root
    if (foundPath.empty())
    {
        std::filesystem::path assetDir = Project::GetAssetDirectory();
        std::filesystem::path p1 = assetDir / path;
        if (std::filesystem::exists(p1))
        {
            foundPath = p1.string();
        }
        else
        {
            std::filesystem::path projectRoot = Project::GetProjectDirectory();
            std::filesystem::path p2 = projectRoot / path;
            if (std::filesystem::exists(p2))
            {
                foundPath = p2.string();
            }
        }
    }

    // Final Fallback: use current directory or rootRel if set
    if (foundPath.empty() && !m_RootPath.empty())
    {
        std::filesystem::path rootRel = m_RootPath / path;
        if (std::filesystem::exists(rootRel))
        {
            foundPath = rootRel.string();
        }
    }

    if (foundPath.empty())
    {
        // Don't warn for engine/ assets during early init, they might load lazily
        if (!path.starts_with("engine/"))
        {
            CH_CORE_WARN("AssetManager: Could not resolve asset path '{}'.", path);
        }
        foundPath = path;
    }

    // Use Project::NormalizePath to handle absolute/relative and unify slashes WITHOUT forcing lowercase
    std::string normalized = Project::NormalizePath(foundPath).generic_string();

    CH_CORE_TRACE("AssetManager: Resolved '{}' -> '{}'", path, normalized);

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        m_PathCache[path] = normalized;
    }

    return normalized;
}

std::shared_ptr<Asset> AssetManager::GetAsset(const std::string& path, AssetType type)
{
    if (path.empty() || type == AssetType::None)
    {
        return nullptr;
    }

    std::string resolved = ResolvePath(path);

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        auto& cache = m_AssetCaches[type];
        if (auto it = cache.find(resolved); it != cache.end())
        {
            // CH_CORE_INFO("AssetManager: Cache HIT for '{}' (state: {})", resolved, (int)it->second->GetState());
            return it->second;
        }
    }

    // CH_CORE_INFO("AssetManager: Cache MISS for '{}', creating new asset", resolved);
    std::shared_ptr<Asset> asset = nullptr;

    // Async path for specific types
    bool isProceduralModel = (type == AssetType::Model && resolved.starts_with(":"));
    if (!isProceduralModel && (type == AssetType::Texture || type == AssetType::Model || type == AssetType::Audio))
    {
        if (type == AssetType::Texture)
        {
            asset = std::make_shared<TextureAsset>();
        }
        else if (type == AssetType::Model)
        {
            asset = std::make_shared<ModelAsset>();
        }
        else
        {
            asset = std::make_shared<SoundAsset>();
        }

        asset->SetPath(resolved);
        asset->SetState(AssetState::Loading);

        // Cache immediately to prevent duplicate requests
        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            auto& cache = m_AssetCaches[type];
            if (auto it = cache.find(resolved); it != cache.end())
            {
                CH_CORE_WARN("AssetManager: [Race Condition Avoided] Asset for '{}' was created by another thread "
                             "while we were preparing it.",
                             resolved);
                return it->second;
            }
            m_AssetCaches[type][resolved] = asset;
        }

        // Start background load
        std::string pathCopy = resolved;
        CH_CORE_INFO("AssetManager: [Main] Scheduling async load for '{}' (Type: {})", pathCopy, (int)type);
        std::weak_ptr<Asset> weakAsset = asset;

        auto future = ThreadPool::Get().Enqueue([this, pathCopy, type, weakAsset]() {
            auto sharedAsset = weakAsset.lock();
            if (!sharedAsset)
            {
                return;
            }

            bool success = false;
            if (type == AssetType::Texture)
            {
                auto texAsset = std::static_pointer_cast<TextureAsset>(sharedAsset);
                Image img = {0};
                std::string ext = std::filesystem::path(pathCopy).extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                bool isHDR = (ext == ".hdr");

                CH_CORE_INFO("AssetManager: [Background] Processing texture '{}', extension detected: '{}', isHDR: {}",
                             pathCopy, ext, isHDR ? "YES" : "NO");

                if (isHDR)
                {
                    CH_CORE_INFO("AssetManager: Recognized {} as HDR file, will load directly on main thread",
                                 pathCopy);
                    success = true;
                }
                else
                {
                    img = TextureImporter::LoadImageFromDisk(pathCopy);
                    if (img.data != nullptr)
                    {
                        CH_CORE_INFO("AssetManager: Loaded image {} ({}x{}, format={})", pathCopy, img.width,
                                     img.height, img.format);

                        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                        texAsset->SetPendingImage(img);
                        success = true;
                    }
                    else
                    {
                        CH_CORE_ERROR("AssetManager: Failed to load image from disk: {}", pathCopy);
                    }
                }
            }
            else if (type == AssetType::Model)
            {
                auto modelAsset = std::static_pointer_cast<ModelAsset>(sharedAsset);
                auto pendingData = MeshImporter::LoadMeshDataFromDisk(pathCopy);
                if (pendingData.isValid)
                {
                    modelAsset->SetPendingData(pendingData);
                    success = true;
                }
            }
            else if (type == AssetType::Audio)
            {
                auto soundAsset = std::static_pointer_cast<SoundAsset>(sharedAsset);
                AudioImporter::ImportSoundAsync(soundAsset, pathCopy);
                if (soundAsset->GetState() != AssetState::Failed)
                {
                    success = true;
                }
            }

            if (success)
            {
                std::lock_guard<std::mutex> lock(m_PendingUploadsMutex);
                m_PendingUploads.push_back(sharedAsset);
            }
            else
            {
                sharedAsset->SetState(AssetState::Failed);
                CH_CORE_ERROR("AssetManager: Background load FAILED for '{}'", pathCopy);
            }
        });

        {
            std::lock_guard<std::mutex> lock(m_FuturesMutex);
            m_Futures.push_back(std::move(future));
        }

        return asset;
    }

    // Sync path for others
    switch (type)
    {
    case AssetType::Model:
        if (resolved.starts_with(":"))
        {
            asset = std::static_pointer_cast<Asset>(MeshImporter::ImportMesh(resolved));
        }
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

        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        auto& cache = m_AssetCaches[type];
        if (auto it = cache.find(resolved); it != cache.end())
        {
            CH_CORE_WARN("AssetManager: [Race Condition Avoided Sync] Asset for '{}' was created by another thread.",
                         resolved);
            return it->second;
        }
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
    if (path.empty() || type == AssetType::None)
    {
        return;
    }
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
    // 1. Clean up finished futures
    {
        std::lock_guard<std::mutex> lock(m_FuturesMutex);
        m_Futures.erase(std::remove_if(m_Futures.begin(), m_Futures.end(),
                                       [](std::future<void>& f) {
                                           return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                                       }),
                        m_Futures.end());
    }

    // 2. Process finished background loads on the main thread (GPU upload)
    // Throttled: upload    int m_MaxUploadsPerFrame = 10; // Throttle GPU uploads to avoid per-frame spikes
    std::vector<std::shared_ptr<Asset>> toUpload;
    {
        std::lock_guard<std::mutex> lock(m_PendingUploadsMutex);
        if (m_PendingUploads.empty())
        {
            return;
        }
        int toProcess = std::min((int)m_PendingUploads.size(), m_MaxUploadsPerFrame);
        toUpload.insert(toUpload.end(), m_PendingUploads.begin(), m_PendingUploads.begin() + toProcess);
        m_PendingUploads.erase(m_PendingUploads.begin(), m_PendingUploads.begin() + toProcess);
    }

    for (auto& asset : toUpload)
    {
        if (asset->GetType() == AssetType::Texture)
        {
            std::static_pointer_cast<TextureAsset>(asset)->UploadToGPU();
        }
        else if (asset->GetType() == AssetType::Model)
        {
            std::static_pointer_cast<ModelAsset>(asset)->UploadToGPU();
        }
        else if (asset->GetType() == AssetType::Audio)
        {
            asset->SetState(AssetState::Ready);
        }
        CH_CORE_INFO("AssetManager: Background load completed and uploaded to GPU for '{}'", asset->GetPath());
    }
}

int AssetManager::GetPendingCount() const
{
    std::lock_guard<std::mutex> lock(m_PendingUploadsMutex);
    return (int)m_PendingUploads.size();
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
