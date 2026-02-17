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
        int animationIndex, float frameIndex,
        const std::shared_ptr<ShaderAsset>& shaderOverride,
        const std::vector<ShaderUniform>& shaderUniformOverrides)
    {
        CH_CORE_ASSERT(s_Instance, "Renderer not initialized!");
        if (!modelAsset || modelAsset->GetState() != AssetState::Ready)
        {
            if (modelAsset && modelAsset->GetState() == AssetState::Failed)
            {
                static std::string lastFailed = "";
                if (lastFailed != modelAsset->GetPath()) {
                    CH_CORE_WARN("Renderer::DrawModel - Model asset failed to load: {}", modelAsset->GetPath());
                    lastFailed = modelAsset->GetPath();
                }
            }
            return;
        }

        Model& model = modelAsset->GetModel();
        std::vector<Matrix> boneMatrices = ComputeBoneMatrices(modelAsset, animationIndex, frameIndex);

        for (int i = 0; i < model.meshCount; i++)
        {
            Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);
            auto activeShader = shaderOverride ? shaderOverride : (m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr);
            Matrix meshTransform = MatrixMultiply(model.transform, transform);

            if (activeShader)
            {
                BindShaderUniforms(activeShader.get(), boneMatrices, shaderUniformOverrides);
                BindMaterialUniforms(activeShader.get(), material, i, model, materialSlotOverrides);

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
                DrawMesh(model.meshes[i], material, meshTransform);
            }
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
        ApplyFogUniforms(skyboxShader.get());

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

    void Renderer::ApplyFogUniforms(ShaderAsset* shader)
    {
        if (!shader) return;
        auto& fog = m_Data->CurrentFog;
        shader->SetInt("fogEnabled", fog.Enabled ? 1 : 0);
        if (fog.Enabled)
        {
            shader->SetColor("fogColor", fog.FogColor);
            shader->SetFloat("fogDensity", fog.Density);
            shader->SetFloat("fogStart", fog.Start);
            shader->SetFloat("fogEnd", fog.End);
        }
    }

    void Renderer::SetDirectionalLight(Vector3 direction, Color color)
    {
        m_Data->CurrentLighting.Direction = direction;
        m_Data->CurrentLighting.LightColor = color;
    }

    void Renderer::SetAmbientLight(float intensity)
    {
        m_Data->CurrentLighting.Ambient = intensity;
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
        m_Data->CurrentLighting = settings.Lighting;
        m_Data->CurrentFog = settings.Fog;
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

    std::vector<Matrix> Renderer::ComputeBoneMatrices(const std::shared_ptr<ModelAsset>& modelAsset, int animationIndex, float frameIndex)
    {
        if (!modelAsset) return {};

        Model& model = modelAsset->GetModel();
        if (model.boneCount <= 0) return {};

        const auto& offsetMatrices = modelAsset->GetOffsetMatrices();
        if (offsetMatrices.empty()) {
            CH_CORE_WARN("ModelAsset '{}' has bones but no offset matrices loaded.", modelAsset->GetPath());
            return {};
        }

        std::vector<Matrix> boneMatrices(model.boneCount);
        std::vector<Matrix> globalPose(model.boneCount);

        // 1. Calculate Global Animation Pose with Interpolation
        const auto& animations = modelAsset->GetRawAnimations();
        
        if (animationIndex >= 0 && animationIndex < (int)animations.size())
        {
            const auto& anim = animations[animationIndex];
            int currentFrame = (int)frameIndex % anim.frameCount;
            int nextFrame = (currentFrame + 1) % anim.frameCount;
            float interp = frameIndex - (float)((int)frameIndex);
            
            for (int i = 0; i < anim.boneCount; i++)
            {
                Transform t = anim.framePoses[currentFrame * anim.boneCount + i];
                Transform tNext = anim.framePoses[nextFrame * anim.boneCount + i];

                Vector3 translation = Vector3Lerp(t.translation, tNext.translation, interp);
                Quaternion rotation = QuaternionSlerp(t.rotation, tNext.rotation, interp);
                Vector3 scale = Vector3Lerp(t.scale, tNext.scale, interp);

                Matrix localMat = MatrixMultiply(QuaternionToMatrix(rotation), MatrixTranslate(translation.x, translation.y, translation.z));
                localMat = MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), localMat);

                int parent = model.bones[i].parent;
                if (parent == -1) globalPose[i] = localMat;
                else globalPose[i] = MatrixMultiply(globalPose[parent], localMat);
            }
        }
        else
        {
            // Use global bind poses if no animation
            // Actually, for KISS: just use local transforms from bindPose
            for (int i = 0; i < model.boneCount; i++) {
                Matrix localBind = MatrixMultiply(QuaternionToMatrix(model.bindPose[i].rotation), MatrixTranslate(model.bindPose[i].translation.x, model.bindPose[i].translation.y, model.bindPose[i].translation.z));
                int parent = model.bones[i].parent;
                if (parent == -1) globalPose[i] = localBind;
                else globalPose[i] = MatrixMultiply(localBind, globalPose[parent]);
            }
        }

        // 3. Compute final skinning matrices (OffsetMatrix * GlobalPose)
        // Order depends on convention. Assimp: Final = GlobalTransform * OffsetMatrix
        for (int i = 0; i < model.boneCount; i++)
        {
            if (i < (int)offsetMatrices.size()) {
                boneMatrices[i] = MatrixMultiply(offsetMatrices[i], globalPose[i]);
            } else {
                boneMatrices[i] = MatrixIdentity();
            }
        }

        return boneMatrices;
    }

    Material Renderer::ResolveMaterialForMesh(int meshIndex, const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
    {
        Material material = model.materials[model.meshMaterial[meshIndex]];
        
        for (const auto& slot : materialSlotOverrides)
        {
            bool match = false;
            if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex) match = true;
            else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex]) match = true;

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
        return material;
    }

    void Renderer::BindShaderUniforms(ShaderAsset* activeShader, const std::vector<Matrix>& boneMatrices, const std::vector<ShaderUniform>& shaderUniformOverrides)
    {
        if (!activeShader) return;

        activeShader->SetVec3("lightDir", m_Data->CurrentLighting.Direction);
        activeShader->SetColor("lightColor", m_Data->CurrentLighting.LightColor);
        activeShader->SetFloat("ambient", m_Data->CurrentLighting.Ambient);
        
        ApplyFogUniforms(activeShader);

        if (!boneMatrices.empty())
        {
            int count = (int)boneMatrices.size();
            if (count > 128) count = 128; // Shader limit
            activeShader->SetMatrices("boneMatrices", boneMatrices.data(), count);
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
    }

    void Renderer::BindMaterialUniforms(ShaderAsset* activeShader, const Material& material, int meshIndex, const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
    {
        if (!activeShader) return;

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

        // Emissive defaults
        Color colEmissive = material.maps[MATERIAL_MAP_EMISSION].color;
        float emissiveIntensity = 0.0f;
        
        // Sync with custom MaterialInstance overrides
        for (const auto& slot : materialSlotOverrides) {
            bool match = false;
            if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex) match = true;
            else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex]) match = true;
            if (match) {
                emissiveIntensity = slot.Material.EmissiveIntensity;
                if (slot.Material.OverrideEmissive) colEmissive = slot.Material.EmissiveColor;
                metalness = slot.Material.Metalness;
                roughness = slot.Material.Roughness;
                break;
            }
        }

        activeShader->SetFloat("metalness", metalness);
        activeShader->SetFloat("roughness", roughness);

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
    }
}
