#ifndef CH_ANIMATION_COMPONENT_H
#define CH_ANIMATION_COMPONENT_H

#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
struct AnimationComponent
{
    float BlendDuration = 0.25f;
    bool IsLooping = true;
    bool PlayOnStart = true;

    int CurrentAnimationIndex = 0;
    int TargetAnimationIndex = -1;
    float FrameTimeCounter = 0.0f;
    float BlendTimer = 0.0f;
    int CurrentFrame = 0;
    int TargetFrame = 0;
    bool IsPlaying = true;
    bool Blending = false;

    static const char* GetStaticName()
    {
        return "AnimationComponent";
    }

    struct UI
    {

        UIMeta BlendDuration = {
            .Min = 0.0f, .Max = 2.0f, .Speed = 0.05f, .Tooltip = "Time to blend between animations (in seconds)"};

        UIMeta IsLooping = {.Tooltip = "Whether the animation will loop"};
        UIMeta PlayOnStart = {.Tooltip = "Whether the animation will play at the start of the scene"};

        UIMeta CurrentAnimationIndex = {.Hint = PropertyMeta::WidgetHint::Enum, .ReadOnly = false, .Transient = true};
        UIMeta TargetAnimationIndex = {.ReadOnly = true, .Transient = true};
        UIMeta FrameTimeCounter = {.ReadOnly = true, .Transient = true};
        UIMeta BlendTimer = {.ReadOnly = true, .Transient = true};
        UIMeta CurrentFrame = {.ReadOnly = true, .Transient = true};
        UIMeta TargetFrame = {.ReadOnly = true, .Transient = true};
        UIMeta IsPlaying = {.Hint = PropertyMeta::WidgetHint::Checkbox, .ReadOnly = false, .Transient = true};
        UIMeta Blending = {.ReadOnly = true, .Transient = true};
    };
};

CH_MARK_RFL(AnimationComponent);

} // namespace Chained

#endif // CH_ANIMATION_COMPONENT_H