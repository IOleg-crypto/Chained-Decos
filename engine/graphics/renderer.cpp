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

        // Shaders loading - through AssetManager into the Library
        if (assetManager)
        {
            auto& lib = s_Instance->GetShaderLibrary();
            
            auto lightingShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/lighting.chshader");
            if (lightingShader) lib.Add("Lighting", lightingShader);

            auto skyboxShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/skybox.chshader");
            if (skyboxShader) lib.Add("Skybox", skyboxShader);
            
            auto unlitShader = assetManager->Get<ShaderAsset>("engine/resources/shaders/unlit.chshader");
            if (unlitShader) lib.Add("Unlit", unlitShader);

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
        m_Data->Shaders = std::make_unique<ShaderLibrary>();
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
        int animationIndex, int frameIndex,
        const std::shared_ptr<ShaderAsset>& shaderOverride,
        const std::vector<ShaderUniform>& shaderUniformOverrides)
    {
        CH_CORE_ASSERT(s_Instance, "Renderer not initialized!");
        if (modelAsset && modelAsset->GetState() == AssetState::Ready)
        {
            Model& model = modelAsset->GetModel();
            //CH_CORE_TRACE("Renderer::DrawModel - Rendering: {} ({} meshes)", modelAsset->GetPath(), model.meshCount);

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
                // Start with the default material for this mesh
                Material material = model.materials[model.meshMaterial[i]];
                
                // Apply overrides from the component
                for (const auto& slot : materialSlotOverrides)
                {
                    bool match = false;
                    if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == i) match = true;
                    else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[i]) match = true;

                    if (match)
                    {
                        material.maps[MATERIAL_MAP_ALBEDO].color = slot.Material.AlbedoColor;
                        if (slot.Material.OverrideAlbedo && !slot.Material.AlbedoPath.empty())
                        {
                            if (Project::GetActive())
                            {
                                auto textureAsset = Project::GetActive()->GetAssetManager()->Get<TextureAsset>(slot.Material.AlbedoPath);
                                if (textureAsset && textureAsset->IsReady())
                                    material.maps[MATERIAL_MAP_ALBEDO].texture = textureAsset->GetTexture();
                            }
                        }
                        break;
                    }
                }

                auto activeShader = shaderOverride;

                if (!activeShader)
                {
                    if (m_Data->Shaders->Exists("Lighting"))
                        activeShader = m_Data->Shaders->Get("Lighting");
                }

                Matrix meshTransform = MatrixMultiply(model.transform, transform);

                if (activeShader)
                {
                    // Apply standard engine uniforms to custom shaders too (if they want them)
                    activeShader->SetVec3("lightDir", m_Data->CurrentLightDirection);
                    activeShader->SetColor("lightColor", m_Data->CurrentLightColor);
                    activeShader->SetFloat("ambient", m_Data->CurrentAmbientIntensity);
                    
                    activeShader->SetInt("fogEnabled", m_Data->FogEnabled ? 1 : 0);
                    if (m_Data->FogEnabled)
                    {
                        activeShader->SetColor("fogColor", m_Data->FogColor);
                        activeShader->SetFloat("fogDensity", m_Data->FogDensity);
                        activeShader->SetFloat("fogStart", m_Data->FogStart);
                        activeShader->SetFloat("fogEnd", m_Data->FogEnd);
                    }

                    activeShader->SetVec3("viewPos", m_Data->CurrentCameraPosition);
                    activeShader->SetFloat("uTime", m_Data->Time);
                    
                    // Set lights (Unified)
                    for (int l = 0; l < RendererData::MaxLights; l++)
                    {
                        std::string base = "lights[" + std::to_string(l) + "].";
                        activeShader->SetColor(base + "color", m_Data->Lights[l].color);
                        activeShader->SetVec3(base + "position", m_Data->Lights[l].position);
                        activeShader->SetVec3(base + "direction", m_Data->Lights[l].direction);
                        activeShader->SetFloat(base + "intensity", m_Data->Lights[l].intensity);
                        activeShader->SetFloat(base + "radius", m_Data->Lights[l].radius);
                        activeShader->SetFloat(base + "innerCutoff", m_Data->Lights[l].innerCutoff);
                        activeShader->SetFloat(base + "outerCutoff", m_Data->Lights[l].outerCutoff);
                        activeShader->SetInt(base + "type", m_Data->Lights[l].type);
                        activeShader->SetInt(base + "enabled", m_Data->Lights[l].enabled ? 1 : 0);
                    }

                    // Set untextured support hint
                    activeShader->SetInt("useTexture", material.maps[MATERIAL_MAP_ALBEDO].texture.id > 0 ? 1 : 0);
                    
                    // Set material properties
                    activeShader->SetColor("colDiffuse", material.maps[MATERIAL_MAP_ALBEDO].color);
                    
                    // PBR Maps & Uniforms
                    int useNormalMap = material.maps[MATERIAL_MAP_NORMAL].texture.id > 0 ? 1 : 0;
                    int useMetallicMap = material.maps[MATERIAL_MAP_METALNESS].texture.id > 0 ? 1 : 0;
                    int useRoughnessMap = material.maps[MATERIAL_MAP_ROUGHNESS].texture.id > 0 ? 1 : 0;
                    int useOcclusionMap = material.maps[MATERIAL_MAP_OCCLUSION].texture.id > 0 ? 1 : 0;
                    int useEmissiveTexture = material.maps[MATERIAL_MAP_EMISSION].texture.id > 0 ? 1 : 0;

                    activeShader->SetInt("useNormalMap", useNormalMap);
                    activeShader->SetInt("useMetallicMap", useMetallicMap);
                    activeShader->SetInt("useRoughnessMap", useRoughnessMap);
                    activeShader->SetInt("useOcclusionMap", useOcclusionMap);
                    activeShader->SetInt("useEmissiveTexture", useEmissiveTexture);

                    float metalness = material.maps[MATERIAL_MAP_METALNESS].value;
                    float roughness = material.maps[MATERIAL_MAP_ROUGHNESS].value;

                    activeShader->SetFloat("metalness", metalness);
                    activeShader->SetFloat("roughness", roughness);

                    // Emissive
                    Color colEmissive = material.maps[MATERIAL_MAP_EMISSION].color;
                    float emissiveIntensity = 0.0f;
                    
                    // Sync with custom MaterialInstance if available
                    for (const auto& slot : materialSlotOverrides) {
                        bool match = false;
                        if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == i) match = true;
                        else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[i]) match = true;
                        if (match) {
                            emissiveIntensity = slot.Material.EmissiveIntensity;
                            if (slot.Material.OverrideEmissive) colEmissive = slot.Material.EmissiveColor;
                            
                            // Also potential overrides for roughness/metalness if we add them to MaterialInstance
                            roughness = slot.Material.Roughness;
                            activeShader->SetFloat("roughness", roughness);
                            break;
                        }
                    }

                    // Fallback: If emissive color is set but intensity is 0, default to 1.0
                    if (emissiveIntensity == 0.0f && (colEmissive.r > 0 || colEmissive.g > 0 || colEmissive.b > 0)) {
                        emissiveIntensity = 1.0f;
                    }

                    activeShader->SetColor("colEmissive", colEmissive);
                    activeShader->SetFloat("emissiveIntensity", emissiveIntensity);
                    
                    // Map Roughness to Shininess for legacy parts (if any)
                    float shininess = (1.0f - roughness) * 128.0f;
                    if (shininess < 1.0f) shininess = 1.0f;
                    activeShader->SetFloat("shininess", shininess);

                    // Set global diagnostic mode
                    activeShader->SetFloat("uMode", m_Data->DiagnosticMode);

                    // Apply custom uniforms from ShaderComponent
                    for (const auto& u : shaderUniformOverrides)
                    {
                        if (u.Type == 0) activeShader->SetFloat(u.Name, u.Value[0]);
                        else if (u.Type == 1) activeShader->SetVec2(u.Name, {u.Value[0], u.Value[1]});
                        else if (u.Type == 2) activeShader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]});
                        else if (u.Type == 3) activeShader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
                        else if (u.Type == 4) activeShader->SetColor(u.Name, Color{(unsigned char)(u.Value[0]*255), (unsigned char)(u.Value[1]*255), (unsigned char)(u.Value[2]*255), (unsigned char)(u.Value[3]*255)});
                    }

                    // Actually apply the shader to the material for this draw call
                    Shader originalShader = material.shader;
                    material.shader = activeShader->GetShader();
                    
                    ProfilerStats stats;
                    stats.DrawCalls++;
                    stats.MeshCount++;
                    stats.PolyCount += model.meshes[i].triangleCount;
                    Profiler::UpdateStats(stats);

                    DrawMesh(model.meshes[i], material, meshTransform);
                    
                    material.shader = originalShader;
                }
                else
                {
                    // Fallback DrawMesh
                    DrawMesh(model.meshes[i], material, meshTransform);
                }
            }
        }
        else
        {
            if (modelAsset && modelAsset->GetState() == AssetState::Failed)
            {
                // Only log if we haven't logged recently? For now, just change to Trace or Debug to reduce spam.
                // Or check if we should warn.
                static std::string lastFailed = "";
                if (lastFailed != modelAsset->GetPath()) {
                    CH_CORE_WARN("Renderer::DrawModel - Model asset failed to load: {}", modelAsset->GetPath());
                    lastFailed = modelAsset->GetPath();
                }
            }
            // If Loading or None, just return silently.
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

    void Renderer::DrawCapsuleWires(const Matrix& transform, float radius, float height, Color color)
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));

        float cylinderHeight = height - 2.0f * radius;
        if (cylinderHeight < 0) cylinderHeight = 0;
        float halfCylinder = cylinderHeight * 0.5f;

        // Draw Cylinder (aligned to Y axis)
        if (cylinderHeight > 0)
        {
            Vector3 start = {0, -halfCylinder, 0};
            Vector3 end = {0, halfCylinder, 0};
            ::DrawCylinderWiresEx(start, end, radius, radius, 8, color);
        }

        // Draw Caps
        ::DrawSphereWires({0, -halfCylinder, 0}, radius, 8, 8, color);
        ::DrawSphereWires({0, halfCylinder, 0}, radius, 8, 8, color);

        rlPopMatrix();
    }

    void Renderer::DrawSphereWires(const Matrix& transform, float radius, Color color)
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));

        ::DrawSphereWires({0, 0, 0}, radius, 8, 8, color);

        rlPopMatrix();
    }

    void Renderer::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
    {
        if (skybox.TexturePath.empty()) return;

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
        
        auto skyboxShader = m_Data->Shaders->Exists("Skybox") ? m_Data->Shaders->Get("Skybox") : nullptr;
        if (!skyboxShader || skyboxShader->GetShader().id == 0) return;

        RenderCommand::DisableBackfaceCulling();
        RenderCommand::DisableDepthMask();

        Material material = LoadMaterialDefault();
        material.shader = skyboxShader->GetShader();
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
        skyboxShader->SetMatrix("matProjection", rlGetMatrixProjection());
        skyboxShader->SetMatrix("matView", rlGetMatrixModelview());

        skyboxShader->SetFloat("exposure", skybox.Exposure);
        skyboxShader->SetFloat("brightness", skybox.Brightness);
        skyboxShader->SetFloat("contrast", skybox.Contrast);
        skyboxShader->SetInt("vflipped", 0);
        skyboxShader->SetInt("skyboxMode", skybox.Mode); // 0: Equirect, 1: Cross
        skyboxShader->SetInt("doGamma", 0);
        skyboxShader->SetFloat("fragGamma", 2.2f);

        // Fog uniforms
        skyboxShader->SetInt("fogEnabled", m_Data->FogEnabled ? 1 : 0);
        if (m_Data->FogEnabled)
        {
            skyboxShader->SetColor("fogColor", m_Data->FogColor);
            skyboxShader->SetFloat("fogDensity", m_Data->FogDensity);
            skyboxShader->SetFloat("fogStart", m_Data->FogStart);
            skyboxShader->SetFloat("fogEnd", m_Data->FogEnd);
        }

        skyboxShader->SetFloat("uTime", m_Data->Time);

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

    void Renderer::SetLight(int index, const RenderLight& light)
    {
        if (index < 0 || index >= RendererData::MaxLights) return;
        m_Data->Lights[index] = light;
    }

    void Renderer::ClearLights()
    {
        for (int i = 0; i < RendererData::MaxLights; i++)
            m_Data->Lights[i].enabled = false;
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

    void Renderer::SetDiagnosticMode(float mode)
    {
        m_Data->DiagnosticMode = mode;
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
        auto project = Project::GetActive();
        if (!project) return;

        auto assetManager = project->GetAssetManager();
        if (!assetManager) return;
        
        auto& lib = s_Instance->GetShaderLibrary();

        // 1. Lighting Shader
        if (!lib.Exists("Lighting"))
        {
            auto shader = assetManager->Get<ShaderAsset>("engine/resources/shaders/lighting.chshader");
            if (shader) 
            {
                lib.Add("Lighting", shader);
                CH_CORE_INFO("Renderer: 'Lighting' shader loaded lazily.");
            }
        }

        // 2. Skybox Shader
        if (!lib.Exists("Skybox"))
        {
            auto shader = assetManager->Get<ShaderAsset>("engine/resources/shaders/skybox.chshader");
            if (shader) 
            {
                lib.Add("Skybox", shader);
                CH_CORE_INFO("Renderer: 'Skybox' shader loaded lazily.");
            }
        }
    }
}
