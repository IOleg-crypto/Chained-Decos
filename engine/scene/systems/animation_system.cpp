#include "animation_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/model_component.h"

namespace Chained::AnimationSystem
{

void Update(entt::registry& reg, Timestep ts)
{
    CH_PROFILE_FUNCTION();

    auto animView = reg.view<AnimationComponent, ModelComponent>();
    for (auto entity : animView)
    {
        auto& animation = animView.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
        {
            continue;
        }

        auto& model = animView.get<ModelComponent>(entity);

        auto* assets = ServiceLocator::TryGet<AssetManager>();
        if (!assets) continue;

        auto handle = assets->LoadAsset(model.ModelPath, ModelAsset::GetStaticType());
        auto modelAsset = assets->Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetAnimationCount() == 0)
        {
            continue;
        }

        int animCount = modelAsset->GetAnimationCount();
        if (animation.CurrentAnimationIndex >= animCount)
        {
            animation.CurrentAnimationIndex = 0;
            animation.CurrentFrame = 0;
        }
        if (animation.TargetAnimationIndex >= animCount)
        {
            animation.TargetAnimationIndex = -1;
        }

        float dt = ts.GetSeconds();
        animation.FrameTimeCounter += dt;

        float targetFPS = 30.0f;
        const auto& rawAnims = modelAsset->GetAnimations();
        if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
        {
            targetFPS = rawAnims[animation.CurrentAnimationIndex].frameRate;
        }
        float frameTime = 1.0f / targetFPS;

        while (animation.FrameTimeCounter >= frameTime)
        {
            animation.FrameTimeCounter -= frameTime;
            animation.CurrentFrame++;

            if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
            {
                int totalFrames = rawAnims[animation.CurrentAnimationIndex].frameCount;
                if (animation.CurrentFrame >= totalFrames)
                {
                    if (animation.IsLooping)
                    {
                        animation.CurrentFrame = 0;
                    }
                    else
                    {
                        animation.CurrentFrame = totalFrames - 1;
                        animation.IsPlaying = false;
                    }
                }
            }
        }

        if (animation.Blending)
        {
            animation.BlendTimer += dt;
            if (animation.BlendTimer >= animation.BlendDuration)
            {
                animation.CurrentAnimationIndex = animation.TargetAnimationIndex;
                animation.CurrentFrame = animation.TargetFrame;
                animation.Blending = false;
                animation.TargetAnimationIndex = -1;
            }
            else
            {
                animation.TargetFrame++;
                if (animation.TargetAnimationIndex >= 0 &&
                    animation.TargetAnimationIndex < modelAsset->GetAnimationCount())
                {
                    int targetTotalFrames = modelAsset->GetAnimations()[animation.TargetAnimationIndex].frameCount;
                    if (animation.TargetFrame >= targetTotalFrames)
                    {
                        animation.TargetFrame = 0;
                    }
                }
            }
        }
    }
}

} // namespace Chained::AnimationSystem
