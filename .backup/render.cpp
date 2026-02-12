#include "engine/graphics/render.h"
#include "engine/graphics/render_command.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/graphics/renderer2d.h"
#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/core/profiler.h"
#include "imgui.h"
#include "rlgl.h"
#include <algorithm>
#include <vector>
#include <filesystem>

namespace CHEngine
{
    RenderState Render::s_State;

    void Render::Initialize()
    {
        CH_CORE_INFO("Initializing Render System...");
        s_State = RenderState();

        RenderCommand::Initialize();
        Renderer2D::Initialize();

        auto project = Project::GetActive();
        auto assetManager = project ? project->GetAssetManager() : nullptr;

        // Shaders loading - either through AssetManager or fallback
        if (assetManager)
        {
            s_State.LightingShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/lighting.chshader");
            s_State.SkyboxShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/skybox.chshader");

            // Icons
            auto lightIcon = assetManager->Get<TextureAsset>("engine/resources/icons/light_bulb.png");
            if (lightIcon) s_State.LightIcon = lightIcon->GetTexture();

            auto spawnIcon = assetManager->Get<TextureAsset>("engine/resources/icons/leaf_icon.png");
            if (spawnIcon) s_State.SpawnIcon = spawnIcon->GetTexture();

            auto cameraIcon = assetManager->Get<TextureAsset>("engine/resources/icons/camera_icon.png");
            if (cameraIcon) s_State.CameraIcon = cameraIcon->GetTexture();
        }
        else
        {
            CH_CORE_WARN("Render::Initialize: No active project, engine shaders will be loaded lazily.");
        }

        InitializeSkybox();
        CH_CORE_INFO("Render System Initialized (Core).");
    }

    void Render::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");

        if (s_State.LightIcon.id > 0) ::UnloadTexture(s_State.LightIcon);
        if (s_State.SpawnIcon.id > 0) ::UnloadTexture(s_State.SpawnIcon);
        if (s_State.CameraIcon.id > 0) ::UnloadTexture(s_State.CameraIcon);

