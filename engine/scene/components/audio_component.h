#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/assets/asset.h"
#include <string>
#include <glm/glm.hpp>

namespace CHEngine
{
struct AudioComponent
{
    AssetHandle SoundHandle = AssetHandle(0);
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
    
    bool IsPlaying = false;

    CH_REFLECT_BEGIN(AudioComponent)
        if (props.GetMode() != CHEngine::ReflectionMode::UI)
            CH_HANDLE(props, SoundHandle);
        if (CH_FILE(props, SoundPath, "mp3,wav,ogg"))
        {
            // Path changed — invalidate the cached handle so SceneAudioSystem loads the new file.
            SoundHandle = AssetHandle(0);
            IsPlaying = false;
        }
        CH_PROP_META(props, Volume, PropertyMeta(0.0f, 1.0f, 0.01f));
        CH_PROP_META(props, Pitch, PropertyMeta(0.5f, 2.0f, 0.05f));
        CH_PROP(props, Loop);
        CH_PROP(props, PlayOnStart);
        CH_PROP(props, Spatialized);
        // Always serialize spatial fields to preserve values on save/load.
        // Only skip display in UI mode.
        if (props.GetMode() != CHEngine::ReflectionMode::UI || Spatialized)
        {
            CH_PROP(props, Position);
            CH_PROP_META(props, MinDistance, PropertyMeta(0.1f, 100.0f, 0.5f));
            CH_PROP_META(props, MaxDistance, PropertyMeta(0.1f, 500.0f, 1.0f));
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
