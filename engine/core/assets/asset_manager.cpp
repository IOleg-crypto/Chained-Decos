#include "engine/core/assets/asset_manager.h"
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

    std::string resolved = Project::NormalizePath(path).string();

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
    if (path.empty()) return;

    std::string resolved = ResolvePath(path);
    loaderIt->second->Load(asset, resolved);
    asset->OnLoaded();
}

} // namespace CHEngine
