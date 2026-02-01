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

    bool IsAnyEnabled() const
    {
        return DrawColliders || DrawLights || DrawSpawnZones || DrawSkeleton || DrawBoundingBoxes ||
               DrawIcons || DrawNavMesh || DrawGrid;
    }
};

struct ShaderLightLocs
{
    int position = -1;
    int color = -1;
    int radius = -1;
    int radiance = -1;
    int falloff = -1;
    int enabled = -1;
};

struct RendererState
{
    // Shaders
    std::shared_ptr<ShaderAsset> LightingShader;
    std::shared_ptr<ShaderAsset> SkyboxShader;
    std::shared_ptr<ShaderAsset> PanoramaShader;

    // Shared Resources
    Model SkyboxCube = { 0 };

    // Uniform Locations
    int LightDirLoc = -1;
    int LightColorLoc = -1;
    int AmbientLoc = -1;
    ShaderLightLocs LightLocs[8];

    int SkyboxVflippedLoc = -1;
    int SkyboxDoGammaLoc = -1;
    int SkyboxFragGammaLoc = -1;
    int SkyboxExposureLoc = -1;
    int SkyboxBrightnessLoc = -1;
    int SkyboxContrastLoc = -1;

    int PanoDoGammaLoc = -1;
    int PanoFragGammaLoc = -1;
    int PanoExposureLoc = -1;
    int PanoBrightnessLoc = -1;
    int PanoContrastLoc = -1;

    // Scene Data
    Color CurrentLightColor = WHITE;
    Vector3 CurrentLightDir = {0.0f, -1.0f, 0.0f};
    float CurrentAmbientIntensity = 0.2f;
    Camera3D ActiveCamera = { 0 };
};

} // namespace CHEngine

#endif // CH_RENDER_TYPES_H
