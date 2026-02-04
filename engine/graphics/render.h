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
    class Scene;
    class ShaderAsset;

    struct RenderState
    {
        std::shared_ptr<ShaderAsset> LightingShader;
        std::shared_ptr<ShaderAsset> SkyboxShader;
        Model SkyboxCube;

        Vector3 CurrentLightDir = {-0.5f, -1.0f, -0.5f};
        Color CurrentLightColor = WHITE;
        float CurrentAmbientIntensity = 0.5f;
    };

    struct DebugRenderFlags
    {
        bool DrawColliders = false;
        bool DrawHierarchy = false;
        bool DrawAABB = false;
        bool DrawGrid = true;
        bool DrawSelection = true;
        bool DrawLights = true;
        bool DrawSpawnZones = true;
    };

    class Render
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const Camera3D& camera);
        static void EndScene();

        // High-level entry points (delegates to SceneRenderer/UIRenderer)
        static void DrawScene(Scene* scene, const Camera3D& camera, Timestep ts, const DebugRenderFlags* debugFlags = nullptr);
        static void DrawUI(Scene* scene, const ImVec2& refPos, const ImVec2& refSize, bool editMode = false);

        static void Clear(Color color);
        static void SetViewport(int x, int y, int width, int height);

        static void DrawModel(const std::string& path, const Matrix& transform = MatrixIdentity(), 
                             const std::vector<MaterialSlot>& overrides = {}, 
                             int animIndex = 0, int frame = 0);
        
        static void DrawLine(Vector3 start, Vector3 end, Color color);
        static void DrawGrid(int slices, float spacing);
        static void DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera);

        static void SetDirectionalLight(Vector3 direction, Color color);
        static void SetAmbientLight(float intensity);
        static void ApplyEnvironment(const EnvironmentSettings& settings);

        static RenderState& GetState();

        // Submit a command to the render queue (immediate execution for now)
        template<typename F>
        static void Submit(F&& func)
        {
            func();
        }

    private:
        static void InitSkybox();

    private:
        static RenderState s_State;
    };
}

#endif // CH_RENDER_H
