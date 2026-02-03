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
    static RaycastResult Raycast(Scene *scene, Ray ray, Physics* physics);
};
} // namespace CHEngine

#endif // CH_SCENE_TRACE_H
