#ifndef CH_SCENE_TRACE_H
#define CH_SCENE_TRACE_H

#include "physics.h"
#include "raylib.h"

namespace CHEngine
{
class Scene;

class Physics;

class SceneTrace
{
public:
    SceneTrace(Physics* physics) : m_Physics(physics) {}

    RaycastResult Raycast(Scene *scene, Ray ray);

private:
    Physics* m_Physics;
};
} // namespace CHEngine

#endif // CH_SCENE_TRACE_H
