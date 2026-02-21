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

    Sound sound = ::LoadSound(fullPath.string().c_str());
    if (sound.stream.buffer == nullptr)
    {
        CH_CORE_ERROR("AudioImporter: Failed to load sound: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<SoundAsset>();
    asset->SetPath(path);
    asset->GetSound() = sound;
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

    Wave wave = ::LoadWave(fullPath.string().c_str());
    if (wave.frameCount > 0)
    {
        asset->SetPendingWave(wave);
        // State remains Loading, AssetManager will call UploadToGPU on main thread
    }
    else
    {
        asset->SetState(AssetState::Failed);
    }
}
} // namespace CHEngine
