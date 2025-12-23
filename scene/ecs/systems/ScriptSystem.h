#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#include "scene/ecs/Entity.h"

namespace CHEngine
{

class ScriptSystem
{
public:
    static void Update(float deltaTime);
    static void OnStart();
};

} // namespace CHEngine

#endif // SCRIPT_SYSTEM_H
