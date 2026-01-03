#ifndef CD_SCENE_ECS_SYSTEMS_RENDER_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_RENDER_SYSTEM_H

#include <entt/entt.hpp>

namespace CHEngine
{

/**
 * @brief System for rendering models from RenderComponent in Pure ECS.
 */
class RenderSystem
{
public:
    static void Render(entt::registry &registry);
};

} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_RENDER_SYSTEM_H
