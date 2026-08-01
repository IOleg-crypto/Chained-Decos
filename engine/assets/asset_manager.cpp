#include "engine/assets/asset_manager.h"
#include "engine/common/thread_pool.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"

#include "engine/assets/asset_metadata.h"
#include "engine/assets/loaders/font_loader.h"
#include "engine/assets/loaders/model_loader.h"
#include "engine/assets/loaders/audio_loader.h"
#include "engine/assets/loaders/material_loader.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "engine/assets/loaders/texture_loader.h"
#include "engine/assets/loaders/environment_loader.h"
#include "engine/assets/loaders/shader_loader.h"
#include "engine/assets/loaders/anim_graph_loader.h"

#include "pack/reader.hpp"

namespace Chained
{
constexpr size_t kMaxAssetFinalizationsPerFrame = 32;
constexpr auto kMaxAssetFinalizeBudget = std::chrono::milliseconds(5);

AssetManager::AssetManager() = default;

void AssetManager::Initialize()
{
    RegisterLoader(AssetType::Model, std::make_unique<ModelLoader>());
    RegisterLoader(AssetType::Texture, std::make_unique<TextureLoader>());
    RegisterLoader(AssetType::Shader, std::make_unique<ShaderLoader>());
    RegisterLoader(AssetType::Environment, std::make_unique<EnvironmentLoader>());
    RegisterLoader(AssetType::Font, std::make_unique<FontLoader>());
    RegisterLoader(AssetType::Audio, std::make_unique<AudioLoader>());
    RegisterLoader(AssetType::Material, std::make_unique<MaterialLoader>());
    RegisterLoader(AssetType::AnimationGraph, std::make_unique<AnimGraphLoader>());
}

void AssetManager::Shutdown()
{
    constexpr int kMaxIter = 5000;
    int iter = 0;
    while (HasBackgroundWork() && iter++ < kMaxIter)
    {
        FinalizePendingLoads();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (iter >= kMaxIter)
    {
        CH_CORE_ERROR("AssetManager: Shutdown timed out — some assets may still be loading.");
    }
    if (auto* tp = ServiceLocator::TryGet<ThreadPool>())
    {
        tp->WaitIdle();
    }
}

void AssetManager::Update(Timestep ts)
{
    if (m_HotReloadInterval > 0.0f)
    {
        m_HotReloadAccumulator += ts.GetSeconds();
        if (m_HotReloadAccumulator >= m_HotReloadInterval)
        {
            m_HotReloadAccumulator = 0.0f;
            CheckAssetHotReload();
        }
    }

    FinalizePendingLoads();
}

std::string AssetManager::ResolvePackKey(const std::string& assetPath) const
{
    auto NormalizeSlashes = [](std::string& s) { std::replace(s.begin(), s.end(), '\\', '/'); };

    auto StripPrefix = [](std::string& input, const std::filesystem::path& base) {
        if (base.empty())
        {
            return;
        }
        std::string basePath = base.generic_string();
        if (!basePath.empty() && basePath.back() != '/')
        {
            basePath += '/';
        }
        if (input.size() >= basePath.size() && input.compare(0, basePath.size(), basePath) == 0)
        {
            input = input.substr(basePath.size());
        }
    };

    std::string packKey = assetPath;
    NormalizeSlashes(packKey);
    StripPrefix(packKey, m_EngineRoot);
    StripPrefix(packKey, m_ProjectDirectory);

    packKey = std::filesystem::path(packKey).lexically_normal().generic_string();
    return packKey;
}

std::vector<AssetManager::StaleAsset> AssetManager::CollectStaleAssets(int thresholdSeconds) const
{
    CH_PROFILE_FUNCTION();
    std::vector<StaleAsset> stale;

    // Snapshot the cache under a short lock
    std::vector<std::pair<AssetHandle, std::shared_ptr<Asset>>> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_AssetLock);
        snapshot.reserve(m_AssetCache.size());
        for (const auto& [handle, asset] : m_AssetCache)
        {
            if (asset && asset->GetState() != AssetState::Loading)
            {
                snapshot.emplace_back(handle, asset);
            }
        }
    }

