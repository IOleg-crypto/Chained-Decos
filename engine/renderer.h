#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include <raylib.h>
#include <string>

namespace CH
{
class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void DrawGrid(int slices, float spacing);
    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawModel(const std::string &path, const Matrix &transform, Color tint = WHITE);
    static void DrawModel(const std::string &path, const Matrix &transform,
                          const struct MaterialComponent &material);

    static void DrawScene(class Scene *scene);

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

        Color lightColor = WHITE;
        Vector3 lightDir = {0.0f, -1.0f, 0.0f};
        float ambient = 0.2f;

        Shader skyboxShader;
        Model skyboxCube;
        int skyboxVflippedLoc;
        int skyboxDoGammaLoc;
        int skyboxFragGammaLoc;
        int skyboxExposureLoc;
        int skyboxBrightnessLoc;
        int skyboxContrastLoc;
    };

    static LightState s_LightState;
    static Camera3D s_CurrentCamera;
    Renderer() = default;
};
} // namespace CH

#endif // CH_RENDERER_H
