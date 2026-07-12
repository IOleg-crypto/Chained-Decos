#ifndef CH_ANIMATION_SYSTEMS_H
#define CH_ANIMATION_SYSTEMS_H
#include "engine/common/timestep.h"
#include <string>

namespace Chained {
class Scene;
struct AnimationComponent;

namespace Animation {
    void Play(AnimationComponent& anim, int index, bool loop = true);
    void CrossFade(AnimationComponent& anim, int index, float duration = 0.2f, bool loop = true);
    void Stop(AnimationComponent& anim);
    void UpdatePlayback(Scene* scene, Timestep ts);
}
} // namespace Chained

#endif // CH_ANIMATION_SYSTEMS_H
