#include "audio_importer.h"
#include "engine/core/log.h"
#include "miniaudio.h"
#include "audio.h"
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<SoundAsset> AudioImporter::ImportSound(const std::string& path)
{
    if (path.empty())
        return nullptr;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("AudioImporter: File not found: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<SoundAsset>();
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);
    
    CH_CORE_INFO("AudioImporter: Loaded sound {}", path);
    return asset;
}

void AudioImporter::ImportSoundAsync(const std::shared_ptr<SoundAsset>& asset, const std::string& path)
{
    if (!asset)
    {
        return;
    }

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        asset->SetState(AssetState::Failed);
        return;
    }

    // Stub: Native audio implementation pending
    // Wave wave = ::LoadWave(fullPath.string().c_str());
    asset->SetState(AssetState::Ready);
}
} // namespace CHEngine
