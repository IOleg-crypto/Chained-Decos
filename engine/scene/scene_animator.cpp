#include "scene_animator.h"
#include "components.h"
#include "engine/core/async_utils.h"
#include "engine/core/profiler.h"
#include "engine/renderer/model_asset.h"
#include "project.h"
#include "scene.h"

namespace CHEngine
{
void SceneAnimator::OnUpdate(float deltaTime)
{
    CH_PROFILE_FUNCTION();
    auto &registry = m_Scene->GetRegistry();
    auto view = registry.view<AnimationComponent, ModelComponent>();

    std::vector<entt::entity> entities;
    entities.reserve(view.size_hint());
    for (auto entity : view)
        entities.push_back(entity);

    if (entities.empty())
        return;

    size_t count = entities.size();
    ParallelFor(
        count,
        [&](size_t i)
        {
            auto entity = entities[i];
            auto &anim = registry.get<AnimationComponent>(entity);
            auto &model = registry.get<ModelComponent>(entity);

            if (anim.IsPlaying && model.Asset)
            {
                int animCount = 0;
                auto *animations = model.Asset->GetAnimations(&animCount);

                if (animations && anim.CurrentAnimationIndex < animCount)
                {
                    anim.FrameTimeCounter += deltaTime;

                    float targetFPS = 30.0f;
                    if (Project::GetActive())
                        targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;

                    float frameTime = 1.0f / targetFPS;

                    if (anim.FrameTimeCounter >= frameTime)
                    {
                        anim.CurrentFrame++;
                        anim.FrameTimeCounter = 0;

                        if (anim.CurrentFrame >= animations[anim.CurrentAnimationIndex].frameCount)
                        {
                            if (anim.IsLooping)
                                anim.CurrentFrame = 0;
                            else
                            {
                                anim.CurrentFrame =
                                    animations[anim.CurrentAnimationIndex].frameCount - 1;
                                anim.IsPlaying = false;
                            }
                        }
                        model.Asset->UpdateAnimation(anim.CurrentAnimationIndex, anim.CurrentFrame);
                    }
                }
            }
        });
}
} // namespace CHEngine
