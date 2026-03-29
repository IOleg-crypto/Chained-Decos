#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/base.h"
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

    static const char* GetStaticName() { return "AudioComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, AudioComponent& component)
    {
        archive.Handle("SoundHandle", component.SoundHandle)
            .Path("SoundPath", component.SoundPath)
            .Property("Loop", component.Loop)
            .Property("PlayOnStart", component.PlayOnStart)
            .Property("Volume", component.Volume)
            .Property("Pitch", component.Pitch);
    }
};

} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
