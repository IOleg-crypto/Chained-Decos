#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/core/reflection_rfl.h"
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

    static const char* GetStaticName() { return "AudioComponent"; }
};

CH_MARK_RFL(AudioComponent);

} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
