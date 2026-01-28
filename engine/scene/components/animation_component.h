#ifndef CH_ANIMATION_COMPONENT_H
#define CH_ANIMATION_COMPONENT_H

#include "raylib.h"
#include "string"
#include "vector"

namespace CHEngine
{
struct AnimationComponent
{
    std::string AnimationPath;
    int CurrentAnimationIndex = 0;
    float FrameTimeCounter = 0.0f;
    int CurrentFrame = 0;
    bool IsLooping = true;
    bool IsPlaying = true;

    // Runtime data (not serialized directly or handled by AssetManager)
    // ModelAnimation *Animations = nullptr;
    // int AnimationCount = 0;

    AnimationComponent() = default;
    AnimationComponent(const AnimationComponent &) = default;
    AnimationComponent(const std::string &path) : AnimationPath(path)
    {
    }

    void Play(int index, bool loop = true)
    {
        CurrentAnimationIndex = index;
        CurrentFrame = 0;
        FrameTimeCounter = 0.0f;
        IsLooping = loop;
        IsPlaying = true;
    }

    void Stop()
    {
        IsPlaying = false;
    }
};
} // namespace CHEngine

#endif // CH_ANIMATION_COMPONENT_H
