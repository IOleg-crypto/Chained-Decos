#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#include "scene/ecs/Entity.h"
#include <entt/entt.hpp>


namespace CHEngine
{

class ScriptSystem
{
public:
    static void Update(entt::registry &registry, float deltaTime);
    static void OnStart(entt::registry &registry);
};

} // namespace CHEngine

#endif // SCRIPT_SYSTEM_H
