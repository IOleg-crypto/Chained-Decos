#ifndef CH_SCENE_SCRIPTING_H
#define CH_SCENE_SCRIPTING_H

#include "engine/core/events.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class SceneScripting
{
public:
    static void Update(Scene *scene, Timestep deltaTime);
    static void DispatchEvent(Scene *scene, Event &e);
    static void RenderUI(Scene *scene);
};
} // namespace CHEngine

#endif // CH_SCENE_SCRIPTING_H
