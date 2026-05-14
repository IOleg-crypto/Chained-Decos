#include "engine/scene/animation_systems.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/core/log.h"

#include <entt/entt.hpp>

namespace CHEngine {
namespace AnimationSystems {

void Play(AnimationComponent& anim, int index, bool loop)
{
    if (anim.CurrentAnimationIndex == index && anim.IsPlaying && !anim.Blending)
        return;

    anim.CurrentAnimationIndex = index;
    anim.CurrentFrame = 0;
    anim.FrameTimeCounter = 0.0f;
    anim.IsLooping = loop;
    anim.IsPlaying = true;
    anim.Blending = false;
    anim.TargetAnimationIndex = -1;
}

void CrossFade(AnimationComponent& anim, int index, float duration, bool loop)
{
    if (anim.CurrentAnimationIndex == index)
        return;
    if (anim.Blending && anim.TargetAnimationIndex == index)
        return;

    anim.TargetAnimationIndex = index;
    anim.TargetFrame = 0;
    anim.BlendTimer = 0.0f;
    anim.BlendDuration = (duration > 0.0f) ? duration : 0.01f;
    anim.Blending = true;
    anim.IsLooping = loop;
    anim.IsPlaying = true;
}

void Stop(AnimationComponent& anim)
{
    anim.IsPlaying = false;
    anim.Blending = false;
}

void TriggerTransition(AnimationComponent& anim, const std::string& triggerName)
{
    if (anim.UseAnimationGraph && anim.Triggers.count(triggerName))
        anim.Triggers[triggerName] = true;
}

void UpdatePlayback(Scene* scene, Timestep ts)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<AnimationComponent>();
    for (auto entity : view)
    {
        auto& anim = view.get<AnimationComponent>(entity);
        if (!anim.IsPlaying) continue;
        anim.FrameTimeCounter += (float)ts;
        // Placeholder: real frame advancement logic depends on clip data
        // This keeps component active for runtime and allows blending logic.
    }
}

void UpdateGraphs(Scene* scene, Timestep ts)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<AnimationComponent>();
    for (auto entity : view)
    {
        auto& anim = view.get<AnimationComponent>(entity);
        if (!anim.UseAnimationGraph || anim.GraphPath.empty())
            continue;

        AnimationGraphData* graphData = scene->GetOrLoadGraph(anim.GraphPath);
        if (!graphData || graphData->Nodes.empty())
            continue;

        bool stateChanged = false;
        if (!anim.GraphInitialized || anim.CurrentStateName.empty())
        {
            anim.CurrentStateName = graphData->Nodes[0].Name;
            anim.GraphInitialized = true;
            stateChanged = true;
        }

        size_t currentIdx = 0; bool found = false;
        for (size_t i = 0; i < graphData->Nodes.size(); ++i)
        {
            if (graphData->Nodes[i].Name == anim.CurrentStateName) { currentIdx = i; found = true; break; }
        }
        if (!found) { anim.GraphInitialized = false; continue; }

        for (const auto& link : graphData->Links)
        {
            if (link.FromState != currentIdx) continue;
            const auto& fromNode = graphData->Nodes[currentIdx];
            if (link.FromSlot < fromNode.Transitions.size())
            {
                const std::string& trigger = fromNode.Transitions[link.FromSlot];
                if (anim.Triggers[trigger])
                {
                    anim.CurrentStateName = graphData->Nodes[link.ToState].Name;
                    anim.Triggers[trigger] = false;
                    stateChanged = true;
                    break;
                }
            }
        }

        if (stateChanged)
        {
            for (const auto& node : graphData->Nodes)
            {
                if (node.Name == anim.CurrentStateName)
                {
                    if (anim.AnimationPath != node.AnimationPath)
                    {
                        anim.AnimationPath = node.AnimationPath;
                        anim.IsLooping = node.IsLooping;
                        Play(anim, 0, node.IsLooping);
                    }
                    break;
                }
            }
        }
    }
}

} // namespace AnimationSystems
} // namespace CHEngine
