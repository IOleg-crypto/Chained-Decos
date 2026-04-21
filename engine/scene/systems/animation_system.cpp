#include "animation_system.h"
#include "engine/scene/scene.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/core/profiler.h"
#include <cmath>

namespace CHEngine
{
void AnimationSystem::Update(Scene* scene, Timestep ts)
{
    CH_PROFILE_FUNCTION();
    auto& reg = scene->GetRegistry();
    auto view = reg.view<AnimationComponent, ModelComponent>();

    for (auto entity : view)
    {
        auto& animation = view.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
        {
            continue;
        }

        auto& model = view.get<ModelComponent>(entity);
        auto modelAsset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetAnimationCount() == 0)
        {
            continue;
        }

        // Progress timers
        float dt = ts.GetSeconds();
        animation.FrameTimeCounter += dt;

        // Get animation frameRate from asset
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

        // Handle Blending
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
                const float FRAME_EPSILON = 0.001f;
                if (std::abs(animation.FrameTimeCounter - 0.0f) < FRAME_EPSILON)
                {
                    animation.TargetFrame++;
                    if (animation.TargetAnimationIndex >= 0 &&
                        animation.TargetAnimationIndex < modelAsset->GetAnimationCount())
                    {
                        int targetTotalFrames =
                            modelAsset->GetAnimations()[animation.TargetAnimationIndex].frameCount;
                        if (animation.TargetFrame >= targetTotalFrames)
                        {
                            animation.TargetFrame = 0;
                        }
                    }
                }
            }
        }
    }
}
} // namespace CHEngine
