#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include <string>

namespace CHEngine
{
struct AudioComponent
{
    AssetHandle SoundHandle = 0;
    std::string SoundPath; // Cached path for the audio system

    bool Loop = false;
    bool PlayOnStart = true;
    float Volume = 1.0f;
    float Pitch = 1.0f;

    AudioComponent() = default;
    AudioComponent(const AudioComponent&) = default;

    // Runtime
    bool IsPlaying = false;
};

} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