        Renderer2D::Shutdown();
        RenderCommand::Shutdown();
    }

    void Render::BeginScene(const Camera3D& camera)
    {
        s_State.CurrentCameraPosition = camera.position;
        BeginMode3D(camera);
    }

    void Render::EndScene()
    {
        EndMode3D();
    }

    void Render::Clear(Color color)
    {
        RenderCommand::Clear(color);
    }

    void Render::SetViewport(int x, int y, int width, int height)
    {
        RenderCommand::SetViewport(x, y, width, height);
    }

    void Render::DrawModel(const std::string& path, const Matrix& transform,
        const std::vector<MaterialSlot>& materialSlotOverrides,
        int animationIndex, int frameIndex)
    {
        auto project = Project::GetActive();
        if (!project)
        {
            CH_CORE_WARN("Render::DrawModel - No active project!");
            return;
        }

        auto modelAsset = project->GetAssetManager()->Get<ModelAsset>(path);
        if (!modelAsset)
        {
            CH_CORE_WARN("Render::DrawModel - Failed to get asset by path: '{}'", path);
            return;
        }

        DrawModel(modelAsset, transform, materialSlotOverrides, animationIndex, frameIndex);
    }

    void Render::DrawModel(std::shared_ptr<ModelAsset> modelAsset, const Matrix& transform,
        const std::vector<MaterialSlot>& materialSlotOverrides,
        int animationIndex, int frameIndex)
    {
        if (modelAsset && modelAsset->GetState() == AssetState::Ready)
        {
            Model& model = modelAsset->GetModel();
            CH_CORE_TRACE("Render::DrawModel - Rendering: {} ({} meshes)", modelAsset->GetPath(), model.meshCount);

            // Apply animation if needed
            if (animationIndex >= 0)
            {
                int animationCount = 0;
                auto* animations = modelAsset->GetAnimations(&animationCount);
                if (animations && animationIndex < animationCount)
                {
                    UpdateModelAnimation(model, animations[animationIndex], frameIndex);
                }
            }

            for (int i = 0; i < model.meshCount; i++)
            {
                Material& material = model.materials[model.meshMaterial[i]];

                if (material.shader.id == 0 && s_State.LightingShader)
                    material.shader = s_State.LightingShader->GetShader();

                if (s_State.LightingShader && material.shader.id == s_State.LightingShader->GetShader().id)
                {
                    s_State.LightingShader->SetVec3("lightDir", s_State.CurrentLightDirection);
                    s_State.LightingShader->SetColor("lightColor", s_State.CurrentLightColor);
                    s_State.LightingShader->SetFloat("ambient", s_State.CurrentAmbientIntensity);

                    // Fog uniforms
                    s_State.LightingShader->SetInt("fogEnabled", s_State.FogEnabled ? 1 : 0);
                    if (s_State.FogEnabled)
                    {
                        s_State.LightingShader->SetColor("fogColor", s_State.FogColor);
                        s_State.LightingShader->SetFloat("fogDensity", s_State.FogDensity);
                        s_State.LightingShader->SetFloat("fogStart", s_State.FogStart);
                        s_State.LightingShader->SetFloat("fogEnd", s_State.FogEnd);
                    }

                    s_State.LightingShader->SetVec3("viewPos", s_State.CurrentCameraPosition);
                    s_State.LightingShader->SetFloat("uTime", s_State.Time);
                }

                ProfilerStats stats;
                stats.DrawCalls++;
                stats.MeshCount++;
                stats.PolyCount += model.meshes[i].triangleCount;
                Profiler::UpdateStats(stats);

                // Combine model base transform with entity transform
                Matrix meshTransform = MatrixMultiply(model.transform, transform);

                DrawMesh(model.meshes[i], material, meshTransform);
            }
        }
        else
        {
            static int logCount = 0;
            if (logCount++ < 10) // Limit spam
            {
                std::string path = modelAsset ? modelAsset->GetPath() : "NULL";
                int state = modelAsset ? (int)modelAsset->GetState() : -1;
                CH_CORE_WARN("Render::DrawModel - Asset not ready: {} (state: {})", path, state);
            }
        }
    }

    void Render::DrawLine(Vector3 startPosition, Vector3 endPosition, Color color)
    {
        RenderCommand::DrawLine(startPosition, endPosition, color);
    }

    void Render::DrawGrid(int sliceCount, float spacing)
    {
        RenderCommand::DrawGrid(sliceCount, spacing);
    }

    void Render::DrawCubeWires(const Matrix& transform, Vector3 size, Color color)
    {
        // Draw oriented wireframe box using transform matrix
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));

        ::DrawCubeWires({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);

        rlPopMatrix();
    }

    void Render::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
    {
        if (!s_State.SkyboxShader || skybox.TexturePath.empty()) return;

        auto project = Project::GetActive();
        if (!project) return;

        auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(skybox.TexturePath);
        if (!textureAsset)
        {
            CH_CORE_WARN("Render::DrawSkybox: Failed to get texture asset: {}", skybox.TexturePath);
            return;
        }

        if (textureAsset->GetState() != AssetState::Ready) return;

        RenderCommand::DisableBackfaceCulling();
        RenderCommand::DisableDepthMask();

        Material material = LoadMaterialDefault();
        material.shader = s_State.SkyboxShader->GetShader();
        Texture2D skyTexture = textureAsset->GetTexture();
        ::SetTextureFilter(skyTexture, TEXTURE_FILTER_BILINEAR);
        ::SetTextureWrap(skyTexture, TEXTURE_WRAP_CLAMP);
        material.maps[MATERIAL_MAP_ALBEDO].texture = skyTexture;

        s_State.SkyboxShader->SetFloat("exposure", skybox.Exposure);
        s_State.SkyboxShader->SetFloat("brightness", skybox.Brightness);
        s_State.SkyboxShader->SetFloat("contrast", skybox.Contrast);
        s_State.SkyboxShader->SetInt("vflipped", 0);
        s_State.SkyboxShader->SetInt("doGamma", 0);
        s_State.SkyboxShader->SetFloat("fragGamma", 2.2f);

        // Fog uniforms
        s_State.SkyboxShader->SetInt("fogEnabled", s_State.FogEnabled ? 1 : 0);
        if (s_State.FogEnabled)
        {
            s_State.SkyboxShader->SetColor("fogColor", s_State.FogColor);
            s_State.SkyboxShader->SetFloat("fogDensity", s_State.FogDensity);
            s_State.SkyboxShader->SetFloat("fogStart", s_State.FogStart);
            s_State.SkyboxShader->SetFloat("fogEnd", s_State.FogEnd);
        }

        s_State.SkyboxShader->SetFloat("uTime", s_State.Time);

        DrawMesh(s_State.SkyboxCube.meshes[0], material, MatrixTranslate(camera.position.x, camera.position.y, camera.position.z));

        RenderCommand::EnableBackfaceCulling();
        RenderCommand::EnableDepthMask();
    }

    void Render::DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color color)
    {
        if (texture.id == 0) return;
        ::DrawBillboard(camera, texture, position, size, color);
    }

    RenderState& Render::GetState()
    {
        return s_State;
    }

    void Render::SetDirectionalLight(Vector3 direction, Color color)
    {
        s_State.CurrentLightDirection = direction;
        s_State.CurrentLightColor = color;
    }

    void Render::SetAmbientLight(float intensity)
    {
        s_State.CurrentAmbientIntensity = intensity;
    }

    void Render::ApplyEnvironment(const EnvironmentSettings& settings)
    {
        SetAmbientLight(settings.AmbientIntensity);
        SetDirectionalLight(settings.LightDirection, settings.LightColor);

        // Sync Fog to State
        s_State.FogEnabled = settings.Fog.Enabled;
        s_State.FogColor = settings.Fog.FogColor;
        s_State.FogDensity = settings.Fog.Density;
        s_State.FogStart = settings.Fog.Start;
        s_State.FogEnd = settings.Fog.End;
    }

    void Render::UpdateTime(float time)
    {
        s_State.Time = time;
    }

    void Render::InitializeSkybox()
    {
        Mesh cube = GenMeshCube(100.0f, 100.0f, 100.0f);
        s_State.SkyboxCube = LoadModelFromMesh(cube);
    }
}
