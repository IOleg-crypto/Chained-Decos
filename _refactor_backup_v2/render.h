#ifndef CH_RENDER_H
#define CH_RENDER_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "engine/graphics/environment.h"
#include "engine/scene/components/mesh_component.h" // For MaterialSlot
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace CHEngine
{
    class ModelAsset;
    class ShaderAsset;

    struct RenderState
    {
        std::shared_ptr<ShaderAsset> LightingShader;
        std::shared_ptr<ShaderAsset> SkyboxShader;
        Model SkyboxCube;

        Vector3 CurrentLightDirection = {-0.5f, -1.0f, -0.5f};
        Color CurrentLightColor = WHITE;
        float CurrentAmbientIntensity = 0.5f;

        // Editor Icons
        Texture2D LightIcon = { 0 };
        Texture2D SpawnIcon = { 0 };
        Texture2D CameraIcon = { 0 };

        // Fog settings for shader
        bool FogEnabled = false;
        Color FogColor = GRAY;
        float FogDensity = 0.01f;
        float FogStart = 10.0f;
        float FogEnd = 100.0f;

        Vector3 CurrentCameraPosition = { 0.0f, 0.0f, 0.0f };
        float Time = 0.0f;
    };

    struct DebugRenderFlags
    {
        bool DrawColliders = false;
        bool DrawHierarchy = false;
        bool DrawAABB = false;
        bool DrawGrid = false;
        bool DrawSelection = true;
        bool DrawLights = true;
        bool DrawSpawnZones = true;
    };

    class Render
    {
    public:
        static void Initialize();
        static void Shutdown();

        static void BeginScene(const Camera3D& camera);
        static void EndScene();

        static void Clear(Color color);
        static void SetViewport(int x, int y, int width, int height);

        static void DrawModel(const std::string& path, const Matrix& transform = MatrixIdentity(),
            const std::vector<MaterialSlot>& materialSlotOverrides = {},
            int animationIndex = 0, int frameIndex = 0);

        static void DrawModel(std::shared_ptr<ModelAsset> modelAsset, const Matrix& transform = MatrixIdentity(),
            const std::vector<MaterialSlot>& materialSlotOverrides = {},
            int animationIndex = 0, int frameIndex = 0);

        static void DrawLine(Vector3 startPosition, Vector3 endPosition, Color color);
        static void DrawGrid(int sliceCount, float spacing);
        static void DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera);
        static void DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color color = WHITE);
        static void DrawCubeWires(const Matrix& transform, Vector3 size, Color color);

        static void SetDirectionalLight(Vector3 direction, Color color);
        static void SetAmbientLight(float intensity);
        static void ApplyEnvironment(const EnvironmentSettings& settings);
        static void UpdateTime(float time);

        static RenderState& GetState();

    private:
        static void InitializeSkybox();

    private:
        static RenderState s_State;
    };
}

#endif // CH_RENDER_H
