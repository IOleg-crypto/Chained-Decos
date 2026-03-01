#include "sound_asset.h"
#include "engine/core/log.h"
#include "raylib.h"

namespace CHEngine
{
void SoundAsset::UploadToGPU()
{
    // This MUST run on main thread (where Audio device is initialized)
    if (m_HasPendingWave && m_PendingWave.frameCount > 0)
    {
        if (m_Sound.stream.buffer != nullptr)
        {
            ::UnloadSound(m_Sound);
        }

        m_Sound = ::LoadSoundFromWave(m_PendingWave);
        ::UnloadWave(m_PendingWave);

        m_PendingWave.frameCount = 0;
        m_HasPendingWave = false;

        SetState(AssetState::Ready);
    }
}

SoundAsset::~SoundAsset()
{
    if (m_Sound.stream.buffer != nullptr)
    {
        ::UnloadSound(m_Sound);
    }
}

} // namespace CHEngine
