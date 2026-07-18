#ifndef CH_ANIMATION_SYSTEMS_H
#define CH_ANIMATION_SYSTEMS_H
#include <string>

namespace Chained {
struct AnimationComponent;

namespace Animation {
    void Play(AnimationComponent& anim, int index, bool loop = true);
    void CrossFade(AnimationComponent& anim, int index, float duration = 0.2f, bool loop = true);
    void Stop(AnimationComponent& anim);
}
} // namespace Chained

#endif // CH_ANIMATION_SYSTEMS_H
