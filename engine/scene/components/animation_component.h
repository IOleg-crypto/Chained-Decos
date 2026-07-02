#ifndef CH_ANIMATION_COMPONENT_H
#define CH_ANIMATION_COMPONENT_H

#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include <string>

namespace Chained
{
struct AnimationComponent
{
    
    std::string AnimationPath;
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
        
        UIMeta AnimationPath = {.Hint = PropertyMeta::WidgetHint::FilePicker, .Extensions = ".gltf,.fbx,.anim"};

        
        UIMeta BlendDuration = {
            .Min = 0.0f, .Max = 2.0f, .Speed = 0.05f, .Tooltip = "Час плавної зміни анімацій (в секундах)"};

        
        UIMeta IsLooping = {.Tooltip = "Чи буде анімація зациклюватися"};
        UIMeta PlayOnStart = {.Tooltip = "Чи буде анімація відтворюватися при старті сцени"};
        
        
        UIMeta CurrentAnimationIndex = {.ReadOnly = true, .Transient = true};
        UIMeta TargetAnimationIndex = {.ReadOnly = true, .Transient = true};
        UIMeta FrameTimeCounter = {.ReadOnly = true, .Transient = true};
        UIMeta BlendTimer = {.ReadOnly = true, .Transient = true};
        UIMeta CurrentFrame = {.ReadOnly = true, .Transient = true};
        UIMeta TargetFrame = {.ReadOnly = true, .Transient = true};
        UIMeta IsPlaying = {.ReadOnly = true, .Transient = true};
        UIMeta Blending = {.ReadOnly = true, .Transient = true};
    };
};

CH_MARK_RFL(AnimationComponent);

} // namespace Chained

#endif // CH_ANIMATION_COMPONENT_H