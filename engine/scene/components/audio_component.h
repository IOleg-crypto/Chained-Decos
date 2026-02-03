#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/core/base.h"
#include <string>
#include "../reflect.h"

namespace CHEngine
{
class SoundAsset;

struct AudioComponent
{
    std::string SoundPath;
    float Volume = 1.0f;
    float Pitch = 1.0f;
    bool Loop = false;
    bool PlayOnStart = true;

    // Runtime
    std::shared_ptr<SoundAsset> Asset;
};

BEGIN_REFLECT(AudioComponent)
    PROPERTY(std::string, SoundPath, "Path")
    PROPERTY(float, Volume, "Volume")
    PROPERTY(float, Pitch, "Pitch")
    PROPERTY(bool, Loop, "Looping")
    PROPERTY(bool, PlayOnStart, "Play On Start")
END_REFLECT()
} // namespace CHEngine

#endif // CH_AUDIO_COMPONENT_H
