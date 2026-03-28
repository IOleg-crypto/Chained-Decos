#include "sound_asset.h"
#include "engine/core/log.h"
#include "miniaudio.h"

namespace CHEngine
{
void SoundAsset::UploadToGPU()
{
    if (m_HasPendingWave && m_PendingWave.maDecoder != nullptr)
    {
        // Placeholder for native audio loading
        m_Sound.maSound = nullptr; // Dummy
        m_PendingWave.maDecoder = nullptr;
        m_HasPendingWave = false;
        SetState(AssetState::Ready);
    }
}

SoundAsset::~SoundAsset()
{
    if (m_Sound.maSound)
    {
        ma_sound* sound = (ma_sound*)m_Sound.maSound;
        ma_sound_uninit(sound);
        delete sound;
        m_Sound.maSound = nullptr;
    }
}

} // namespace CHEngine
