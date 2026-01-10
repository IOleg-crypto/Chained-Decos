#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include <raylib.h>
#include <string>

namespace CH
{
struct DebugRenderFlags
{
    bool DrawColliders = false;
    bool DrawLights = false;
    bool DrawSpawnZones = false;

    bool IsAnyEnabled() const
    {
        return DrawColliders || DrawLights || DrawSpawnZones;
    }
};

class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void DrawGrid(int slices, float spacing);
    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawModel(const std::string &path, const Matrix &transform, Color tint = WHITE,
                          Vector3 scale = {1.0f, 1.0f, 1.0f});
    static void DrawModel(const std::string &path, const Matrix &transform,
                          const struct MaterialComponent &material,
                          Vector3 scale = {1.0f, 1.0f, 1.0f});

    static void DrawScene(class Scene *scene, const DebugRenderFlags *debugFlags = nullptr);

    // Lighting
    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);

    static void DrawSkybox(const struct SkyboxComponent &skybox, const Camera3D &camera);

    // For 2D / UI
    static void BeginUI();
    static void EndUI();

private:
    struct LightState
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

        Color lightColor = WHITE;
        Vector3 lightDir = {0.0f, -1.0f, 0.0f};
        float ambient = 0.2f;

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
    };

    static LightState s_LightState;
    static Camera3D s_CurrentCamera;

    // Debug rendering helpers
    static void DrawColliderDebug(Scene *scene);
    static void DrawLightDebug(Scene *scene);
    static void DrawSpawnZoneDebug(Scene *scene);

    Renderer() = default;
};
} // namespace CH

#endif // CH_RENDERER_H
