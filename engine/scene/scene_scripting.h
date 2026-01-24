#ifndef CH_SCENE_SCRIPTING_H
#define CH_SCENE_SCRIPTING_H

#include "engine/core/base.h"
#include <entt/entt.hpp>

namespace CHEngine
{
class Scene;
class Event;

class SceneScripting
{
public:
    SceneScripting(Scene *scene) : m_Scene(scene)
    {
    }

    void OnUpdate(float deltaTime);
    void OnEvent(Event &e);

private:
    Scene *m_Scene;
};
} // namespace CHEngine

#endif // CH_SCENE_SCRIPTING_H
