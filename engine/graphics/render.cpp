#include "render.h"
#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/components.h"
#include "rlgl.h"
#include <algorithm>

namespace CHEngine
{
    RendererState Render::s_State;

    void Render::Init()
    {
        CH_CORE_INFO("Initializing Render System...");
        
        // Initialize underlying API context if needed
        // For Raylib/OpenGL, we ensure initial state is clean
        s_State = RendererState();
        
        InitSkybox();
        
        CH_CORE_INFO("Render System Initialized.");
    }

    void Render::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");
        // Cleanup resources
    }

    void Render::BeginScene(const Camera3D& camera)
    {
        // Preparation for scene rendering (e.g. updating internal buffers)
        BeginMode3D(camera);
    }

    void Render::EndScene()
    {
        EndMode3D();
    }

    void Render::DrawScene(Scene* scene, const Camera3D& camera, const DebugRenderFlags* debugFlags)
    {
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        // 1. Environmental setup
        ApplyEnvironment(scene->GetEnvironment());
        
        // 2. Clear buffers (usually handled by Application/Viewport, but can be done here)
        
        // 3. Scene rendering flow
        DrawSkybox(scene->GetEnvironment().Skybox, camera);
        
        BeginScene(camera);
        {
            RenderModels(scene);
            
            if (debugFlags)
                RenderDebug(scene, debugFlags);
                
            RenderEditorIcons(scene, camera);
        }
        EndScene();
    }

    void Render::Clear(Color color)
    {
        ClearBackground(color);
    }

    void Render::SetViewport(int x, int y, int width, int height)
    {
        // Raylib/OpenGL Viewport
        rlViewport(x, y, width, height);
    }

    void Render::DrawModel(const std::string& path, const Matrix& transform, 
                         const std::vector<MaterialSlot>& overrides, 
                         int animIndex, int frame)
    {
        auto modelAsset = AssetManager::Get<ModelAsset>(path);
        if (!modelAsset) return;

        // Implementation of model drawing with overrides
        // ... (Transplanted logic from DrawCommand)
        DrawMesh(modelAsset->GetModel().meshes[0], modelAsset->GetModel().materials[0], transform);
    }

    void Render::DrawLine(Vector3 start, Vector3 end, Color color)
    {
        DrawLine3D(start, end, color);
    }

    void Render::DrawGrid(int slices, float spacing)
    {
        DrawGrid(slices, spacing); // Raylib built-in
    }

    void Render::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
    {
        // Implementation transplanted from DrawCommand
        // ...
    }

    void Render::DrawUI(Scene* scene)
    {
        // Future home for consolidated UI rendering logic
    }

    RendererState& Render::GetState()
    {
        return s_State;
    }

    void Render::SetDirectionalLight(Vector3 direction, Color color)
    {
        s_State.DirLightDirection = direction;
        s_State.DirLightColor = color;
    }

    void Render::SetAmbientLight(float intensity)
    {
        s_State.AmbientIntensity = intensity;
    }

    void Render::ApplyEnvironment(const EnvironmentSettings& settings)
    {
        SetAmbientLight(settings.AmbientIntensity);
        SetDirectionalLight(settings.LightDirection, settings.LightColor);
        // ... apply fog, etc.
    }

    // --- Private Internal Helpers (Merging logic from ScenePipeline) ---

    void Render::RenderModels(Scene* scene)
    {
        auto view = scene->GetRegistry().view<TransformComponent, ModelComponent>();
        for (auto entity : view)
        {
            auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);
            DrawModel(model.ModelPath, transform.GetTransform());
        }
    }

    void Render::RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags)
    {
        // Debug visualization (colliders, etc)
    }

    void Render::RenderEditorIcons(Scene* scene, const Camera3D& camera)
    {
        // Billboard-style icons for lights/cameras in editor
    }

    void Render::InitSkybox()
    {
        // Setup internal skybox resources
    }

} // namespace CHEngine
