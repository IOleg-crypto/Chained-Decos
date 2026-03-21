#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/loaders/texture_loader.h"
#include "engine/graphics/loaders/model_loader.h"
#include "engine/graphics/loaders/shader_loader.h"
#include "engine/graphics/loaders/font_loader.h"
#include "engine/graphics/loaders/audio_loader.h"
#include "engine/graphics/loaders/environment_loader.h"
#include "engine/core/thread_pool.h"
#include "engine/scene/project.h"
#include "engine/core/constants.h"
#include "engine/core/assets/asset_loader.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <future>

namespace CHEngine
{
static std::unique_ptr<AssetManager> s_Instance = nullptr;

AssetManager::AssetManager()
{
}

AssetManager& AssetManager::Get()
{
    if (!s_Instance)
    {
        s_Instance = std::make_unique<AssetManager>();
    }
    return *s_Instance;
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
#endif
        if (m_RootPath.empty() || !std::filesystem::exists(m_RootPath / "resources"))
        {
            // Traverse up to find resources folder
            std::filesystem::path current = std::filesystem::current_path();
            while (current.has_parent_path())
            {
                if (std::filesystem::exists(current / "resources"))
                {
                    m_RootPath = current;
                    break;
                }
                current = current.parent_path();
            }

            if (m_RootPath.empty())
            {
                m_RootPath = std::filesystem::current_path();
            }
        }
    }
    else
    {
        m_RootPath = rootPath;
    }

    CH_CORE_INFO("AssetManager: Initialized. Root: {}", m_RootPath.string());

    // Register default loaders
    RegisterLoader(AssetType::Texture, std::make_unique<TextureLoader>());
    RegisterLoader(AssetType::Model, std::make_unique<ModelLoader>());
    RegisterLoader(AssetType::Shader, std::make_unique<ShaderLoader>());
    RegisterLoader(AssetType::Font, std::make_unique<FontLoader>());
    RegisterLoader(AssetType::Audio, std::make_unique<AudioLoader>());
    RegisterLoader(AssetType::Environment, std::make_unique<EnvironmentLoader>());
}

void AssetManager::Shutdown()
{
    CH_CORE_INFO("AssetManager: Shutting down...");

    // 1. Wait for all background loading to complete
    {
        std::lock_guard<std::mutex> futuresLock(m_FuturesMutex);
        for (auto& future : m_Futures)
        {
            if (future.valid())
            {
                future.wait();
            }
        }
        m_Futures.clear();
    }

    // 2. Clear loaders and caches
    m_Loaders.clear();

    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_AssetCaches.clear();
    m_AssetMetadata.clear();
    m_PathCache.clear();
}

void AssetManager::RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader)
{
    m_Loaders[type] = std::move(loader);
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
    m_PathCache.clear(); // Important: clear path cache as resolving might change
    CH_CORE_INFO("AssetManager: Cleared search paths.");
}

std::string AssetManager::ResolvePath(const std::string& path) const
{
    if (path.empty())
        return "";

    if (path.starts_with(":"))
        return path;

    std::string internalPath = path;
    if (internalPath.starts_with("/") || internalPath.starts_with("\\"))
        internalPath = internalPath.substr(1);

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_PathCache.find(internalPath); it != m_PathCache.end())
        {
            return it->second;
        }
    }

    std::filesystem::path p(internalPath);
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
        if (engineRoot.empty() && !m_RootPath.empty())
        {
            engineRoot = m_RootPath;
        }

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
                // Try in resources (standard project layout)
                std::filesystem::path p2 = engineRoot / "resources" / sub;
                if (std::filesystem::exists(p2))
                    foundPath = p2.string();
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
                foundPath = p2.string();
        }
    }

    // Final Fallback: use current directory or rootRel if set
    if (foundPath.empty() && !m_RootPath.empty())
    {
        std::filesystem::path rootRel = m_RootPath / internalPath;
        if (std::filesystem::exists(rootRel))
            foundPath = rootRel.string();
    }

    if (foundPath.empty())
    {
        // Don't warn for engine/ assets during early init, they might load lazily
        if (!path.starts_with("engine/"))
            CH_CORE_WARN("AssetManager: Could not resolve asset path '{}'.", path);
        foundPath = path;
    }

    // Use Project::NormalizePath to handle absolute/relative and unify slashes WITHOUT forcing lowercase
    std::string normalized = Project::NormalizePath(foundPath).generic_string();

    CH_CORE_TRACE("AssetManager: Resolved '{}' -> '{}'", path, normalized);
    
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        m_PathCache[internalPath] = normalized;
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
            return it->second;
        }
    }

    // NEW LOGIC: Try registry first
    if (auto it = m_Loaders.find(type); it != m_Loaders.end())
    {
        auto& loader = *it->second;
        auto asset = loader.Create();
        asset->SetPath(resolved);

        // Cache immediately to avoid duplicates
        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            m_AssetCaches[type][resolved] = asset;
            
            AssetMetadata metadata;
            metadata.Handle = asset->GetID();
            metadata.FilePath = resolved;
            metadata.Type = type;
            m_AssetMetadata[metadata.Handle] = metadata;
        }

        if (loader.IsAsync())
        {
            asset->SetState(AssetState::Loading);
            auto future = ThreadPool::Get().Enqueue([this, asset, &loader, resolved]() {
                if (loader.Load(asset, resolved))
                {
                    std::lock_guard<std::mutex> lock(m_PendingUploadsMutex);
                    m_PendingUploads.push_back(asset);
                }
                else
                {
                    asset->SetState(AssetState::Failed);
                }
            });

            std::lock_guard<std::mutex> lock(m_FuturesMutex);
            m_Futures.push_back(std::move(future));
        }
        else
        {
            if (loader.Load(asset, resolved))
            {
                asset->SetState(AssetState::Ready);
            }
            else
            {
                asset->SetState(AssetState::Failed);
            }
        }
        return asset;
    }

    // OLD FALLBACK (to be removed once all types are ported)
    CH_CORE_ERROR("AssetManager: Loader NOT FOUND for type {}", (int)type);
    return nullptr;
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

void AssetManager::ReloadAsset(const std::string& path, AssetType type)
{
    RemoveAsset(path, type);
    GetAsset(path, type);
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
    std::vector<std::shared_ptr<Asset>> toUpload;
    {
        std::lock_guard<std::mutex> lock(m_PendingUploadsMutex);
        if (m_PendingUploads.empty())
        {
            return;
        }
        toUpload = std::move(m_PendingUploads);
        m_PendingUploads.clear();
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
            std::static_pointer_cast<SoundAsset>(asset)->UploadToGPU();
        }
        CH_CORE_TRACE("AssetManager: Background load completed and uploaded to GPU for '{}'", asset->GetPath());
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
