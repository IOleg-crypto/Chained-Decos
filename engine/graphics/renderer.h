#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/base.h"
#include "engine/core/assert.h"
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

    struct RendererData
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
        Timestep Time = 0.0f;
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

    class Renderer
    {
    public:
        static void Init();
        static void Shutdown();

        Renderer();
        ~Renderer();

        void BeginScene(const Camera3D& camera);
        void EndScene();

        void Clear(Color color);
        void SetViewport(int x, int y, int width, int height);

        void DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform = MatrixIdentity(),
            const std::vector<MaterialSlot>& materialSlotOverrides = {},
            int animationIndex = 0, int frameIndex = 0);
        void DrawLine(Vector3 startPosition, Vector3 endPosition, Color color);
        void DrawGrid(int sliceCount, float spacing);
        void DrawSkybox(const SkyboxSettings& settings, const Camera3D& camera);
        void DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color tint);
        void DrawCubeWires(const Matrix& transform, Vector3 size, Color color);

        void SetDirectionalLight(Vector3 direction, Color color);
        void SetAmbientLight(float intensity);
        void ApplyEnvironment(const EnvironmentSettings& settings);
        void UpdateTime(Timestep time);

        inline RendererData& GetData() { return *m_Data; }
        inline const RendererData& GetData() const { return *m_Data; }
        
        static Renderer& Get() 
        { 
            CH_CORE_ASSERT(s_Instance, "Renderer instance is null!");
            return *s_Instance; 
        }

    private:
        void EnsureShadersLoaded();
        void InitializeSkybox();

    private:
        static Renderer* s_Instance;
        std::unique_ptr<RendererData> m_Data;
    };
}

#endif // CH_RENDERER_H
