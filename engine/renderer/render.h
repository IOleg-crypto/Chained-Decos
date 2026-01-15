#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/math_types.h"
#include "engine/renderer/material.h"
#include "engine/scene/scene.h"
#include <string>


namespace CHEngine
{
struct DebugRenderFlags
{
    bool DrawColliders = false;
    bool DrawLights = false;
    bool DrawSpawnZones = false;
    bool DrawGrid = true;

    bool IsAnyEnabled() const
    {
        return DrawColliders || DrawLights || DrawSpawnZones;
    }
};

class Render
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void DrawGrid(int slices, float spacing);
    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawModel(const std::string &path, const Matrix &transform,
                          const MaterialInstance &material, Vector3 scale = {1.0f, 1.0f, 1.0f});
    static void DrawModel(const std::string &path, const Matrix &transform, Color tint = WHITE,
                          Vector3 scale = {1.0f, 1.0f, 1.0f});

    static void DrawScene(Scene *scene,
                          const DebugRenderFlags *debugFlags = nullptr); // To be removed soon

    // Lighting
    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);

    static void DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera);

    // For 2D / UI
    static void BeginUI();
    static void EndUI();

private:
    struct ShaderState
    {
        Shader lightingShader;
        int lightDirLoc;
        int lightColorLoc;
        int ambientLoc;

        struct ShaderLightLocs
        {
            int position;
            int color;
            int radius;
            int radiance;
            int falloff;
            int enabled;
        } lightLocs[8];

        Shader skyboxShader;
        Shader panoramaShader;
        Model skyboxCube;
        int skyboxVflippedLoc;
        int skyboxDoGammaLoc;
        int skyboxFragGammaLoc;
        int skyboxExposureLoc;
        int skyboxBrightnessLoc;
        int skyboxContrastLoc;

        // Panorama shader locations
        int panoDoGammaLoc;
        int panoFragGammaLoc;
        int panoExposureLoc;
        int panoBrightnessLoc;
        int panoContrastLoc;

        // Infinite Grid
        Shader gridShader;
        unsigned int gridVAO = 0;
        unsigned int gridVBO = 0;
        int gridNearLoc;
        int gridFarLoc;
        int gridViewLoc;
        int gridProjLoc;
    };

    struct SceneData
    {
        Color lightColor = WHITE;
        Vector3 lightDir = {0.0f, -1.0f, 0.0f};
        float ambientIntensity = 0.2f;
        Camera3D currentCamera;
    };

    static ShaderState s_Shaders;
    static SceneData s_Scene;

    Render() = default;
};
} // namespace CHEngine

#endif // CH_RENDERER_H