    auto now = std::filesystem::file_time_type::clock::now();
    for (const auto& [handle, asset] : snapshot)
    {
        AssetType type = asset->GetType();
        if (type != AssetType::Model && type != AssetType::Texture && type != AssetType::Material)
        {
            continue;
        }

        const std::string& path = asset->GetPath();
        if (path.empty())
        {
            continue;
        }

        std::error_code ec;
        auto fileTime = std::filesystem::last_write_time(path, ec);
        if (ec)
        {
            continue;
        }

        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - fileTime).count();
        if (age < thresholdSeconds)
        {
            stale.push_back({handle, type, path});
        }
    }

    return stale;
}

void AssetManager::CheckAssetHotReload()
{
    auto stale = CollectStaleAssets(10);
    for (const auto& [handle, type, path] : stale)
    {
        CH_CORE_INFO("AssetManager: Hot-reloading recently modified {} '{}'",
                     type == AssetType::Model     ? "model"
                     : type == AssetType::Texture ? "texture"
                                                  : "material",
                     std::filesystem::path(path).filename().string());
        ReloadAsset(handle, type);
    }
}

AssetManager::~AssetManager()
{
    std::lock_guard<std::mutex> lock(m_AssetLock);
    m_PackOpen = false;
    m_AssetCache.clear();
    m_PathToHandle.clear();
    m_PathCache.clear();
    m_Loaders.clear();
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
        std::lock_guard<std::mutex> lock(m_AssetLock);
        if (auto it = m_PathCache.find(path); it != m_PathCache.end())
        {
            return it->second;
        }
    }

    std::filesystem::path inputPath(path);
    std::filesystem::path resolvedPath;

    if (inputPath.is_absolute())
    {
        resolvedPath = inputPath;
    }
    else
    {
        std::string pathStr = inputPath.generic_string();
        bool isEngineResource = false;
        if (pathStr.find("engine/") == 0)
        {
            pathStr = pathStr.substr(7);
            isEngineResource = true;
        }

        if (isEngineResource)
        {
            if (!m_EngineRoot.empty())
            {
                std::filesystem::path candidate = m_EngineRoot / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }
        }
        else
        {
            if (!m_AssetDirectory.empty())
            {
                std::filesystem::path candidate = m_AssetDirectory / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }

            if (resolvedPath.empty() && !m_ProjectDirectory.empty())
            {
                std::filesystem::path candidate = m_ProjectDirectory / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }
        }

        if (resolvedPath.empty())
        {
            if (!isEngineResource && !m_AssetDirectory.empty())
            {
                resolvedPath = m_AssetDirectory / pathStr;
            }
            else if (!m_EngineRoot.empty())
            {
                resolvedPath = m_EngineRoot / pathStr;
            }
            else
            {
                resolvedPath = m_ProjectDirectory / pathStr;
            }
        }
    }

    std::string resolved = std::filesystem::absolute(resolvedPath).lexically_normal().generic_string();

    std::lock_guard<std::mutex> lock(m_AssetLock);
    m_PathCache[path] = resolved;
    return resolved;
}

AssetHandle AssetManager::ResolveToHandle(const std::string& path) const
{
    std::string resolved = ResolvePath(path);
    std::lock_guard<std::mutex> lock(m_AssetLock);
    if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
    {
        return it->second;
    }
    return AssetHandle(0);
}

bool AssetManager::ExecuteLoad(const std::shared_ptr<Asset>& asset, IAssetLoader* loader, const std::string& resolved)
{
    try
    {
        std::string loaderError;
        if (!loader->Load(asset, resolved, &loaderError))
        {
            asset->Fail(loaderError.empty() ? ("AssetManager: Load failed for '" + resolved + "'") : loaderError);
            return false;
        }

        asset->ClearError();
        return true;
    } catch (const std::exception& e)
    {
        asset->Fail(std::string("AssetManager: Load exception for '") + resolved + "': " + e.what());
    } catch (...)
    {
        asset->Fail(std::string("AssetManager: Load exception for '") + resolved + "' with unknown exception");
    }
    return false;
}

