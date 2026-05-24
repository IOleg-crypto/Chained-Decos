#ifndef CH_ANIMATION_COMPONENT_H
#define CH_ANIMATION_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/core/reflection_rfl.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace CHEngine
{
struct AnimationComponent
{
    // Playback state
    std::string AnimationPath;
    int CurrentAnimationIndex = 0;
    int TargetAnimationIndex = -1;
    float FrameTimeCounter = 0.0f;
    float BlendTimer = 0.0f;
    float BlendDuration = 0.25f;
    int CurrentFrame = 0;
    int TargetFrame = 0;
    bool IsLooping = true;
    bool IsPlaying = true;
    bool Blending = false;
    bool PlayOnStart = true;

    // Animation graph state (merged)
    // (animation-graph removed)

    static const char* GetStaticName() { return "AnimationComponent"; }
};

CH_MARK_RFL(AnimationComponent);



} // namespace CHEngine

#endif // CH_ANIMATION_COMPONENT_H
