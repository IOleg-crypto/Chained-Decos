#ifndef CH_RAYCAST_RESULT_H
#define CH_RAYCAST_RESULT_H

#include "raylib.h"
#include "entt/entt.hpp"

namespace CHEngine
{
struct RaycastResult
{
    bool Hit = false;
    float Distance = 0.0f;
    Vector3 Position = {0.0f, 0.0f, 0.0f};
    Vector3 Normal = {0.0f, 0.0f, 0.0f};
    entt::entity Entity = entt::null;
    int MeshIndex = -1;
};
} // namespace CHEngine

#endif // CH_RAYCAST_RESULT_H
