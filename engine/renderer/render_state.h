#ifndef CH_RENDER_STATE_H
#define CH_RENDER_STATE_H

#include "engine/core/base.h"
#include "engine/renderer/environment.h"
#include "engine/renderer/model_asset.h"
#include "engine/scene/components.h"
#include <raylib.h>
#include <vector>

namespace CHEngine
{

enum class RenderPacketType
{
    Mesh,
    PointLight,
    Skybox,
    DebugBox,
    DebugSphere,
    DebugLine,
    DebugMeshWires
};

struct RenderPacket
{
    RenderPacketType Type;
    Matrix Transform;
    Color Tint = WHITE;

    // Mesh specific
    Ref<ModelAsset> Model;
    std::vector<MaterialSlot> MaterialSlots;

    // Light / Debug specific
    Vector3 Position;
    float Radius;
    float Radiance;
    Vector3 Size;         // For boxes
    std::string Metadata; // Path for mesh wires, etc.
};

struct RenderState
{
    Camera3D SceneCamera;
    EnvironmentSettings Environment;
    std::vector<RenderPacket> Commands;

    float Alpha = 1.0f; // Interpolation factor

    void Clear()
    {
        Commands.clear();
    }
};

} // namespace CHEngine

#endif // CH_RENDER_STATE_H
