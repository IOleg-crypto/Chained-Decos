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
    
    ma_engine* engine = (ma_engine*)Audio::Get().GetEngine();
    if (engine)
    {
        ma_sound* sound = new ma_sound();
        ma_result result = ma_sound_init_from_file(engine, path.c_str(), 0, NULL, NULL, sound);
        if (result == MA_SUCCESS)
        {
            asset->GetSound().maSound = sound;
            asset->SetState(AssetState::Ready);
            CH_CORE_INFO("AudioImporter: Loaded sound {}", path);
        }
        else
        {
            CH_CORE_ERROR("AudioImporter: Failed to load sound {} (error {})", path, (int)result);
            delete sound;
            asset->SetState(AssetState::Failed);
        }
    }
    
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
