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

    CH_REFLECT_BEGIN(AnimationComponent)
        // Startup
        CH_PROP_NAMED(props, "Play On Start", PlayOnStart);

        // Basic playback
        {
            PropertyMeta meta;
            meta.ReadOnly = true;
            CH_PROP_META_NAMED(props, "Animation Path", AnimationPath, meta);
        }
        CH_PROP_NAMED(props, "Current Animation Index", CurrentAnimationIndex);
        CH_PROP_NAMED(props, "Is Looping", IsLooping);
        CH_PROP_NAMED(props, "Is Playing", IsPlaying);
        CH_PROP_META_NAMED(props, "Blend Duration", BlendDuration, PropertyMeta(0.0f, 5.0f, 0.01f));

        // Graph settings
        CH_PROP_NAMED(props, "Use Animation Graph", UseAnimationGraph);
        CH_FILE_NAMED(props, "Animation Graph File", GraphPath, "chanim");
        {
            PropertyMeta meta;
            meta.ReadOnly = true;
            CH_PROP_META_NAMED(props, "Graph Current State", CurrentStateName, meta);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_ANIMATION_COMPONENT_H
