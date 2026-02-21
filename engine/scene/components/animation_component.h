#ifndef CH_ANIMATION_COMPONENT_H
#define CH_ANIMATION_COMPONENT_H

#include "raylib.h"
#include <string>
#include <vector>

namespace CHEngine
{
struct AnimationComponent
{
    std::string AnimationPath;
    int CurrentAnimationIndex = 0;
    int TargetAnimationIndex = -1;
    float FrameTimeCounter = 0.0f;
    float BlendTimer = 0.0f;
    float BlendDuration = 0.0f;
    int CurrentFrame = 0;
    int TargetFrame = 0;
    bool IsLooping = true;
    bool IsPlaying = true;
    bool Blending = false;

    AnimationComponent() = default;
    AnimationComponent(const AnimationComponent&) = default;
    AnimationComponent(const std::string& path)
        : AnimationPath(path)
    {
    }

    void Play(int index, bool loop = true)
    {
        if (CurrentAnimationIndex == index && IsPlaying && !Blending)
        {
            return;
        }
        CurrentAnimationIndex = index;
        CurrentFrame = 0;
        FrameTimeCounter = 0.0f;
        IsLooping = loop;
        IsPlaying = true;
        Blending = false;
        TargetAnimationIndex = -1;
    }

    void CrossFade(int index, float duration = 0.2f, bool loop = true)
    {
        if (CurrentAnimationIndex == index)
        {
            return;
        }
        if (Blending && TargetAnimationIndex == index)
        {
            return;
        }

        TargetAnimationIndex = index;
        TargetFrame = 0;
        BlendTimer = 0.0f;
        BlendDuration = (duration > 0.0f) ? duration : 0.01f;
        Blending = true;
        IsLooping = loop;
        IsPlaying = true;
    }

    void Stop()
    {
        IsPlaying = false;
        Blending = false;
    }
};

} // namespace CHEngine

#endif // CH_ANIMATION_COMPONENT_H
