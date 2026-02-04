#include "engine/graphics/render.h"
#include "engine/graphics/render_command.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/scene/entity.h"
#include "engine/core/profiler.h"
#include "imgui.h"
#include "rlgl.h"
#include <algorithm>
#include <vector>
#include <filesystem>

namespace CHEngine
{
    RenderState Render::s_State;

    void Render::Init()
    {
        CH_CORE_INFO("Initializing Render System...");
        s_State = RenderState();
        
        RenderCommand::Init();
        
        AssetManager am;
        am.Initialize();
        
        std::string lightingPath = am.ResolvePath("engine/resources/shaders/lighting.chshader");
        std::string skyboxPath = am.ResolvePath("engine/resources/shaders/skybox.chshader");

        if (std::filesystem::exists(lightingPath))
            s_State.LightingShader = ShaderAsset::Load(lightingPath);
        else
            CH_CORE_WARN("Render::Init: Lighting shader not found at {}", lightingPath);
            
        if (std::filesystem::exists(skyboxPath))
            s_State.SkyboxShader = ShaderAsset::Load(skyboxPath);
        else
            CH_CORE_WARN("Render::Init: Skybox shader not found at {}", skyboxPath);
            
        InitSkybox();
        CH_CORE_INFO("Render System Initialized.");
    }

    void Render::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");
        RenderCommand::Shutdown();
    }

    void Render::BeginScene(const Camera3D& camera)
    {
        Submit([camera]() {
            BeginMode3D(camera);
        });
    }

    void Render::EndScene()
    {
        Submit([]() {
            EndMode3D();
        });
    }

    void Render::DrawScene(Scene* scene, const Camera3D& camera, Timestep ts, const DebugRenderFlags* debugFlags)
    {
        SceneRenderer::RenderScene(scene, camera, ts, debugFlags);
    }

    void Render::DrawUI(Scene* scene, const ImVec2& refPos, const ImVec2& refSize, bool editMode)
    {
        UIRenderer::DrawCanvas(scene, refPos, refSize, editMode);
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
                         const std::vector<MaterialSlot>& overrides, 
                         int animIndex, int frame)
    {
        auto project = Project::GetActive();
        if (!project) return;
        
        auto modelAsset = project->GetAssetManager()->Get<ModelAsset>(path);
        if (!modelAsset) return;

        Submit([modelAsset, transform]() {
            if (modelAsset && modelAsset->GetState() == AssetState::Ready)
            {
                Model& model = modelAsset->GetModel();
                for (int i = 0; i < model.meshCount; i++)
                {
                    Material& mat = model.materials[model.meshMaterial[i]];
                    
                    if (mat.shader.id == 0 && s_State.LightingShader)
                        mat.shader = s_State.LightingShader->GetShader();

                    if (s_State.LightingShader && mat.shader.id == s_State.LightingShader->GetShader().id)
                    {
                        s_State.LightingShader->SetVec3("lightDir", s_State.CurrentLightDir);
                        s_State.LightingShader->SetColor("lightColor", s_State.CurrentLightColor);
                        s_State.LightingShader->SetFloat("ambient", s_State.CurrentAmbientIntensity);
                    }

                    ProfilerStats stats;
                    stats.DrawCalls++;
                    stats.MeshCount++;
                    stats.PolyCount += model.meshes[i].triangleCount;
                    Profiler::UpdateStats(stats);

                    DrawMesh(model.meshes[i], mat, transform);
                }
            }
        });
    }

    void Render::DrawLine(Vector3 start, Vector3 end, Color color)
    {
        RenderCommand::DrawLine(start, end, color);
    }

    void Render::DrawGrid(int slices, float spacing)
    {
        RenderCommand::DrawGrid(slices, spacing);
    }

    void Render::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
    {
        if (!s_State.SkyboxShader || skybox.TexturePath.empty()) return;

        Submit([skybox, camera]() {
            auto project = Project::GetActive();
            if (!project) return;
            
            auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(skybox.TexturePath);
            if (!textureAsset || textureAsset->GetState() != AssetState::Ready) return;

            RenderCommand::DisableBackfaceCulling();
            RenderCommand::DisableDepthMask();

            Material mat = LoadMaterialDefault();
            mat.shader = s_State.SkyboxShader->GetShader();
            mat.maps[MATERIAL_MAP_ALBEDO].texture = textureAsset->GetTexture();
            
            s_State.SkyboxShader->SetFloat("exposure", skybox.Exposure);
            s_State.SkyboxShader->SetFloat("brightness", skybox.Brightness);
            s_State.SkyboxShader->SetFloat("contrast", skybox.Contrast);
            s_State.SkyboxShader->SetInt("vflipped", 0);
            s_State.SkyboxShader->SetInt("doGamma", 0);
            s_State.SkyboxShader->SetFloat("fragGamma", 2.2f);

            DrawMesh(s_State.SkyboxCube.meshes[0], mat, MatrixTranslate(camera.position.x, camera.position.y, camera.position.z));

            RenderCommand::EnableBackfaceCulling();
            RenderCommand::EnableDepthMask();
        });
    }

    RenderState& Render::GetState()
    {
        return s_State;
    }

    void Render::SetDirectionalLight(Vector3 direction, Color color)
    {
        s_State.CurrentLightDir = direction;
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
    }

    void Render::InitSkybox()
    {
        Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
        s_State.SkyboxCube = LoadModelFromMesh(cube);
    }
}
