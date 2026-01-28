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
    bool DrawSkeleton = false;
    bool DrawBoundingBoxes = false;
    bool DrawIcons = true;
    bool DrawNavMesh = false;

    bool IsAnyEnabled() const
    {
        return DrawColliders || DrawLights || DrawSpawnZones || DrawSkeleton || DrawBoundingBoxes ||
               DrawIcons || DrawNavMesh;
    }
};

struct ShaderLightLocs
{
    int position;
    int color;
    int radius;
    int radiance;
    int falloff;
    int enabled;
};

struct RendererState
{
    // Shaders
    std::shared_ptr<ShaderAsset> LightingShader;
    std::shared_ptr<ShaderAsset> SkyboxShader;
    std::shared_ptr<ShaderAsset> PanoramaShader;

    // Shared Resources
    Model SkyboxCube;

    // Uniform Locations
    int LightDirLoc;
    int LightColorLoc;
    int AmbientLoc;
    ShaderLightLocs LightLocs[8];

    int SkyboxVflippedLoc;
    int SkyboxDoGammaLoc;
    int SkyboxFragGammaLoc;
    int SkyboxExposureLoc;
    int SkyboxBrightnessLoc;
    int SkyboxContrastLoc;

    int PanoDoGammaLoc;
    int PanoFragGammaLoc;
    int PanoExposureLoc;
    int PanoBrightnessLoc;
    int PanoContrastLoc;

    // Scene Data
    Color CurrentLightColor = WHITE;
    Vector3 CurrentLightDir = {0.0f, -1.0f, 0.0f};
    float CurrentAmbientIntensity = 0.2f;
    Camera3D ActiveCamera;
};

} // namespace CHEngine

#endif // CH_RENDER_TYPES_H