std::shared_ptr<Asset> AssetManager::LoadAsset(const std::string& path, AssetType type)
{
    if (path.empty() || path.front() == '*')
    {
        return nullptr;
    }

    std::string resolved = ResolvePath(path);

    IAssetLoader* loader = nullptr;
    std::shared_ptr<Asset> asset;

    {
        std::lock_guard<std::mutex> lock(m_AssetLock);
        if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
        {
            auto handle = it->second;
            if (auto currentIt = m_AssetCache.find(handle); currentIt != m_AssetCache.end())
            {
                return currentIt->second;
            }
        }

        auto loaderIt = m_Loaders.find(type);
        if (loaderIt == m_Loaders.end())
        {
            CH_CORE_ERROR("AssetManager: No loader registered for type {}", (int)type);
            return nullptr;
        }

        asset = loaderIt->second->Create();
        if (!asset)
        {
            return nullptr;
        }

        loader = loaderIt->second.get();

        // Load or create .meta sidecar to get a stable UUID
        auto meta = MetaUtils::LoadOrCreateMeta(resolved, type);
        AssetHandle stableHandle = meta.uuid;
        asset->SetID(stableHandle);
        asset->SetPath(resolved);
        asset->SetState(AssetState::Loading);
        asset->ClearError();
        m_AssetCache[stableHandle] = asset;
        m_PathToHandle[resolved] = stableHandle;
    }

    if (!loader->IsAsync())
    {
        if (ExecuteLoad(asset, loader, resolved))
        {
            asset->OnLoaded();
            asset->SetState(AssetState::Ready);
        }
    }
    else
    {
        if (auto* tp = ServiceLocator::TryGet<ThreadPool>())
        {
            tp->QueueTask([this, asset, loader, resolved]() {
                if (ExecuteLoad(asset, loader, resolved))
                {
                    std::lock_guard<std::mutex> lock(m_PendingMutex);
                    m_PendingAssets.push_back(asset);
                }
            });
        }
    }

    return asset;
}

std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle)
{
    if (handle != 0)
    {
        std::lock_guard<std::mutex> lock(m_AssetLock);
        if (auto it = m_AssetCache.find(handle); it != m_AssetCache.end())
        {
            return it->second;
        }
    }

    return nullptr;
}

void AssetManager::FinalizePendingLoads()
{
    CH_PROFILE_FUNCTION();

    const auto updateStart = std::chrono::steady_clock::now();
    size_t finalizedCount = 0;

    while (finalizedCount < kMaxAssetFinalizationsPerFrame)
    {
        std::shared_ptr<Asset> asset;
        {
            std::lock_guard<std::mutex> lock(m_PendingMutex);
            if (m_PendingAssets.empty())
            {
                return;
            }

            asset = std::move(m_PendingAssets.front());
            m_PendingAssets.pop_front();
        }

        if (!asset)
        {
            continue;
        }

        try
        {
            asset->ClearError();
            asset->OnLoaded();
            if (asset->GetState() != AssetState::Failed)
            {
                asset->SetState(AssetState::Ready);
            }
        } catch (const std::exception& e)
        {
            asset->Fail(std::string("AssetManager: Finalization failed for '") + asset->GetPath() + "': " + e.what());
        } catch (...)
        {
            asset->Fail(std::string("AssetManager: Finalization failed for '") + asset->GetPath() +
                        "' with an unknown exception");
        }

        ++finalizedCount;

        if ((std::chrono::steady_clock::now() - updateStart) >= kMaxAssetFinalizeBudget)
        {
            break;
        }
    }
}

size_t AssetManager::GetPendingFinalizeCount() const
{
    std::lock_guard<std::mutex> lock(m_PendingMutex);
    return m_PendingAssets.size();
}

size_t AssetManager::GetLoadingAssetCount() const
{
    std::lock_guard<std::mutex> lock(m_AssetLock);

    size_t loadingCount = 0;
    for (const auto& [handle, asset] : m_AssetCache)
    {
        (void)handle;
        if (asset && asset->GetState() == AssetState::Loading)
        {
            ++loadingCount;
        }
    }

    return loadingCount;
}

