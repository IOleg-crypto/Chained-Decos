#include "animation_graph_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/entity.h"
#include "engine/scene/animation_systems.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/animation_graph_data.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <unordered_map>

namespace CHEngine {

struct CachedGraph {
    AnimationGraphData Data;
    std::filesystem::file_time_type LastWriteTime;
};

static std::unordered_map<std::string, CachedGraph> s_GraphCache;

static void LoadGraph(const std::string& path, AnimationGraphData& outData)
{
    try {
        YAML::Node root = YAML::LoadFile(path);
        if (!root["Nodes"]) return;

        outData.Nodes.clear();
        outData.Links.clear();

        for (auto node : root["Nodes"])
        {
            AnimationState state;
            state.Name = node["Name"].as<std::string>();
            state.AnimationPath = node["AnimationPath"].as<std::string>();
            state.IsLooping = node["IsLooping"].as<bool>(true);
            if (node["Transitions"])
            {
                for (auto t : node["Transitions"])
                    state.Transitions.push_back(t.as<std::string>());
            }
            outData.Nodes.push_back(state);
        }

        if (root["Links"])
        {
            for (auto link : root["Links"])
            {
                outData.Links.push_back({
                    link["FromState"].as<size_t>(),
                    link["FromSlot"].as<size_t>(),
                    link["ToState"].as<size_t>(),
                    link["ToSlot"].as<size_t>()
                });
            }
        }
    }
    catch (const std::exception& e) {
        CH_CORE_ERROR("Failed to load animation graph {0}: {1}", path, e.what());
    }
}

void AnimationGraphSystem::Update(Scene* scene, Timestep ts)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<AnimationComponent>();
    
    for (auto entity : view)
    {
        auto& anim = view.get<AnimationComponent>(entity);
        
        // Skip if graph not enabled or path not set
        if (!anim.UseAnimationGraph || anim.GraphPath.empty())
            continue;

        // Get absolute path
        std::string absolutePath;
        if (Project::GetActive())
            absolutePath = (Project::GetActive()->GetProjectDirectory() / anim.GraphPath).generic_string();
        else
            absolutePath = anim.GraphPath;

        if (!std::filesystem::exists(absolutePath))
            continue;

        // Load or cache graph
        auto lastWriteTime = std::filesystem::last_write_time(absolutePath);
        if (!s_GraphCache.contains(anim.GraphPath) || 
            s_GraphCache[anim.GraphPath].LastWriteTime < lastWriteTime)
        {
            LoadGraph(absolutePath, s_GraphCache[anim.GraphPath].Data);
            s_GraphCache[anim.GraphPath].LastWriteTime = lastWriteTime;
        }

        auto& graphData = s_GraphCache[anim.GraphPath].Data;
        if (graphData.Nodes.empty())
            continue;

        // Initialize to first state if needed
        bool stateChanged = false;
        if (!anim.GraphInitialized || anim.CurrentStateName.empty())
        {
            anim.CurrentStateName = graphData.Nodes[0].Name;
            anim.GraphInitialized = true;
            stateChanged = true;
        }

        // Find current state index
        size_t currentStateIdx = 0;
        bool found = false;
        for (size_t i = 0; i < graphData.Nodes.size(); i++)
        {
            if (graphData.Nodes[i].Name == anim.CurrentStateName)
            {
                currentStateIdx = i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            anim.GraphInitialized = false;
            continue;
        }

        // Check transitions
        for (const auto& link : graphData.Links)
        {
            if (link.FromState == currentStateIdx)
            {
                const auto& fromNode = graphData.Nodes[currentStateIdx];
                if (link.FromSlot < fromNode.Transitions.size())
                {
                    std::string triggerName = fromNode.Transitions[link.FromSlot];
                    if (anim.Triggers[triggerName])
                    {
                        // Trigger fired, transition to next state
                        anim.CurrentStateName = graphData.Nodes[link.ToState].Name;
                        anim.Triggers[triggerName] = false;
                        stateChanged = true;
                        break;
                    }
                }
            }
        }

        // Apply state animation
        if (stateChanged)
        {
            for (const auto& node : graphData.Nodes)
            {
                if (node.Name == anim.CurrentStateName)
                {
                    if (anim.AnimationPath != node.AnimationPath)
                    {
                        anim.AnimationPath = node.AnimationPath;
                        anim.IsLooping = node.IsLooping;
                        AnimationSystems::Play(anim, 0, node.IsLooping);
                    }
                    break;
                }
            }
        }
    }
}

} // namespace CHEngine
