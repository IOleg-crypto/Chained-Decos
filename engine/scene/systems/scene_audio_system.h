#ifndef CH_SCENE_AUDIO_SYSTEM_H
#define CH_SCENE_AUDIO_SYSTEM_H

#include "engine/core/timestep.h"

namespace CHEngine
{
class Scene;

class SceneAudioSystem
{
public:
    static void Update(Scene* scene, Timestep ts);
};
} // namespace CHEngine

#endif // CH_SCENE_AUDIO_SYSTEM_H
