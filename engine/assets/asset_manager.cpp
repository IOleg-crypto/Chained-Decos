#include "engine/assets/asset_manager.h"
#include "engine/common/thread_pool.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"

#include "engine/assets/asset_metadata.h"
#include "engine/assets/loaders/font_loader.h"
#include "engine/assets/loaders/model_loader.h"
#include "engine/assets/loaders/audio_loader.h"
#include "engine/assets/loaders/material_loader.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include "engine/assets/loaders/texture_loader.h"
#include "engine/assets/loaders/environment_loader.h"
#include "engine/assets/loaders/shader_loader.h"

#include "pack/reader.hpp"

namespace Chained
{
constexpr size_t kMaxAssetFinalizationsPerFrame = 32;
constexpr auto kMaxAssetFinalizeBudget = std::chrono::milliseconds(5);

AssetManager::AssetManager() = default;

void AssetManager::Initialize()
{
    RegisterLoader(AssetType::Model, AssetLoader{ModelLoader::Create, ModelLoader::Load, true});
    RegisterLoader(AssetType::Texture, AssetLoader{TextureLoader::Create, TextureLoader::Load, true});
    RegisterLoader(AssetType::Shader, AssetLoader{ShaderLoader::Create, ShaderLoader::Load, false});
    RegisterLoader(AssetType::Environment, AssetLoader{EnvironmentLoader::Create, EnvironmentLoader::Load, true});
    RegisterLoader(AssetType::Font, AssetLoader{FontLoader::Create, FontLoader::Load, false});
    RegisterLoader(AssetType::Audio, AssetLoader{AudioLoader::Create, AudioLoader::Load, false});
    RegisterLoader(AssetType::Material, AssetLoader{MaterialLoader::Create, MaterialLoader::Load, false});
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
    if (auto* tp = ServiceLocator::Get<ThreadPool>())
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

void AssetManager::CheckAssetHotReload()
{
    CH_PROFILE_FUNCTION();

    std::vector<std::tuple<AssetHandle, AssetType, std::string>> toReload;

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        for (const auto& [handle, asset] : m_AssetCache)
        {
            if (!asset || asset->GetState() == AssetState::Loading)
            {
                continue;
            }

            AssetType type = asset->GetType();
            if (type != AssetType::Model && type != AssetType::Texture)
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

            auto now = std::filesystem::file_time_type::clock::now();
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - fileTime).count();

            // File modified very recently (within last 10 seconds) — likely just saved from external tool
            if (age < 10)
            {
                toReload.emplace_back(handle, type, asset->GetPath());
            }
        }
    }

    for (const auto& [handle, type, path] : toReload)
    {
        CH_CORE_INFO("AssetManager: Hot-reloading recently modified {} '{}'",
                     type == AssetType::Model ? "model" : "texture",
                     std::filesystem::path(path).filename().string());
        ReloadAsset(handle, type);
    }
}

AssetManager::~AssetManager()
{
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_PackReader.reset();
    m_PackOpen = false;
    m_AssetCache.clear();
    m_PathToHandle.clear();
    m_PathCache.clear();
    m_Loaders.clear();
}

void AssetManager::RegisterLoader(AssetType type, AssetLoader loader)
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
            // Try asset directory first
            if (!m_AssetDirectory.empty())
            {
                std::filesystem::path candidate = m_AssetDirectory / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }

            // Try project root next
            if (resolvedPath.empty() && !m_ProjectDirectory.empty())
            {
                std::filesystem::path candidate = m_ProjectDirectory / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }
        }

        // Fallback
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

    // Normalize and convert to string
    std::string resolved = std::filesystem::absolute(resolvedPath).lexically_normal().generic_string();

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
    return AssetHandle(0);
}

std::shared_ptr<Asset> AssetManager::LoadAsset(const std::string& path, AssetType type)
{
    if (path.empty() || path.front() == '*')
    {
        return nullptr;
    }

    std::string resolved = ResolvePath(path);

    AssetLoader* loader = nullptr;
    std::shared_ptr<Asset> asset;

    {
        // Cache check and registration MUST share one critical section: with two
        // separate lock scopes, two threads racing on the same path both miss the
        // cache and both create an asset — the second overwrites m_PathToHandle and
        // callers end up holding different instances of the "same" asset.
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
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

        asset = loaderIt->second.Create();
        if (!asset)
        {
            return nullptr;
        }

        loader = &loaderIt->second;
        AssetHandle newHandle = asset->GetID();
        asset->SetPath(resolved);
        asset->SetState(AssetState::Loading);
        asset->ClearError();
        m_AssetCache[newHandle] = asset;
        m_PathToHandle[resolved] = newHandle;
    }

    if (!loader->IsAsync)
    {
        try
        {
            std::string loaderError;
            if (!loader->Load(asset, resolved, &loaderError))
            {
                asset->Fail(loaderError.empty()
                                ? ("AssetManager: Synchronous loader returned false for '" + resolved + "'")
                                : loaderError);
                return asset;
            }

            asset->ClearError();
            asset->OnLoaded();
            asset->SetState(AssetState::Ready);
        } catch (const std::exception& e)
        {
            asset->Fail(std::string("AssetManager: Synchronous load failed for '") + resolved + "': " + e.what());
        } catch (...)
        {
            asset->Fail(std::string("AssetManager: Synchronous load failed for '") + resolved +
                        "' with an unknown exception");
        }
    }
    else
    {
        ServiceLocator::Get<ThreadPool>()->QueueTask([this, asset, loader, resolved]() {
            try
            {
                std::string loaderError;
                if (!loader->Load(asset, resolved, &loaderError))
                {
                    asset->Fail(loaderError.empty()
                                    ? ("AssetManager: Async loader returned false for '" + resolved + "'")
                                    : loaderError);
                    return;
                }

                asset->ClearError();
                std::lock_guard<std::mutex> lock(m_PendingMutex);
                m_PendingAssets.push_back(asset);
            } catch (const std::exception& e)
            {
                asset->Fail(std::string("AssetManager: Async load failed for '") + resolved + "': " + e.what());
            } catch (...)
            {
                asset->Fail(std::string("AssetManager: Async load failed for '") + resolved +
                            "' with an unknown exception");
            }
        });
    }

    return asset;
}

