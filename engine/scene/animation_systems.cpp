#include "engine/scene/animation_systems.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/core/log.h"

#include <entt/entt.hpp>

namespace CHEngine {
namespace AnimationSystems {

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
                        anim.Play(0, node.IsLooping);
                    }
                    break;
                }
            }
        }
    }
}

} // namespace AnimationSystems
} // namespace CHEngine
