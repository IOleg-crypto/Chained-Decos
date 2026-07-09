#include "engine/scene/animation_systems.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/core/log.h"

#include <entt/entt.hpp>

namespace Chained {

// Starts playback of an animation clip by index, resetting frame state.
// If the same clip is already playing and not blending, this is a no-op.
void AnimationManager::Play(AnimationComponent& anim, int index, bool loop)
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
void AnimationManager::CrossFade(AnimationComponent& anim, int index, float duration, bool loop)
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
void AnimationManager::Stop(AnimationComponent& anim)
{
    anim.IsPlaying = false;
    anim.Blending = false;
}

// Advances frame timers for all playing AnimationComponents in the scene.
// TODO: Replace placeholder frame advancement with real clip-based keyframe interpolation.
void AnimationManager::UpdatePlayback(Scene* scene, Timestep ts)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<AnimationComponent>();
    for (auto entity : view)
    {
        auto& anim = view.get<AnimationComponent>(entity);
        if (!anim.IsPlaying) continue;
        anim.FrameTimeCounter += (float)ts;
        // Placeholder: real frame advancement logic depends on clip data
        // This keeps component active for runtime and allows blending logic.
    }
}

} // namespace Chained
