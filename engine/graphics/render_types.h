#ifndef CH_RENDER_TYPES_H
#define CH_RENDER_TYPES_H

#include "engine/core/base.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{

class ShaderAsset;
class ModelAsset;
class TextureAsset;
class EnvironmentAsset;

struct DebugRenderFlags
{
    bool DrawColliders = false;
    bool DrawLights = false;
    bool DrawSpawnZones = false;
    bool DrawGrid = true;
    bool DrawAxes = true;
    bool DrawSkeleton = false;
    bool DrawBoundingBoxes = false;
    bool DrawIcons = true;
    bool DrawNavMesh = false;
};

struct RendererState
{
    // Shaders
    std::shared_ptr<ShaderAsset> LightingShader;
    std::shared_ptr<ShaderAsset> SkyboxShader;
    std::shared_ptr<ShaderAsset> PanoramaShader;

    // Shared Resources
    Model SkyboxCube = { 0 };

    // Scene Data
    Color CurrentLightColor = WHITE;
    Vector3 CurrentLightDir = {0.0f, -1.0f, 0.0f};
    float CurrentAmbientIntensity = 0.2f;
    Camera3D ActiveCamera = { 0 };
};

} // namespace CHEngine

#endif // CH_RENDER_TYPES_H
