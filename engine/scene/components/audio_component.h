#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/core/assets/asset.h"
#include <string>
#include <glm/glm.hpp>

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
    bool Spatialized = false;
    glm::vec3 Position = {0, 0, 0};
    float MinDistance = 1.0f;
    float MaxDistance = 100.0f;

    // Runtime
    std::shared_ptr<SoundAsset> Asset;
    bool IsPlaying = false;

    CH_REFLECT_BEGIN(AudioComponent)
        props.Handle("SoundHandle", SoundHandle);
        props.File("SoundPath", SoundPath, "mp3,wav,ogg");
        props.Property("Volume", Volume, PropertyMeta(0.0f, 1.0f, 0.01f));
        props.Property("Pitch", Pitch, PropertyMeta(0.5f, 2.0f, 0.05f));
        props.Property("Loop", Loop);
        props.Property("PlayOnStart", PlayOnStart);
        props.Property("Spatialized", Spatialized);
        // Always serialize spatial fields to preserve values on save/load.
        // Only skip display in UI mode.
        if (props.GetMode() != CHEngine::ReflectionMode::UI || Spatialized)
        {
            props.Property("Position", Position);
            props.Property("Min Distance", MinDistance, PropertyMeta(0.1f, 100.0f, 0.5f));
            props.Property("Max Distance", MaxDistance, PropertyMeta(0.1f, 500.0f, 1.0f));
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
