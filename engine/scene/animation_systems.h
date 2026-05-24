#ifndef CH_ANIMATION_SYSTEMS_H
#define CH_ANIMATION_SYSTEMS_H
#include "engine/core/timestep.h"
#include <string>

namespace CHEngine {
class Scene;
struct AnimationComponent;
namespace AnimationSystems {
    void Play(AnimationComponent& anim, int index, bool loop = true);
    void CrossFade(AnimationComponent& anim, int index, float duration = 0.2f, bool loop = true);
    void Stop(AnimationComponent& anim);
    void UpdatePlayback(Scene* scene, Timestep ts);
}
}


#endif // CH_ANIMATION_SYSTEMS_H