bool AssetManager::HasBackgroundWork() const
{
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        if (!m_PendingAssets.empty())
        {
            return true;
        }
    }
    std::lock_guard<std::mutex> lock(m_AssetLock);
    for (const auto& [handle, asset] : m_AssetCache)
    {
        if (asset && asset->GetState() == AssetState::Loading)
        {
            return true;
        }
    }
    return false;
}
void AssetManager::ReloadAsset(AssetHandle handle, AssetType type)
{
    std::shared_ptr<Asset> asset;
    IAssetLoader* loader = nullptr;
    std::string path;

    {
        std::lock_guard<std::mutex> lock(m_AssetLock);

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

        asset = it->second;
        loader = loaderIt->second.get();
        path = asset->GetPath();
        if (path.empty())
        {
            return;
        }
    }

    std::string resolved = ResolvePath(path);
    asset->SetState(AssetState::Loading);
    if (ExecuteLoad(asset, loader, resolved))
    {
        asset->OnLoaded();
        asset->SetState(AssetState::Ready);

        // Update content hash in .meta after successful reload
        if (std::filesystem::exists(resolved))
        {
            auto metaPath = MetaUtils::GetMetaPath(resolved);
            if (std::filesystem::exists(metaPath))
            {
                auto meta = MetaUtils::ReadMeta(metaPath);
                meta.contentHash = MetaUtils::ComputeFileHash(resolved);
                MetaUtils::WriteMeta(metaPath, meta);
            }
        }
    }
}

void AssetManager::Invalidate(const std::string& path, bool deleteFromDisk)
{
    if (path.empty())
    {
        return;
    }

    std::string resolved = ResolvePath(path);

    if (deleteFromDisk)
    {
        DeleteChasset(resolved);
    }

    std::lock_guard<std::mutex> lock(m_AssetLock);

    auto pathIt = m_PathToHandle.find(resolved);
    if (pathIt == m_PathToHandle.end())
    {
        return;
    }

    AssetHandle handle = pathIt->second;
    m_AssetCache.erase(handle);
    m_PathToHandle.erase(pathIt);

    for (auto it = m_PathCache.begin(); it != m_PathCache.end();)
    {
        if (it->second == resolved)
        {
            it = m_PathCache.erase(it);
        }
        else
        {
            ++it;
        }
    }

    CH_CORE_INFO("AssetManager: Invalidated cache for '{}'", resolved);
}

bool AssetManager::DeleteChasset(const std::string& path)
{
    std::filesystem::path modelPath = path;
    std::filesystem::path chassetPath = modelPath;
    chassetPath.replace_extension(".chasset");

    std::error_code ec;
    if (std::filesystem::exists(chassetPath, ec))
    {
        std::filesystem::remove(chassetPath, ec);
        if (!ec)
        {
            CH_CORE_INFO("AssetManager: Deleted .chasset '{}'", chassetPath.filename().string());
            return true;
        }
        CH_CORE_WARN("AssetManager: Failed to delete .chasset '{}': {}", chassetPath.string(), ec.message());
    }
    return false;
}

size_t AssetManager::DeleteAllChassets()
{
    if (m_AssetDirectory.empty())
    {
        CH_CORE_WARN("AssetManager: Asset directory not set, cannot clear .chasset cache");
        return 0;
    }

    size_t deleted = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_AssetDirectory, ec))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".chasset")
        {
            std::filesystem::remove(entry.path(), ec);
            if (!ec)
            {
                CH_CORE_INFO("AssetManager: Deleted .chasset '{}'", entry.path().filename().string());
                ++deleted;
            }
            else
            {
                CH_CORE_WARN("AssetManager: Failed to delete .chasset '{}': {}", entry.path().string(), ec.message());
            }
        }
    }

    CH_CORE_INFO("AssetManager: Cleared .chasset cache ({} file(s) deleted)", deleted);

    if (deleted > 0)
    {
        std::lock_guard<std::mutex> lock(m_AssetLock);
        m_AssetCache.clear();
        m_PathToHandle.clear();
        m_PathCache.clear();
    }

    return deleted;
}

