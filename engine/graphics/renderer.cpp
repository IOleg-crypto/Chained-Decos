#include "engine/graphics/renderer.h"
#include "engine/graphics/render_command.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/graphics/renderer2d.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/core/profiler.h"
#include "imgui.h"
#include "rlgl.h"
#include <algorithm>
#include <vector>
#include <filesystem>

namespace CHEngine
{
    Renderer* Renderer::s_Instance = nullptr;

    void Renderer::Init()
    {
        CH_CORE_ASSERT(!s_Instance, "Renderer already initialized!");
        s_Instance = new Renderer();
        
        CH_CORE_INFO("Initializing Render System...");

        RenderCommand::Initialize();
        Renderer2D::Init();
        UIRenderer::Init();

        auto project = Project::GetActive();
        auto assetManager = project ? project->GetAssetManager() : nullptr;

        // Shaders loading - either through AssetManager or fallback
        if (assetManager)
        {
            s_Instance->m_Data->LightingShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/lighting.chshader");
            s_Instance->m_Data->SkyboxShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/skybox.chshader");

            // Icons
            auto lightIcon = assetManager->Get<TextureAsset>("engine/resources/icons/light_bulb.png");
            if (lightIcon) s_Instance->m_Data->LightIcon = lightIcon->GetTexture();

            auto spawnIcon = assetManager->Get<TextureAsset>("engine/resources/icons/leaf_icon.png");
            if (spawnIcon) s_Instance->m_Data->SpawnIcon = spawnIcon->GetTexture();

            auto cameraIcon = assetManager->Get<TextureAsset>("engine/resources/icons/camera_icon.png");
            if (cameraIcon) s_Instance->m_Data->CameraIcon = cameraIcon->GetTexture();
        }
        else
        {
            CH_CORE_WARN("Renderer::Init: No active project, engine shaders will be loaded lazily.");
        }

        s_Instance->InitializeSkybox();
        CH_CORE_INFO("Render System Initialized (Core).");
    }

