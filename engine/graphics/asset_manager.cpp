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
        
        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            auto& cache = m_AssetCaches[type];
            if (auto it = cache.find(resolved); it != cache.end())
            {
                return it->second;
            }
        }

        std::shared_ptr<Asset> asset = nullptr;

        // Async path for specific types
        if (type == AssetType::Texture || type == AssetType::Model || type == AssetType::Audio)
        {
            if (type == AssetType::Texture) asset = std::make_shared<TextureAsset>();
            else if (type == AssetType::Model) asset = std::make_shared<ModelAsset>();
            else asset = std::make_shared<SoundAsset>();

            asset->SetPath(resolved);
            asset->SetState(AssetState::Loading);

            // Cache immediately to prevent duplicate requests
            {
                std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
                m_AssetCaches[type][resolved] = asset;
            }

            // Start background load
            std::string pathCopy = resolved;
            std::weak_ptr<Asset> weakAsset = asset;
            
            auto future = std::async(std::launch::async, [this, pathCopy, type, weakAsset]() {
                auto sharedAsset = weakAsset.lock();
                if (!sharedAsset) return;

                bool success = false;
                if (type == AssetType::Texture)
                {
                    auto texAsset = std::static_pointer_cast<TextureAsset>(sharedAsset);
                    Image img = TextureImporter::LoadImageFromDisk(pathCopy);
                    if (img.data != nullptr)
                    {
                        // Ensure RGBA (moved from importer to background task)
                        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                        texAsset->SetPendingImage(img);
                        success = true;
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
                        success = true;
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
            m_AssetMetadata[metadata.Handle] = metadata;
            m_AssetCaches[type][resolved] = asset;
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
        // 1. Clean up finished futures
        {
            std::lock_guard<std::mutex> lock(m_FuturesMutex);
            m_Futures.erase(
                std::remove_if(m_Futures.begin(), m_Futures.end(),
                    [](std::future<void>& f) {
                        return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                    }),
                m_Futures.end());
        }

        // 2. Process finished background loads on the main thread (GPU upload)
        std::vector<std::shared_ptr<Asset>> toUpload;
        {
            std::lock_guard<std::mutex> lock(m_PendingUploadsMutex);
            if (m_PendingUploads.empty()) return;
            toUpload = std::move(m_PendingUploads);
            m_PendingUploads.clear();
        }

        for (auto& asset : toUpload)
        {
            if (asset->GetType() == AssetType::Texture)
                std::static_pointer_cast<TextureAsset>(asset)->UploadToGPU();
            else if (asset->GetType() == AssetType::Model)
                std::static_pointer_cast<ModelAsset>(asset)->UploadToGPU();
            else if (asset->GetType() == AssetType::Audio)
                std::static_pointer_cast<SoundAsset>(asset)->UploadToGPU();
            
            CH_CORE_INFO("AssetManager: Background load completed and uploaded to GPU for '{}'", asset->GetPath());
        }
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
