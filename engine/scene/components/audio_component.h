#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include <string>

namespace CHEngine
{
class SoundAsset;

struct AudioComponent
{
    AssetHandle SoundHandle = 0;
    std::string SoundPath;
    float Volume = 1.0f;
    float Pitch = 1.0f;
    bool Loop = false;
    bool PlayOnStart = true;

    // Runtime
    std::shared_ptr<SoundAsset> Asset;
    bool IsPlaying = false;
};

} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
