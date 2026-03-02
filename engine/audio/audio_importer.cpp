#include "audio_importer.h"
#include "engine/core/log.h"
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<SoundAsset> AudioImporter::ImportSound(const std::string& path)
{
    if (path.empty())
    {
        return nullptr;
    }

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("AudioImporter: File not found: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<SoundAsset>();
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);

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

    // Audio assets don't need background loading into the asset object anymore.
    // They are loaded on-demand by the Audio system.
    asset->SetState(AssetState::Ready);
}
} // namespace CHEngine