std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle)
{
    if (handle != 0)
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
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
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

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
    return GetPendingFinalizeCount() > 0 || GetLoadingAssetCount() > 0;
}
void AssetManager::ReloadAsset(AssetHandle handle, AssetType type)
{
    std::shared_ptr<Asset> asset;
    AssetLoader* loader = nullptr;
    std::string path;

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

        asset = it->second;
        loader = &loaderIt->second;
        path = asset->GetPath();
        if (path.empty())
        {
            return;
        }
    }

    std::string resolved = ResolvePath(path);
    asset->SetState(AssetState::Loading);
    asset->ClearError();

    try
    {
        std::string loaderError;
        if (!loader->Load(asset, resolved, &loaderError))
        {
            asset->Fail(loaderError.empty() ? ("AssetManager: Reload failed for '" + resolved + "'") : loaderError);
            return;
        }

        asset->ClearError();
        asset->OnLoaded();
        asset->SetState(AssetState::Ready);
    } catch (const std::exception& e)
    {
        asset->Fail(std::string("AssetManager: Reload failed for '") + resolved + "': " + e.what());
    } catch (...)
    {
        asset->Fail(std::string("AssetManager: Reload failed for '") + resolved +
                    "' with an unknown exception");
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

    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

    auto pathIt = m_PathToHandle.find(resolved);
    if (pathIt == m_PathToHandle.end())
    {
        return;
    }

    AssetHandle handle = pathIt->second;
    m_AssetCache.erase(handle);
    m_PathToHandle.erase(pathIt);

    // Also remove any path cache entries that resolve to this path
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
    return deleted;
}

std::vector<std::string> AssetManager::GetStaleAssets() const
{
    std::vector<std::string> stale;
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

    for (const auto& [handle, asset] : m_AssetCache)
    {
        if (!asset || asset->GetState() == AssetState::Loading)
        {
            continue;
        }

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

        // Use file's modification time as proxy — if it's newer than a threshold after asset creation, it's stale.
        // We compare against the current time minus a small grace period to avoid false positives.
        auto now = std::filesystem::file_time_type::clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - fileTime).count();

        // File modified very recently (within last 30 seconds) — likely just saved from external tool
        if (age < 30)
        {
            stale.push_back(path);
        }
    }

    return stale;
}

size_t AssetManager::ReloadAllStale()
{
    auto stale = GetStaleAssets();
    size_t reloaded = 0;

    for (const auto& path : stale)
    {
        AssetHandle handle = AssetHandle(0);
        AssetType type = AssetType::Model;

        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            auto pathIt = m_PathToHandle.find(path);
            if (pathIt == m_PathToHandle.end())
            {
                continue;
            }

            handle = pathIt->second;
            auto assetIt = m_AssetCache.find(handle);
            if (assetIt == m_AssetCache.end())
            {
                continue;
            }

            type = assetIt->second->GetType();
        }

        if (type == AssetType::Model)
        {
            ReloadAsset(handle, AssetType::Model);
            ++reloaded;
        }
        else if (type == AssetType::Texture)
        {
            ReloadAsset(handle, AssetType::Texture);
            ++reloaded;
        }
        else if (type == AssetType::Material)
        {
            ReloadAsset(handle, AssetType::Material);
            ++reloaded;
        }
    }

    if (reloaded > 0)
    {
        CH_CORE_INFO("AssetManager: Reloaded {} stale assets", reloaded);
    }

    return reloaded;
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
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (m_PackOpen && m_PackReader)
        {
            uint64_t idx = 0;
            if (m_PackReader->getItemIndex(assetPath.c_str(), idx))
            {
                std::vector<uint8_t> data;
                m_PackReader->readItemData(idx, data);
                return data;
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

} // namespace Chained