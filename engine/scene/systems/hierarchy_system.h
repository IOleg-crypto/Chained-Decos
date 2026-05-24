#ifndef CH_HIERARCHY_SYSTEM_H
#define CH_HIERARCHY_SYSTEM_H

#include "entt/entt.hpp"
#include <glm/glm.hpp>

namespace CHEngine
{
class Scene;

class HierarchySystem
{
private:
    struct UpdateTask
    {
        entt::entity Entity;
        glm::mat4 ParentTransform;
        bool ParentChanged;
    };

public:
    void Update(Scene* scene);
};
} // namespace CHEngine

#endif // CH_HIERARCHY_SYSTEM_H
