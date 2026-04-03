#include "engine/core/assets/asset_manager.h"
#include "engine/core/thread_pool.h"
#include "engine/scene/project.h"

namespace CHEngine
{
static std::unique_ptr<AssetManager> s_Instance = nullptr;

AssetManager::AssetManager()
{
}

AssetManager::~AssetManager()
{
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_AssetCache.clear();
    m_PathToHandle.clear();
    m_PathCache.clear();
    m_Loaders.clear();
}

AssetManager& AssetManager::Get()
{
    if (!s_Instance)
    {
        s_Instance = std::make_unique<AssetManager>();
    }
    return *s_Instance;
}

void AssetManager::RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader)
{
    m_Loaders[type] = std::move(loader);
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

    std::string resolved = Project::GetAbsolutePath(path).string();

    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_PathCache[path] = resolved;
    return resolved;
}

AssetHandle AssetManager::ResolveToHandle(const std::string& path) const
{
    std::string resolved = ResolvePath(path);
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
    {
        return it->second;
    }
    return AssetHandle(0); // Invalid handle if not loaded yet
}

std::shared_ptr<Asset> AssetManager::LoadAsset(const std::string& path, AssetType type)
{
    if (path.empty())
    {
        return nullptr;
    }

    std::string resolved = ResolvePath(path);

    // 1. Ensure we don't try to load it twice
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
        {
            auto handle = it->second;
            if (auto currentIt = m_AssetCache.find(handle); currentIt != m_AssetCache.end())
            {
                return currentIt->second;
            }
        }
    }

    // 2. Find loader
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    auto loaderIt = m_Loaders.find(type);
    if (loaderIt == m_Loaders.end())
    {
        CH_CORE_ERROR("AssetManager: No loader registered for type {}", (int)type);
        return nullptr;
    }

    // 3. Create and Load
    auto asset = loaderIt->second->Create();
    if (!asset)
    {
        return nullptr;
    }

    // Use a new valid UUID for the loaded asset
    UUID newHandle;
    // We cannot change m_ID of asset easily since it isn't exposed except via constructor or friend?
    // Let's assume Asset sets its ID in constructor to a random UUID, we retrieve it:
    newHandle = asset->GetID();
    asset->SetPath(resolved);
    asset->SetState(AssetState::Loading);

    // 4. Start Loading
    m_AssetCache[newHandle] = asset;
    m_PathToHandle[resolved] = newHandle;

    if (!loaderIt->second->IsAsync())
    {
        bool success = loaderIt->second->Load(asset, resolved);
        if (!success)
        {
            asset->SetState(AssetState::Failed);
            return asset;
        }
        asset->OnLoaded();
        asset->SetState(AssetState::Ready);
    }
    else
    {
        // True Async Loading via ThreadPool
        IAssetLoader* loader = loaderIt->second.get();
        ThreadPool::Get().QueueTask([this, asset, loader, resolved]() {
            bool success = loader->Load(asset, resolved);
            if (!success)
            {
                asset->SetState(AssetState::Failed);
            }
            
            std::lock_guard<std::mutex> lock(m_PendingMutex);
            m_PendingAssets.push_back(asset);
        });
    }

    return asset;
}

std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle, AssetType type)
{
    // If handle is valid, check cache
    if (handle != 0)
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_AssetCache.find(handle); it != m_AssetCache.end())
        {
            return it->second;
        }
    }

    // Try to find the path corresponding to this handle for loading
    // Since we don't know the path here, we can't auto-load just by handle unless we have a registry
    // In this ultra-simplified version, Get(path) handles loading. Get(handle) just fetches.
    return nullptr;
}

void AssetManager::Update()
{
    std::vector<std::shared_ptr<Asset>> completed;
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        if (m_PendingAssets.empty())
        {
            return;
        }
        completed = std::move(m_PendingAssets);
        m_PendingAssets.clear();
    }

    for (auto& asset : completed)
    {
        asset->OnLoaded();
        if (asset->GetState() != AssetState::Failed)
        {
            asset->SetState(AssetState::Ready);
        }
    }
}

void AssetManager::ReloadAsset(AssetHandle handle, AssetType type)
{
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

    auto it = m_AssetCache.find(handle);
    if (it == m_AssetCache.end())
    {
        return;
    }

    auto loaderIt = m_Loaders.find(type);
    if (loaderIt == m_Loaders.end())
    {
        return;
    }

    auto& asset = it->second;
    std::string path = asset->GetPath();
    if (path.empty())
    {
        return;
    }

    std::string resolved = ResolvePath(path);
    loaderIt->second->Load(asset, resolved);
    asset->OnLoaded();
}

} // namespace CHEngine