std::vector<std::string> AssetManager::GetStaleAssets() const
{
    auto stale = CollectStaleAssets(30);
    std::vector<std::string> paths;
    paths.reserve(stale.size());
    for (auto& s : stale)
    {
        paths.push_back(std::move(s.path));
    }
    return paths;
}

size_t AssetManager::ReloadAllStale()
{
    auto stale = CollectStaleAssets(30);
    for (const auto& [handle, type, path] : stale)
    {
        ReloadAsset(handle, type);
    }

    if (!stale.empty())
    {
        CH_CORE_INFO("AssetManager: Reloaded {} stale assets", stale.size());
    }

    return stale.size();
}

bool AssetManager::OpenPack(const std::filesystem::path& packPath)
{
    if (m_PackOpen)
    {
        return true;
    }

    if (!std::filesystem::exists(packPath))
    {
        CH_CORE_WARN("AssetManager: Pack file not found: {}", packPath.string());
        return false;
    }

    try
    {
        m_PackReader = std::make_unique<pack::Reader>(packPath);
        m_PackOpen = true;
        CH_CORE_INFO("AssetManager: Opened pack '{}' ({} items)", packPath.string(), m_PackReader->getItemCount());
        return true;
    } catch (const pack::Error& err)
    {
        CH_CORE_ERROR("AssetManager: Failed to open pack '{}': {}", packPath.string(), err.what());
        m_PackReader.reset();
        m_PackOpen = false;
        return false;
    }
}

std::vector<uint8_t> AssetManager::ReadAssetData(const std::string& assetPath)
{
    {
        std::lock_guard<std::mutex> lock(m_AssetLock);
        if (m_PackOpen && m_PackReader)
        {
            std::string packKey = ResolvePackKey(assetPath);

            uint64_t idx = 0;
            if (m_PackReader->getItemIndex(packKey.c_str(), idx))
            {
                std::vector<uint8_t> data;
                m_PackReader->readItemData(idx, data);
                return data;
            }

            if (packKey.rfind("assets/", 0) != 0 && packKey.rfind("resources/", 0) != 0)
            {
                std::string altKey = "assets/" + packKey;
                if (m_PackReader->getItemIndex(altKey.c_str(), idx))
                {
                    std::vector<uint8_t> data;
                    m_PackReader->readItemData(idx, data);
                    return data;
                }
                altKey = "resources/" + packKey;
                if (m_PackReader->getItemIndex(altKey.c_str(), idx))
                {
                    std::vector<uint8_t> data;
                    m_PackReader->readItemData(idx, data);
                    return data;
                }
            }
        }
    }

    std::ifstream file(assetPath, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        auto size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> data(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    return {};
}

bool AssetManager::HasAsset(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_AssetLock);
    if (m_PackOpen && m_PackReader)
    {
        std::string packKey = ResolvePackKey(path);

        uint64_t idx = 0;
        if (m_PackReader->getItemIndex(packKey.c_str(), idx))
        {
            return true;
        }

        if (packKey.rfind("assets/", 0) != 0 && packKey.rfind("resources/", 0) != 0)
        {
            std::string altKey = "assets/" + packKey;
            if (m_PackReader->getItemIndex(altKey.c_str(), idx))
            {
                return true;
            }
            altKey = "resources/" + packKey;
            if (m_PackReader->getItemIndex(altKey.c_str(), idx))
            {
                return true;
            }
        }
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

void AssetManager::EnumeratePackedPaths(const std::function<void(std::string_view)>& callback) const
{
    if (!m_PackOpen || !m_PackReader)
    {
        return;
    }
    const uint64_t count = m_PackReader->getItemCount();
    for (uint64_t i = 0; i < count; ++i)
    {
        callback(m_PackReader->getItemPath(i));
    }
}

std::vector<uint8_t> AssetManager::ReadProjectAsset(const std::filesystem::path& absolutePath)
{
    if (!m_PackOpen || !m_PackReader || m_ProjectDirectory.empty())
    {
        return {};
    }
    std::error_code ec;
    auto rel = std::filesystem::relative(absolutePath, m_ProjectDirectory, ec);
    if (ec || rel.empty())
    {
        return {};
    }
    return ReadAssetData(rel.generic_string());
}

} // namespace Chained
