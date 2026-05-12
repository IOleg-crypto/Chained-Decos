#ifndef CH_ANIMATION_COMPONENT_H
#define CH_ANIMATION_COMPONENT_H

#include "engine/core/reflection.h"
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
    std::string GraphPath;
    bool UseAnimationGraph = false;
    std::string CurrentStateName;
    bool GraphInitialized = false;
    std::unordered_map<std::string, bool> Triggers;

    AnimationComponent() = default;
    AnimationComponent(const AnimationComponent&) = default;
    AnimationComponent(const std::string& path)
        : AnimationPath(path)
    {
    }

    // Direct animation control
    void Play(int index, bool loop = true)
    {
        if (CurrentAnimationIndex == index && IsPlaying && !Blending)
            return;

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
            return;
        if (Blending && TargetAnimationIndex == index)
            return;

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

    // Graph trigger interface
    void TriggerTransition(const std::string& triggerName)
    {
        if (UseAnimationGraph && Triggers.count(triggerName))
        {
            Triggers[triggerName] = true;
        }
    }

    CH_REFLECT_BEGIN(AnimationComponent)
        // Startup
        props.Property("Play On Start", PlayOnStart);

        // Basic playback
        {
            PropertyMeta meta;
            meta.ReadOnly = true;
            props.Property("Animation Path", AnimationPath, meta);
        }
        props.Property("Current Animation Index", CurrentAnimationIndex);
        props.Property("Is Looping", IsLooping);
        props.Property("Is Playing", IsPlaying);
        props.Property("Blend Duration", BlendDuration, PropertyMeta(0.0f, 5.0f, 0.01f));

        // Graph settings
        props.Property("Use Animation Graph", UseAnimationGraph);
        props.File("Animation Graph File", GraphPath, "chanim");
        {
            PropertyMeta meta;
            meta.ReadOnly = true;
            props.Property("Graph Current State", CurrentStateName, meta);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_ANIMATION_COMPONENT_H