    void Renderer::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");
        delete s_Instance;
        s_Instance = nullptr;
    }

    Renderer::Renderer()
    {
        m_Data = std::make_unique<RendererData>();
    }

    Renderer::~Renderer()
    {
        if (m_Data->LightIcon.id > 0) ::UnloadTexture(m_Data->LightIcon);
        if (m_Data->SpawnIcon.id > 0) ::UnloadTexture(m_Data->SpawnIcon);
        if (m_Data->CameraIcon.id > 0) ::UnloadTexture(m_Data->CameraIcon);

        Renderer2D::Shutdown();
        UIRenderer::Shutdown();
        RenderCommand::Shutdown();
    }

    void Renderer::BeginScene(const Camera3D& camera)
    {
        EnsureShadersLoaded();
        m_Data->CurrentCameraPosition = camera.position;
        BeginMode3D(camera);
    }

    void Renderer::EndScene()
    {
        EndMode3D();
    }

    void Renderer::Clear(Color color)
    {
        RenderCommand::Clear(color);
    }

    void Renderer::SetViewport(int x, int y, int width, int height)
    {
        RenderCommand::SetViewport(x, y, width, height);
    }

    void Renderer::DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform,
        const std::vector<MaterialSlot>& materialSlotOverrides,
        int animationIndex, int frameIndex)
    {
        CH_CORE_ASSERT(s_Instance, "Renderer not initialized!");
        if (modelAsset && modelAsset->GetState() == AssetState::Ready)
        {
            Model& model = modelAsset->GetModel();
            CH_CORE_TRACE("Renderer::DrawModel - Rendering: {} ({} meshes)", modelAsset->GetPath(), model.meshCount);

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

                if (material.shader.id == 0 && m_Data->LightingShader)
                    material.shader = m_Data->LightingShader->GetShader();

                if (m_Data->LightingShader && material.shader.id == m_Data->LightingShader->GetShader().id)
                {
                    m_Data->LightingShader->SetVec3("lightDir", m_Data->CurrentLightDirection);
                    m_Data->LightingShader->SetColor("lightColor", m_Data->CurrentLightColor);
                    m_Data->LightingShader->SetFloat("ambient", m_Data->CurrentAmbientIntensity);

                    // Fog uniforms
                    m_Data->LightingShader->SetInt("fogEnabled", m_Data->FogEnabled ? 1 : 0);
                    if (m_Data->FogEnabled)
                    {
                        m_Data->LightingShader->SetColor("fogColor", m_Data->FogColor);
                        m_Data->LightingShader->SetFloat("fogDensity", m_Data->FogDensity);
                        m_Data->LightingShader->SetFloat("fogStart", m_Data->FogStart);
                        m_Data->LightingShader->SetFloat("fogEnd", m_Data->FogEnd);
                    }

                    m_Data->LightingShader->SetVec3("viewPos", m_Data->CurrentCameraPosition);
                    m_Data->LightingShader->SetFloat("uTime", m_Data->Time);
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
            CH_CORE_WARN("Renderer::DrawModel - Model asset not ready!");
        }
    }

    void Renderer::DrawLine(Vector3 startPosition, Vector3 endPosition, Color color)
    {
        RenderCommand::DrawLine(startPosition, endPosition, color);
    }

    void Renderer::DrawGrid(int sliceCount, float spacing)
    {
        RenderCommand::DrawGrid(sliceCount, spacing);
    }

    void Renderer::DrawCubeWires(const Matrix& transform, Vector3 size, Color color)
    {
        // Draw oriented wireframe box using transform matrix
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));

        ::DrawCubeWires({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);

        rlPopMatrix();
    }

    void Renderer::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
    {
        if (!m_Data->SkyboxShader || skybox.TexturePath.empty()) return;

        auto project = Project::GetActive();
        if (!project) return;

        auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(skybox.TexturePath);
        
        if (!textureAsset)
        {
            static std::string lastFailedPath = "";
            if (lastFailedPath != skybox.TexturePath)
            {
                CH_CORE_WARN("Renderer::DrawSkybox: Failed to find texture asset: {}", skybox.TexturePath);
                lastFailedPath = skybox.TexturePath;
            }
            return;
        }

        if (textureAsset->GetState() != AssetState::Ready)
        {
            // If it's pending, we might need to upload it if we are on main thread
            textureAsset->UploadToGPU();
            if (!textureAsset->IsReady()) return;
        }
        
        if (m_Data->SkyboxShader->GetShader().id == 0) return;

        RenderCommand::DisableBackfaceCulling();
        RenderCommand::DisableDepthMask();

        Material material = LoadMaterialDefault();
        material.shader = m_Data->SkyboxShader->GetShader();
        Texture2D skyTexture = textureAsset->GetTexture();
        
        if (skyTexture.id == 0)
        {
            CH_CORE_ERROR("Renderer::DrawSkybox: Texture asset ready but Raylib texture ID is 0! Path: {}", skybox.TexturePath);
            RenderCommand::EnableBackfaceCulling();
            RenderCommand::EnableDepthMask();
            return;
        }

        ::SetTextureFilter(skyTexture, TEXTURE_FILTER_BILINEAR);
        ::SetTextureWrap(skyTexture, TEXTURE_WRAP_CLAMP);
        material.maps[MATERIAL_MAP_ALBEDO].texture = skyTexture;

        // Pass missing matrices to shader (required by skybox.vs)
        m_Data->SkyboxShader->SetMatrix("matProjection", rlGetMatrixProjection());
        m_Data->SkyboxShader->SetMatrix("matView", rlGetMatrixModelview());

        m_Data->SkyboxShader->SetFloat("exposure", skybox.Exposure);
        m_Data->SkyboxShader->SetFloat("brightness", skybox.Brightness);
        m_Data->SkyboxShader->SetFloat("contrast", skybox.Contrast);
        m_Data->SkyboxShader->SetInt("vflipped", 0);
        m_Data->SkyboxShader->SetInt("doGamma", 0);
        m_Data->SkyboxShader->SetFloat("fragGamma", 2.2f);

        // Fog uniforms
        m_Data->SkyboxShader->SetInt("fogEnabled", m_Data->FogEnabled ? 1 : 0);
        if (m_Data->FogEnabled)
        {
            m_Data->SkyboxShader->SetColor("fogColor", m_Data->FogColor);
            m_Data->SkyboxShader->SetFloat("fogDensity", m_Data->FogDensity);
            m_Data->SkyboxShader->SetFloat("fogStart", m_Data->FogStart);
            m_Data->SkyboxShader->SetFloat("fogEnd", m_Data->FogEnd);
        }

        m_Data->SkyboxShader->SetFloat("uTime", m_Data->Time);

        DrawMesh(m_Data->SkyboxCube.meshes[0], material, MatrixTranslate(camera.position.x, camera.position.y, camera.position.z));

        RenderCommand::EnableBackfaceCulling();
        RenderCommand::EnableDepthMask();
    }

    void Renderer::DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color color)
    {
        if (texture.id == 0) return;
        ::DrawBillboard(camera, texture, position, size, color);
    }

    void Renderer::SetDirectionalLight(Vector3 direction, Color color)
    {
        m_Data->CurrentLightDirection = direction;
        m_Data->CurrentLightColor = color;
    }

    void Renderer::SetAmbientLight(float intensity)
    {
        m_Data->CurrentAmbientIntensity = intensity;
    }

    void Renderer::ApplyEnvironment(const EnvironmentSettings& settings)
    {
        SetAmbientLight(settings.AmbientIntensity);
        SetDirectionalLight(settings.LightDirection, settings.LightColor);

        // Sync Fog to State
        m_Data->FogEnabled = settings.Fog.Enabled;
        m_Data->FogColor = settings.Fog.FogColor;
        m_Data->FogDensity = settings.Fog.Density;
        m_Data->FogStart = settings.Fog.Start;
        m_Data->FogEnd = settings.Fog.End;
    }

    void Renderer::UpdateTime(Timestep time)
    {
        m_Data->Time = time;
    }

    void Renderer::InitializeSkybox()
    {
        Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
        m_Data->SkyboxCube = LoadModelFromMesh(cube);
    }

    void Renderer::EnsureShadersLoaded()
    {
        if (m_Data->LightingShader && m_Data->SkyboxShader && m_Data->LightingShader->GetShader().id != 0 && m_Data->SkyboxShader->GetShader().id != 0)
            return;

        auto project = Project::GetActive();
        if (!project) return;

        auto assetManager = project->GetAssetManager();
        if (!assetManager) return;

        if (!m_Data->LightingShader || m_Data->LightingShader->GetShader().id == 0)
        {
            m_Data->LightingShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/lighting.chshader");
            if (m_Data->LightingShader)
                CH_CORE_INFO("Renderer: Lighting shader loaded lazily.");
        }

        if (!m_Data->SkyboxShader || m_Data->SkyboxShader->GetShader().id == 0)
        {
            m_Data->SkyboxShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/skybox.chshader");
            if (m_Data->SkyboxShader)
                CH_CORE_INFO("Renderer: Skybox shader loaded lazily.");
        }
    }
}
