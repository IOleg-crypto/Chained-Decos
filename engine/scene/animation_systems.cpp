#include "engine/scene/animation_systems.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/core/log.h"

#include <entt/entt.hpp>

namespace CHEngine {
namespace AnimationSystems {

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

void Stop(AnimationComponent& anim)
{
    anim.IsPlaying = false;
    anim.Blending = false;
}

void TriggerTransition(AnimationComponent& anim, const std::string& triggerName)
{
    // Animation graphs removed: no-op
}

void UpdatePlayback(Scene* scene, Timestep ts)
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

void UpdateGraphs(Scene* scene, Timestep ts)
{
    // Animation graphs removed: no-op
}

} // namespace AnimationSystems
} // namespace CHEngine
