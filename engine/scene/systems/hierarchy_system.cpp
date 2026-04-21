#include "hierarchy_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/core/profiler.h"
#include <vector>

namespace CHEngine
{
void HierarchySystem::Update(Scene* scene)
{
    CH_PROFILE_FUNCTION();
    auto& reg = scene->GetRegistry();
    auto view = reg.view<TransformComponent>();

   

    std::vector<UpdateTask> stack;
    stack.reserve(reg.storage<entt::entity>().size());

    // 1. Find all root entities and push to stack
    for (auto entity : view)
    {
        bool isRoot = true;
        if (reg.all_of<HierarchyComponent>(entity))
        {
            auto& hc = reg.get<HierarchyComponent>(entity);
            if (hc.Parent != entt::null && reg.valid(hc.Parent) && reg.all_of<TransformComponent>(hc.Parent))
            {
                isRoot = false;
            }
        }

        if (isRoot)
        {
            stack.push_back({entity, glm::mat4(1.0f), false});
        }
    }

    // 2. Iterative DFS update with dirty flag propagation
    while (!stack.empty())
    {
        UpdateTask task = stack.back();
        stack.pop_back();

        auto& tc = view.get<TransformComponent>(task.Entity);
        
        // A node needs update if it is explicitly dirty OR its parent's world transform changed
        bool needsUpdate = task.ParentChanged || tc.IsDirty;
        
        if (needsUpdate)
        {
            tc.WorldTransform = task.ParentTransform * tc.GetTransform();
            tc.IsDirty = false;
        }

        if (reg.all_of<HierarchyComponent>(task.Entity))
        {
            auto& hc = reg.get<HierarchyComponent>(task.Entity);
            for (auto child : hc.Children)
            {
                if (reg.valid(child) && reg.all_of<TransformComponent>(child))
                {
                    stack.push_back({child, tc.WorldTransform, needsUpdate});
                }
            }
        }
    }
}
} // namespace CHEngine
