#ifndef CH_SOUND_ASSET_H
#define CH_SOUND_ASSET_H

#include "engine/core/base.h"
#include <raylib.h>
#include <string>

namespace CHEngine
{
class SoundAsset
{
public:
    SoundAsset(Sound sound);
    ~SoundAsset();

    static Ref<SoundAsset> Load(const std::string &path);

    Sound &GetSound()
    {
        return m_Sound;
    }
    const Sound &GetSound() const
    {
        return m_Sound;
    }

private:
    Sound m_Sound;
};
} // namespace CHEngine

#endif // CH_SOUND_ASSET_H
