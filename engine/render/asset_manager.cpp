#include "asset_manager.h"
#include "asset_archive.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "resource_provider.h"


namespace CHEngine
{
std::vector<std::shared_ptr<Asset>> AssetManager::s_GPUUploadQueue;
std::mutex AssetManager::s_GPUQueueMutex;

void AssetManager::Init()
{
    CH_CORE_INFO("AssetManager: Initialized (SOLID)");
}

void AssetManager::Shutdown()
{
    AssetArchive::Clear();
    std::lock_guard<std::mutex> gpuLock(s_GPUQueueMutex);
    s_GPUUploadQueue.clear();
    CH_CORE_INFO("AssetManager: Shut Down");
}

static void ProcessTextureUploads(const std::shared_ptr<Asset> &asset)
{
    if (asset->GetType() == AssetType::Texture)
        std::static_pointer_cast<TextureAsset>(asset)->UploadToGPU();
}

static void ProcessModelUploads(const std::shared_ptr<Asset> &asset)
{
    if (asset->GetType() == AssetType::Model)
        std::static_pointer_cast<ModelAsset>(asset)->UploadToGPU();
}

void AssetManager::Update()
{
    std::vector<std::shared_ptr<Asset>> toUpload;
    {
        std::lock_guard<std::mutex> lock(s_GPUQueueMutex);
        if (s_GPUUploadQueue.empty())
            return;
        toUpload = std::move(s_GPUUploadQueue);
        s_GPUUploadQueue.clear();
    }

    for (auto &asset : toUpload)
    {
        ProcessTextureUploads(asset);
        ProcessModelUploads(asset);
    }
}

void AssetManager::QueueForGPUUpload(std::shared_ptr<Asset> asset)
{
    std::lock_guard<std::mutex> lock(s_GPUQueueMutex);
    s_GPUUploadQueue.push_back(asset);
}

std::shared_ptr<ShaderAsset> AssetManager::LoadShader(const std::string &vsPath,
                                                      const std::string &fsPath)
{
    std::string key = vsPath + "|" + fsPath;
    auto cached = AssetArchive::Get(key);
    if (cached)
        return std::static_pointer_cast<ShaderAsset>(cached);

    auto asset = ShaderAsset::Load(vsPath, fsPath);
    if (asset)
        AssetArchive::Add(key, asset);
    return asset;
}

std::shared_ptr<ShaderAsset> AssetManager::LoadShader(const std::string &path)
{
    return Get<ShaderAsset>(path);
}

std::shared_ptr<EnvironmentAsset> AssetManager::LoadEnvironment(const std::string &path)
{
    return Get<EnvironmentAsset>(path);
}

std::filesystem::path AssetManager::ResolvePath(const std::string &path)
{
    return ResourceProvider::ResolvePath(path);
}
} // namespace CHEngine
