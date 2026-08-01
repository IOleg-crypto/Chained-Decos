#include "animation_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/animation_graph.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/scene.h"
#include <algorithm>

namespace Chained::AnimationSystem
{

static void EvaluateGraph(AnimationComponent& anim, const ModelComponent& model)
{
    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    if (anim.GraphAssetHandle == 0 && !anim.GraphPath.empty())
    {
        auto asset = assets->LoadAsset(anim.GraphPath, AssetType::AnimationGraph);
        if (asset)
        {
            anim.GraphAssetHandle = asset->GetID();
        }
    }

    auto graphAsset = assets->Get<AnimationGraphAsset>(anim.GraphAssetHandle);
    if (!graphAsset)
    {
        return;
    }

    if (anim.CurrentNodeID == -1)
    {
        anim.CurrentNodeID = graphAsset->EntryNodeID;
    }

    AnimNode* currentNode = graphAsset->FindNode(anim.CurrentNodeID);
    if (!currentNode)
    {
        return;
    }

    // Ensure animation and node properties match current state when not blending
    if (!anim.Blending)
    {
        anim.CurrentAnimationIndex = currentNode->AnimationIndex;
        anim.IsLooping = currentNode->IsLooping;
        anim.StartFrame = currentNode->StartFrame;
        anim.EndFrame = currentNode->EndFrame;
        anim.Speed = (currentNode->Speed > 0.0f) ? currentNode->Speed : 1.0f;
    }

    // Collect valid transitions from current node
    struct ValidTransition
    {
        const AnimTransition* transition;
        AnimNode* targetNode;
    };
    std::vector<ValidTransition> validTransitions;

    for (auto& link : graphAsset->Transitions)
    {
        if (link.SourceNodeID == anim.CurrentNodeID)
        {
            if (link.HasExitTime)
            {
                float exitTimeRef = 0.0f;
                switch (link.ExitTimeMode)
                {
                case AnimTransition::ExitTimeMode::SourceAnimation:
                    exitTimeRef = anim.NormalizedTime;
                    break;
                case AnimTransition::ExitTimeMode::TargetAnimation:
                    // For target animation, we'd need target's normalized time
                    // For now, fall back to source
                    exitTimeRef = anim.NormalizedTime;
                    break;
                case AnimTransition::ExitTimeMode::AbsoluteTime:
                    // ExitTime in seconds from state entry
                    exitTimeRef = (anim.Duration > 0.0001f) ? (anim.CurrentTime / anim.Duration) : 0.0f;
                    break;
                }
                if (!anim.IsFinished && exitTimeRef < link.ExitTime)
                {
                    continue;
                }
            }

            bool conditionsMet = true;
            for (auto& cond : link.Conditions)
            {
                if (!cond.Evaluate(anim.Variables))
                {
                    conditionsMet = false;
                    break;
                }
            }

            if (conditionsMet)
            {
                AnimNode* targetNode = graphAsset->FindNode(link.TargetNodeID);
                if (targetNode)
                {
                    validTransitions.push_back({&link, targetNode});
                }
            }
        }
    }

    // Sort by priority (higher first)
    std::sort(validTransitions.begin(), validTransitions.end(), [](const ValidTransition& a, const ValidTransition& b) {
        return a.transition->Priority > b.transition->Priority;
    });

    // Take highest priority transition
    if (!validTransitions.empty() && !anim.Blending)
    {
        const auto& best = validTransitions[0];
        const AnimTransition* link = best.transition;
        AnimNode* targetNode = best.targetNode;

        // Store target info for blend - capture target node params
        anim.TargetNodeID = link->TargetNodeID;
        anim.TargetAnimationIndex = targetNode->AnimationIndex;
        anim.TargetFrame = targetNode->StartFrame;
        anim.TargetFrameTimeCounter = 0.0f; // Initialize target frame counter
        anim.TargetStartFrame = targetNode->StartFrame;
        anim.TargetEndFrame = targetNode->EndFrame;
        anim.TargetSpeed = (targetNode->Speed > 0.0f) ? targetNode->Speed : 1.0f;
        anim.TargetIsLooping = targetNode->IsLooping;
        anim.BlendDuration = link->BlendDuration;
        anim.BlendTimer = 0.0f;
        anim.Blending = true;
        anim.IsPlaying = true;
        anim.IsFinished = false;

        // Target node params will be applied after blend completes
        // Current anim params (StartFrame, EndFrame, Speed, IsLooping) remain unchanged during blend
    }
}

static void ProcessPlayback(entt::registry& reg, AnimationComponent& anim, const ModelComponent& model, Timestep ts)
{
    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    auto handle = assets->LoadAsset(model.ModelPath, ModelAsset::GetStaticType());
    auto modelAsset = assets->Get<ModelAsset>(model.ModelPath);
    if (!modelAsset || modelAsset->GetAnimationCount() == 0)
    {
        return;
    }

    int animCount = modelAsset->GetAnimationCount();
    if (anim.CurrentAnimationIndex >= animCount)
    {
        anim.CurrentAnimationIndex = 0;
        anim.CurrentFrame = 0;
        anim.IsFinished = false;
    }
    if (anim.TargetAnimationIndex >= animCount)
    {
        anim.TargetAnimationIndex = -1;
    }

    const auto& rawAnims = modelAsset->GetAnimations();

    // ===== CURRENT ANIMATION TIMING =====
    float currentFPS = 30.0f;
    int currentTotalFrames = 1;
    if (anim.CurrentAnimationIndex >= 0 && anim.CurrentAnimationIndex < (int)rawAnims.size())
    {
        currentFPS = rawAnims[anim.CurrentAnimationIndex].frameRate;
        currentTotalFrames = rawAnims[anim.CurrentAnimationIndex].frameCount;
    }
    if (currentFPS <= 0.0f)
    {
        currentFPS = 30.0f;
    }
    if (currentTotalFrames <= 0)
    {
        currentTotalFrames = 1;
    }

    int currentEffectiveStart = std::clamp(anim.StartFrame, 0, currentTotalFrames - 1);
    int currentEffectiveEnd = (anim.EndFrame < 0)
                                  ? (currentTotalFrames - 1)
                                  : std::clamp(anim.EndFrame, currentEffectiveStart, currentTotalFrames - 1);
    int currentEffectiveLength = currentEffectiveEnd - currentEffectiveStart + 1;

    if (anim.CurrentFrame < currentEffectiveStart || anim.CurrentFrame > currentEffectiveEnd)
    {
        anim.CurrentFrame = currentEffectiveStart;
    }

    float currentFrameTime = 1.0f / currentFPS;
    anim.Duration = (float)currentEffectiveLength / currentFPS;

    // ===== TARGET ANIMATION TIMING (for blend) =====
    float targetFPS = currentFPS;
    int targetTotalFrames = currentTotalFrames;
    int targetEffectiveStart = currentEffectiveStart;
    int targetEffectiveEnd = currentEffectiveEnd;
    int targetEffectiveLength = currentEffectiveLength;

    if (anim.Blending && anim.TargetAnimationIndex >= 0 && anim.TargetAnimationIndex < (int)rawAnims.size())
    {
        targetFPS = rawAnims[anim.TargetAnimationIndex].frameRate;
        targetTotalFrames = rawAnims[anim.TargetAnimationIndex].frameCount;
        if (targetFPS <= 0.0f)
        {
            targetFPS = 30.0f;
        }
        if (targetTotalFrames <= 0)
        {
            targetTotalFrames = 1;
        }

        // Use TARGET node's frame range (captured at transition trigger)
        targetEffectiveStart = std::clamp(anim.TargetStartFrame, 0, targetTotalFrames - 1);
        targetEffectiveEnd = (anim.TargetEndFrame < 0)
                                 ? (targetTotalFrames - 1)
                                 : std::clamp(anim.TargetEndFrame, targetEffectiveStart, targetTotalFrames - 1);
        targetEffectiveLength = targetEffectiveEnd - targetEffectiveStart + 1;

        // Initialize TargetFrame if not set
        if (anim.TargetFrame < targetEffectiveStart || anim.TargetFrame > targetEffectiveEnd)
        {
            anim.TargetFrame = targetEffectiveStart;
        }
    }

    bool isRuntimePlay = false;
    if (auto* scenePtr = reg.ctx().find<Scene*>())
    {
        if (*scenePtr && (*scenePtr)->IsSimulationRunning())
        {
            isRuntimePlay = true;
        }
    }

    if (anim.IsPlaying)
    {
        float dt = ts.GetSeconds();
        float baseSpeed = anim.Speed > 0.0f ? anim.Speed : 0.0f;
        if (baseSpeed <= 0.0f)
        {
            baseSpeed = 1.0f;
        }
        if (isRuntimePlay)
        {
            auto itSpd = anim.Variables.find("speed");
            if (itSpd == anim.Variables.end())
            {
                itSpd = anim.Variables.find("Speed");
            }
            if (itSpd != anim.Variables.end() && itSpd->second >= 0.0f)
            {
                baseSpeed *= itSpd->second;
            }
        }

        // ===== BLEND HANDLING =====
        if (anim.Blending)
        {
            anim.BlendTimer += dt;
            float blendAlpha =
                (anim.BlendDuration > 0.0f) ? glm::clamp(anim.BlendTimer / anim.BlendDuration, 0.0f, 1.0f) : 1.0f;

            // Interpolate parameters during blend (Speed, StartFrame, EndFrame)
            // Note: IsLooping switches at blend end (handled below)
            float interpSpeed = glm::mix(anim.Speed, anim.TargetSpeed, blendAlpha);
            interpSpeed = interpSpeed > 0.0f ? interpSpeed : 1.0f;
            if (isRuntimePlay)
            {
                auto itSpd = anim.Variables.find("speed");
                if (itSpd == anim.Variables.end())
                {
                    itSpd = anim.Variables.find("Speed");
                }
                if (itSpd != anim.Variables.end() && itSpd->second >= 0.0f)
                {
                    interpSpeed *= itSpd->second;
                }
            }
            int interpStartFrame =
                (int)glm::round(glm::mix((float)anim.StartFrame, (float)anim.TargetStartFrame, blendAlpha));
            int interpEndFrame =
                (int)glm::round(glm::mix((float)anim.EndFrame, (float)anim.TargetEndFrame, blendAlpha));

            // Recalculate effective range with interpolated StartFrame/EndFrame
            int interpEffectiveStart = std::clamp(interpStartFrame, 0, currentTotalFrames - 1);
            int interpEffectiveEnd = (interpEndFrame < 0)
                                         ? (currentTotalFrames - 1)
                                         : std::clamp(interpEndFrame, interpEffectiveStart, currentTotalFrames - 1);
            float interpFrameTime = 1.0f / currentFPS;

            // Advance current animation with interpolated speed
            anim.FrameTimeCounter += dt * interpSpeed;
            while (anim.FrameTimeCounter >= interpFrameTime)
            {
                anim.FrameTimeCounter -= interpFrameTime;
                anim.CurrentFrame++;

                if (anim.CurrentFrame > interpEffectiveEnd)
                {
                    if (anim.IsLooping)
                    {
                        anim.CurrentFrame = interpEffectiveStart;
                        anim.IsFinished = false;
                    }
                    else
                    {
                        anim.CurrentFrame = interpEffectiveEnd;
                        anim.IsPlaying = false;
                        anim.IsFinished = true;
                    }
                }
            }

            anim.CurrentTime =
                (float)(anim.CurrentFrame - interpEffectiveStart) * interpFrameTime + anim.FrameTimeCounter;
            anim.NormalizedTime =
                (anim.Duration > 0.0001f) ? std::clamp(anim.CurrentTime / anim.Duration, 0.0f, 1.0f) : 0.0f;

            // Advance target animation in ITS OWN FPS
            float targetFrameTime = 1.0f / targetFPS;
            anim.TargetFrameTimeCounter += dt * interpSpeed; // Use interpolated speed for consistency
            while (anim.TargetFrameTimeCounter >= targetFrameTime)
            {
                anim.TargetFrameTimeCounter -= targetFrameTime;
                anim.TargetFrame++;

                if (anim.TargetFrame > targetEffectiveEnd)
                {
                    if (anim.TargetIsLooping)
                    {
                        anim.TargetFrame = targetEffectiveStart;
                    }
                    else
                    {
                        anim.TargetFrame = targetEffectiveEnd;
                    }
                }
            }

            // Check if blend is complete
            if (anim.BlendTimer >= anim.BlendDuration)
            {
                // ===== BLEND COMPLETE - Switch to target animation =====
                // Convert target time to new current animation time
                float targetCurrentTime =
                    (float)(anim.TargetFrame - targetEffectiveStart) * targetFrameTime + anim.TargetFrameTimeCounter;

                // New current animation params (from target node)
                float newCurrentFPS = targetFPS;
                int newCurrentTotalFrames = targetTotalFrames;
                int newEffectiveStart = targetEffectiveStart;
                int newEffectiveEnd = targetEffectiveEnd;
                int newEffectiveLength = newEffectiveEnd - newEffectiveStart + 1;
                float newFrameTime = 1.0f / newCurrentFPS;

                anim.CurrentAnimationIndex = anim.TargetAnimationIndex;
                anim.CurrentFrame = newEffectiveStart + (int)(targetCurrentTime / newFrameTime);
                anim.CurrentFrame = std::clamp(anim.CurrentFrame, newEffectiveStart, newEffectiveEnd);
                anim.FrameTimeCounter = fmodf(targetCurrentTime, newFrameTime);

                // Apply target node parameters
                anim.StartFrame = anim.TargetStartFrame;
                anim.EndFrame = anim.TargetEndFrame;
                anim.Speed = anim.TargetSpeed;
                anim.IsLooping = anim.TargetIsLooping;

                // Update graph state
                anim.CurrentNodeID = anim.TargetNodeID;
                anim.TargetNodeID = -1;

                // Clear blend state
                anim.Blending = false;
                anim.TargetAnimationIndex = -1;
                anim.TargetFrame = 0;
                anim.TargetFrameTimeCounter = 0.0f;
                anim.IsFinished = false;
            }
        }
        else
        {
            // Non-blending: advance with current params
            anim.FrameTimeCounter += dt * baseSpeed;

            while (anim.FrameTimeCounter >= currentFrameTime)
            {
                anim.FrameTimeCounter -= currentFrameTime;
                anim.CurrentFrame++;

                if (anim.CurrentFrame > currentEffectiveEnd)
                {
                    if (anim.IsLooping)
                    {
                        anim.CurrentFrame = currentEffectiveStart;
                        anim.IsFinished = false;
                    }
                    else
                    {
                        anim.CurrentFrame = currentEffectiveEnd;
                        anim.IsPlaying = false;
                        anim.IsFinished = true;
                    }
                }
            }

            anim.CurrentTime =
                (float)(anim.CurrentFrame - currentEffectiveStart) * currentFrameTime + anim.FrameTimeCounter;
            anim.NormalizedTime =
                (anim.Duration > 0.0001f) ? std::clamp(anim.CurrentTime / anim.Duration, 0.0f, 1.0f) : 0.0f;
        }
    }
}

void Update(entt::registry& reg, Timestep ts)
{
    CH_PROFILE_FUNCTION();

    auto view = reg.view<AnimationComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto& anim = view.get<AnimationComponent>(entity);
        auto& model = view.get<ModelComponent>(entity);

        if (anim.IsGraphDriven())
        {
            EvaluateGraph(anim, model);
        }

        ProcessPlayback(reg, anim, model, ts);
    }
}

} // namespace Chained::AnimationSystem
