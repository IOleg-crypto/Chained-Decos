#include "engine/scene/animation_systems.h"
#include "engine/scene/components.h"
#include "engine/core/log.h"

#include <entt/entt.hpp>

namespace Chained::Animation {

// Starts playback of an animation clip by index, resetting frame state.
// If the same clip is already playing and not blending, this is a no-op.
void Play(AnimationComponent& anim, int index, bool loop)
{
    if (anim.CurrentAnimationIndex == index && anim.IsPlaying && !anim.Blending)
        return;

    anim.CurrentAnimationIndex = index;
    anim.CurrentFrame = 0;
    anim.FrameTimeCounter = 0.0f;
    anim.IsLooping = loop;
    anim.IsPlaying = true;
    anim.Blending = false;
    anim.TargetAnimationIndex = -1;
}

// Initiates a cross-fade transition from the current animation to a target clip.
// The blend is performed over the specified duration; after completion the target
// becomes the current animation.
void CrossFade(AnimationComponent& anim, int index, float duration, bool loop)
{
    if (anim.CurrentAnimationIndex == index)
        return;
    if (anim.Blending && anim.TargetAnimationIndex == index)
        return;

    anim.TargetAnimationIndex = index;
    anim.TargetFrame = 0;
    anim.BlendTimer = 0.0f;
    anim.BlendDuration = (duration > 0.0f) ? duration : 0.01f;
    anim.Blending = true;
    anim.IsLooping = loop;
    anim.IsPlaying = true;
}

// Stops the current animation and clears any active blend.
void Stop(AnimationComponent& anim)
{
    anim.IsPlaying = false;
    anim.Blending = false;
}

} // namespace Chained::Animation
