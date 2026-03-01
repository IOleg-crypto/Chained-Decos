#include "engine/graphics/renderer.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/renderer2d.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "imgui.h"
#include "rlgl.h"
#include <algorithm>
#include <filesystem>
#include <vector>

#include "renderer2d.h"
#include "ui_renderer.h"

#include "engine/core/application.h"

namespace CHEngine
{

bool Renderer::IsInitialized()
{
    return Application::Get().GetRenderer().m_Data != nullptr;
}

Renderer& Renderer::Get()
{
    return Application::Get().GetRenderer();
}

void Renderer::Init()
{
    CH_CORE_INFO("Initializing Render System...");

    m_Renderer2D->Init();
    m_UIRenderer->Init();

    InitializeSkybox();
    CH_CORE_INFO("Render System Initialized (Core).");
}

void Renderer::LoadEngineResources(AssetManager& assetManager)
{
    CH_CORE_INFO("Renderer: Loading engine materials and shaders...");
    auto& renderer = Renderer::Get();
    auto& lib = renderer.GetShaderLibrary();

    // Shaders loading - through AssetManager into the Library
    auto loadShader = [&](const std::string& name, const std::string& path) {
        auto shader = assetManager.Get<ShaderAsset>(path);
        if (shader)
        {
            lib.Add(name, shader);
            return true;
        }
        return false;
    };

    loadShader("Lighting", "engine/resources/shaders/lighting.chshader");
    loadShader("Skybox", "engine/resources/shaders/skybox.chshader");
    loadShader("Unlit", "engine/resources/shaders/unlit.chshader");
    loadShader("CubemapGen", "engine/resources/shaders/cubemap.chshader");
    loadShader("SkyboxCubemap", "engine/resources/shaders/skybox_cubemap.chshader");
    loadShader("PostProcess", "engine/resources/shaders/post_process.chshader");

    // Icons
    auto loadIcon = [&](Texture2D& target, const std::string& path) {
        auto tex = assetManager.Get<TextureAsset>(path);
        if (tex) target = tex->GetTexture();
    };

    loadIcon(renderer.m_Data->LightIcon, "engine/resources/icons/light_bulb.png");
    loadIcon(renderer.m_Data->SpawnIcon, "engine/resources/icons/leaf_icon.png");
    loadIcon(renderer.m_Data->CameraIcon, "engine/resources/icons/camera_icon.png");
}

void Renderer::Shutdown()
{
    CH_CORE_INFO("Shutting down Render System...");
    if (m_Renderer2D) m_Renderer2D->Shutdown();
    if (m_UIRenderer) m_UIRenderer->Shutdown();
    
    InitializeSkybox(); // wait, why InitializeSkybox in shutdown? Ah, it was probably meant to be CleanupSkybox if it existed.
    // Actually, renderer.cpp had InitializeSkybox(); in Init().
}

Renderer::Renderer()
{
    m_Data = std::make_unique<RendererData>();
    m_Renderer2D = std::make_unique<Renderer2D>();
    m_UIRenderer = std::make_unique<UIRenderer>();
    
    m_Data->Shaders = std::make_unique<ShaderLibrary>();

    // Initialize SSBO for lights
    m_Data->LightSSBO = rlLoadShaderBuffer(sizeof(RenderLight) * RendererData::MaxLights, nullptr, RL_DYNAMIC_DRAW);
    m_Data->LightsDirty = true;
}

Renderer::~Renderer()
{
    if (m_Data->LightIcon.id > 0)
    {
        ::UnloadTexture(m_Data->LightIcon);
    }
    if (m_Data->SpawnIcon.id > 0)
    {
        ::UnloadTexture(m_Data->SpawnIcon);
    }
    if (m_Data->CameraIcon.id > 0)
    {
        ::UnloadTexture(m_Data->CameraIcon);
    }

    if (m_Data->LightSSBO > 0)
    rlUnloadShaderBuffer(m_Data->LightSSBO);

    Shutdown();
}

void Renderer::BeginScene(const Camera3D& camera)
{
    m_Data->CurrentCameraPosition = camera.position;

    // Update light SSBO once per frame
    if (m_Data->LightsDirty)
    {
        rlUpdateShaderBuffer(m_Data->LightSSBO, m_Data->Lights, sizeof(RenderLight) * RendererData::MaxLights, 0);
        m_Data->LightsDirty = false;
    }

    // Push global shader uniforms once per frame for the Lighting shader
    if (auto lightingShader = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr)
    {
        rlEnableShader(lightingShader->GetShader().id);
        lightingShader->SetVec3("viewPos", camera.position);
        lightingShader->SetFloat("uTime", m_Data->Time);
        lightingShader->SetFloat("uMode", m_Data->DiagnosticMode);
        lightingShader->SetVec3("lightDir", m_Data->CurrentLighting.Direction);
        lightingShader->SetColor("lightColor", m_Data->CurrentLighting.LightColor);
        lightingShader->SetFloat("ambient", m_Data->CurrentLighting.Ambient);
        lightingShader->SetInt("uLightCount", m_Data->LightCount);
        lightingShader->SetFloat("uExposure", m_Data->CurrentLighting.Exposure);
        lightingShader->SetFloat("uGamma", m_Data->CurrentLighting.Gamma);
        ApplyFogUniforms(lightingShader.get());
        rlBindShaderBuffer(m_Data->LightSSBO, 0);
        m_Data->CurrentShader = lightingShader.get();
    }

    BeginMode3D(camera);
}

void Renderer::EndScene()
{
    m_Data->CurrentShader = nullptr;
    EndMode3D();
}

void Renderer::Clear(Color color)
{
    ClearBackground(color);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    rlViewport(x, y, width, height);
}

void Renderer::DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform,
                         const std::vector<MaterialSlot>& materialSlotOverrides, int animationIndex, float frameIndex,
                         int targetAnimationIndex, float targetFrameIndex, float blendWeight,
                         const std::shared_ptr<ShaderAsset>& shaderOverride,
                         const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    CH_CORE_ASSERT(s_Instance, "Renderer not initialized!");
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready)
    {
        if (modelAsset && modelAsset->GetState() == AssetState::Failed)
        {
            static std::string lastFailed = "";
            if (lastFailed != modelAsset->GetPath())
            {
                //CH_CORE_WARN("Renderer::DrawModel - Model asset failed to load: {}", modelAsset->GetPath());
                lastFailed = modelAsset->GetPath();
            }
        }
        return;
    }

    Model& model = modelAsset->GetModel();

    std::vector<Matrix> boneMatrices = ComputeBoneMatrices(modelAsset, animationIndex, frameIndex, targetAnimationIndex,
                                                           targetFrameIndex, blendWeight);

    for (int i = 0; i < model.meshCount; i++)
    {
        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);
        auto activeShaderAsset = shaderOverride
                                ? shaderOverride
                                : (m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr);
        Matrix meshTransform = MatrixMultiply(model.transform, transform);

        if (activeShaderAsset)
        {
            ShaderAsset* shader = activeShaderAsset.get();
            if (shader != m_Data->CurrentShader)
            {
                // Shader switch — a non-default override shader; re-upload globals
                rlEnableShader(shader->GetShader().id);
                shader->SetVec3("viewPos", m_Data->CurrentCameraPosition);
                shader->SetFloat("uTime", m_Data->Time);
                shader->SetFloat("uMode", m_Data->DiagnosticMode);
                shader->SetVec3("lightDir", m_Data->CurrentLighting.Direction);
                shader->SetColor("lightColor", m_Data->CurrentLighting.LightColor);
                shader->SetFloat("ambient", m_Data->CurrentLighting.Ambient);
                shader->SetInt("uLightCount", m_Data->LightCount);
                shader->SetFloat("uExposure", m_Data->CurrentLighting.Exposure);
                shader->SetFloat("uGamma", m_Data->CurrentLighting.Gamma);
                ApplyFogUniforms(shader);
                rlBindShaderBuffer(m_Data->LightSSBO, 0);
                m_Data->CurrentShader = shader;
            }

            BindShaderUniforms(shader, boneMatrices, shaderUniformOverrides);
            BindMaterialUniforms(shader, material, i, model, materialSlotOverrides);

            Shader originalShader = material.shader;
            material.shader = shader->GetShader();

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
    DrawLine3D(startPosition, endPosition, color);
}

void Renderer::DrawGrid(int sliceCount, float spacing)
{
    ::DrawGrid(sliceCount, spacing);
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
    if (cylinderHeight < 0)
    {
        cylinderHeight = 0;
    }
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

void Renderer::ApplyPostProcessing(RenderTexture2D target, const Camera3D& camera)
{
    CH_PROFILE_FUNCTION();
    
    auto shaderAsset = m_Data->Shaders->Exists("PostProcess") ? m_Data->Shaders->Get("PostProcess") : nullptr;
    if (!shaderAsset)
    {
        return;
    }

    // 1. Prepare Matrices
    Matrix view = GetCameraMatrix(camera);
    // Note: Proj matrix might be tricky if not in Mode3D, but we can reconstruct it
    // Or just use rlGetMatrixProjection() if we call this inside BeginTextureMode
    Matrix proj = rlGetMatrixProjection(); 
    Matrix viewProj = MatrixMultiply(view, proj);
    Matrix invViewProj = MatrixInvert(viewProj);

    // 2. Bind Shader and Uniforms
    ShaderAsset* shader = shaderAsset.get();
    rlEnableShader(shader->GetShader().id);

    shader->SetMatrix("matInverseViewProj", invViewProj);
    shader->SetVec3("viewPos", camera.position);
    shader->SetFloat("uTime", m_Data->Time);
    shader->SetFloat("uExposure", m_Data->CurrentLighting.Exposure);
    shader->SetFloat("uGamma", m_Data->CurrentLighting.Gamma);

    // Fog
    ApplyFogUniforms(shader);

    // 3. Bind Depth Texture to Slot 1
    shader->SetInt("texture1", 1);
    rlActiveTextureSlot(1);
    rlEnableTexture(target.depth.id);
    rlActiveTextureSlot(0); // Back to slot 0 for main texture

    // 4. Draw Fullscreen Quad
    // DrawTextureRec vertically flips the texture (standard FBO behavior)
    Rectangle source = { 0, 0, (float)target.texture.width, (float)-target.texture.height };
    Rectangle dest = { 0, 0, (float)target.texture.width, (float)target.texture.height };
    ::DrawTexturePro(target.texture, source, dest, { 0, 0 }, 0.0f, WHITE);

    // 5. Cleanup
    rlActiveTextureSlot(1);
    rlDisableTexture();
    rlActiveTextureSlot(0);
    rlDisableShader();
}

void Renderer::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
{
    if (skybox.TexturePath.empty())
    {
        return;
    }

    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(skybox.TexturePath);

    if (!textureAsset)
    {
        CH_CORE_WARN("Renderer::DrawSkybox: Failed to find texture asset: {}", skybox.TexturePath);
        return;
    }

    //CH_CORE_INFO("Renderer: Drawing skybox '{}', asset ready: {}", skybox.TexturePath, textureAsset->IsReady());

    if (textureAsset->GetState() != AssetState::Ready)
    {
        // If it's pending, upload it (this will trigger direct LoadTexture for HDR)
        textureAsset->UploadToGPU();
        if (!textureAsset->IsReady())
        {
            return;
        }
    }

    int activeMode = skybox.Mode;
    //CH_CORE_INFO("Renderer: DrawSkybox Mode: {}, Texture: {}", activeMode, skybox.TexturePath);
    auto shaderName = (activeMode == 2) ? "SkyboxCubemap" : "Skybox";
    auto skyboxShader = m_Data->Shaders->Exists(shaderName) ? m_Data->Shaders->Get(shaderName) : nullptr;
    
    if (!skyboxShader || skyboxShader->GetShader().id == 0)
    {
        //CH_CORE_WARN("Renderer::DrawSkybox: Required shader '{}' not found or failed to load. Falling back to default skybox.", shaderName);
        activeMode = 0;
        skyboxShader = m_Data->Shaders->Get("Skybox");
        if (!skyboxShader) return;
    }

    // Cubemap Generation Trigger
    if (activeMode == 2 && !textureAsset->IsCubemap())
    {
        auto genShader = m_Data->Shaders->Exists("CubemapGen") ? m_Data->Shaders->Get("CubemapGen") : nullptr;
        if (genShader && genShader->GetShader().id > 0)
        {
            CH_CORE_INFO("Renderer: Generating cubemap for '{}' (Panorama ID: {})...", skybox.TexturePath, textureAsset->GetTexture().id);
            Texture2D panorama = textureAsset->GetTexture();
            
            // Note: We use PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 or R32G32B32A32 to preserve HDR float values
            TextureCubemap cubemap = GenTextureCubemap(genShader->GetShader(), panorama, 1024, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
            
            if (cubemap.id > 0)
            {
                textureAsset->Unload(); // Unload panorama
                textureAsset->SetTexture(cubemap);
                textureAsset->SetIsCubemap(true);
                CH_CORE_INFO("Renderer: Cubemap generated successfully for '{}' (Cubemap ID: {}).", skybox.TexturePath, cubemap.id);
            }
            else
            {
                CH_CORE_ERROR("Renderer: Failed to generate cubemap for '{}'!", skybox.TexturePath);
                activeMode = 0; // Fallback
                skyboxShader = m_Data->Shaders->Get("Skybox");
            }
        }
        else
        {
             CH_CORE_ERROR("Renderer: CubemapGen shader missing! Cannot generate cubemap.");
             activeMode = 0;
             skyboxShader = m_Data->Shaders->Get("Skybox");
        }
    }

    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    Material& material = m_Data->SkyboxMaterial;
    material.shader = skyboxShader->GetShader();
    Texture2D skyTexture = textureAsset->GetTexture();

    if (skyTexture.id == 0)
    {
        //CH_CORE_ERROR("Renderer::DrawSkybox: Texture asset ready but Raylib texture ID is 0! Path: {}",
                      //skybox.TexturePath);
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
        return;
    }

    if (activeMode == 2)
    {
        // For Cubemap, we assign it to the CUBEMAP map slot and set environmentMap uniform
        // to MATERIAL_MAP_CUBEMAP, so Raylib's DrawMesh handles the binding automatically.
        material.maps[MATERIAL_MAP_CUBEMAP].texture = skyTexture;
    }
    else
    {
        ::SetTextureFilter(skyTexture, TEXTURE_FILTER_BILINEAR);
        ::SetTextureWrap(skyTexture, TEXTURE_WRAP_CLAMP);
        material.maps[MATERIAL_MAP_ALBEDO].texture = skyTexture;
    }

    // Pass missing matrices to shader (required by skybox.vs)
    skyboxShader->SetMatrix("matProjection", rlGetMatrixProjection());
    skyboxShader->SetMatrix("matView", rlGetMatrixModelview());

    skyboxShader->SetFloat("exposure", skybox.Exposure);
    skyboxShader->SetFloat("brightness", skybox.Brightness);
    skyboxShader->SetFloat("contrast", skybox.Contrast);
    
    std::string skyExt = std::filesystem::path(skybox.TexturePath).extension().string();
    std::transform(skyExt.begin(), skyExt.end(), skyExt.begin(), ::tolower);
    bool isHDR = (skyExt == ".hdr");

    if (activeMode != 2)
    {
        skyboxShader->SetInt("vflipped", 0);
        skyboxShader->SetInt("skyboxMode", activeMode); // 0: Equirect, 1: Cross
        
        // Detect HDR texture and enable tone mapping + gamma correction
        skyboxShader->SetInt("isHDR", isHDR ? 1 : 0);
        skyboxShader->SetInt("doGamma", isHDR ? 1 : 0);
        skyboxShader->SetFloat("fragGamma", 2.2f);
    }
    else
    {
        // Cubemap path
        skyboxShader->SetInt("environmentMap", MATERIAL_MAP_CUBEMAP);
        skyboxShader->SetInt("isHDR", isHDR ? 1 : 0);
        skyboxShader->SetInt("doGamma", isHDR ? 1 : 0);
        skyboxShader->SetFloat("fragGamma", 2.2f);
    }

    // Fog uniforms
    ApplyFogUniforms(skyboxShader.get());

    skyboxShader->SetFloat("uTime", m_Data->Time);

    DrawMesh(m_Data->SkyboxCube.meshes[0], material,
             MatrixTranslate(camera.position.x, camera.position.y, camera.position.z));

    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void Renderer::DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color color)
{
    if (texture.id == 0)
    {
        return;
    }
    ::DrawBillboard(camera, texture, position, size, color);
}

void Renderer::ApplyFogUniforms(ShaderAsset* shader)
{
    if (!shader)
    {
        return;
    }
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
    if (index < 0 || index >= RendererData::MaxLights)
    {
        return;
    }
    m_Data->Lights[index] = light;
    m_Data->LightsDirty = true;
}

void Renderer::SetLightCount(int count)
{
    m_Data->LightCount = std::min(count, RendererData::MaxLights);
}

void Renderer::ClearLights()
{
    for (int i = 0; i < RendererData::MaxLights; i++)
    {
        m_Data->Lights[i].enabled = 0;
    }
    m_Data->LightCount = 0;
    m_Data->LightsDirty = true;
}

void Renderer::ApplyEnvironment(const EnvironmentSettings& settings)
{
    // Skip if nothing changed (POD-like member comparison for Lighting and Fog)
    bool lightingChanged = (m_Data->CurrentLighting.Ambient != settings.Lighting.Ambient) ||
                           (m_Data->CurrentLighting.Direction.x != settings.Lighting.Direction.x) ||
                           (m_Data->CurrentLighting.Direction.y != settings.Lighting.Direction.y) ||
                           (m_Data->CurrentLighting.Direction.z != settings.Lighting.Direction.z) ||
                           (m_Data->CurrentLighting.LightColor.r != settings.Lighting.LightColor.r) ||
                           (m_Data->CurrentLighting.LightColor.g != settings.Lighting.LightColor.g) ||
                           (m_Data->CurrentLighting.LightColor.b != settings.Lighting.LightColor.b) ||
                           (m_Data->CurrentLighting.LightColor.a != settings.Lighting.LightColor.a) ||
                           (m_Data->CurrentLighting.Exposure != settings.Lighting.Exposure) ||
                           (m_Data->CurrentLighting.Gamma != settings.Lighting.Gamma);

    bool fogChanged = (m_Data->CurrentFog.Enabled != settings.Fog.Enabled) ||
                      (m_Data->CurrentFog.Density != settings.Fog.Density) ||
                      (m_Data->CurrentFog.Start != settings.Fog.Start) ||
                      (m_Data->CurrentFog.End != settings.Fog.End) ||
                      (m_Data->CurrentFog.FogColor.r != settings.Fog.FogColor.r) ||
                      (m_Data->CurrentFog.FogColor.g != settings.Fog.FogColor.g) ||
                      (m_Data->CurrentFog.FogColor.b != settings.Fog.FogColor.b) ||
                      (m_Data->CurrentFog.FogColor.a != settings.Fog.FogColor.a);

    if (!lightingChanged && !fogChanged)
        return;

    m_Data->CurrentLighting = settings.Lighting;
    m_Data->CurrentFog = settings.Fog;
    
    // If lighting settings changed significantly (like ambient), we might want to flag lights dirty
    // to force a refresh if the shader uses it.
    m_Data->LightsDirty = true;
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
    m_Data->SkyboxMaterial = LoadMaterialDefault();
}

void Renderer::DrawModelInstanced(const std::shared_ptr<ModelAsset>& modelAsset, const std::vector<Matrix>& transforms,
                                   const std::vector<MaterialSlot>& materialSlotOverrides)
{
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready || transforms.empty())
    {
        return;
    }

    Model& model = modelAsset->GetModel();

    auto activeShader = m_Data->Shaders->Exists("Lighting") ? m_Data->Shaders->Get("Lighting") : nullptr;

    for (int i = 0; i < model.meshCount; i++)
    {
        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);

        if (activeShader)
        {
            // Bind lighting uniforms (no bone matrices for instanced rendering)
            BindShaderUniforms(activeShader.get(), {}, {});
            BindMaterialUniforms(activeShader.get(), material, i, model, materialSlotOverrides);

            Shader originalShader = material.shader;
            material.shader = activeShader->GetShader();
            DrawMeshInstanced(model.meshes[i], material, transforms.data(), (int)transforms.size());
            material.shader = originalShader;
        }
        else
        {
            DrawMeshInstanced(model.meshes[i], material, transforms.data(), (int)transforms.size());
        }
    }
}

std::vector<Matrix> Renderer::ComputeBoneMatrices(const std::shared_ptr<ModelAsset>& modelAsset, int animationIndex,
                                                  float frameIndex, int targetAnimationIndex, float targetFrameIndex,
                                                  float blendWeight)
{
    if (!modelAsset)
    {
        return {};
    }

    Model& model = modelAsset->GetModel();
    if (model.boneCount <= 0)
    {
        return {};
    }

    const auto& offsetMatrices = modelAsset->GetOffsetMatrices();
    if (offsetMatrices.empty())
    {
        CH_CORE_WARN("ModelAsset '{}' has bones but no offset matrices loaded.", modelAsset->GetPath());
        return {};
    }

    const int boneCount = model.boneCount;

    // Reuse pre-allocated scratch buffers — avoids heap allocations each frame
    auto& boneMatrices = m_Data->ScratchBoneMatrices;
    auto& globalPose   = m_Data->ScratchGlobalPose;
    auto& localPoseA   = m_Data->ScratchLocalPoseA;
    boneMatrices.resize(boneCount);
    globalPose.resize(boneCount);
    localPoseA.resize(boneCount);

    auto CalculateLocalPose = [&](int animIdx, float fIdx, std::vector<Transform>& outLocalPose) {
        const auto& animations = modelAsset->GetRawAnimations();
        if (animIdx >= 0 && animIdx < (int)animations.size())
        {
            const auto& anim = animations[animIdx];
            int currentFrame = (int)fIdx % anim.frameCount;
            int nextFrame = (currentFrame + 1) % anim.frameCount;
            float interp = fIdx - (float)((int)fIdx);

            for (int i = 0; i < anim.boneCount; i++)
            {
                Transform t     = anim.framePoses[currentFrame * anim.boneCount + i];
                Transform tNext = anim.framePoses[nextFrame    * anim.boneCount + i];

                outLocalPose[i].translation = Vector3Lerp(t.translation, tNext.translation, interp);
                outLocalPose[i].rotation    = QuaternionSlerp(t.rotation, tNext.rotation, interp);
                outLocalPose[i].scale       = Vector3Lerp(t.scale, tNext.scale, interp);
            }
            return true;
        }
        return false;
    };

    bool hasA = CalculateLocalPose(animationIndex, frameIndex, localPoseA);
    if (!hasA)
    {
        for (int i = 0; i < boneCount; i++)
            localPoseA[i] = model.bindPose[i];
    }

    if (targetAnimationIndex >= 0 && blendWeight > 0.0f)
    {
        auto& localPoseB = m_Data->ScratchLocalPoseB;
        localPoseB.resize(boneCount);
        if (CalculateLocalPose(targetAnimationIndex, targetFrameIndex, localPoseB))
        {
            for (int i = 0; i < boneCount; i++)
            {
                localPoseA[i].translation = Vector3Lerp(localPoseA[i].translation, localPoseB[i].translation, blendWeight);
                localPoseA[i].rotation    = QuaternionSlerp(localPoseA[i].rotation, localPoseB[i].rotation, blendWeight);
                localPoseA[i].scale       = Vector3Lerp(localPoseA[i].scale, localPoseB[i].scale, blendWeight);
            }
        }
    }

    // Convert local transforms to global matrices
    for (int i = 0; i < boneCount; i++)
    {
        Matrix localMat = MatrixMultiply(
            QuaternionToMatrix(localPoseA[i].rotation),
            MatrixTranslate(localPoseA[i].translation.x, localPoseA[i].translation.y, localPoseA[i].translation.z));
        localMat = MatrixMultiply(
            MatrixScale(localPoseA[i].scale.x, localPoseA[i].scale.y, localPoseA[i].scale.z), localMat);

        int parent = model.bones[i].parent;
        globalPose[i] = (parent == -1) ? localMat : MatrixMultiply(globalPose[parent], localMat);
    }

    // Compute final skinning matrices
    for (int i = 0; i < boneCount; i++)
    {
        boneMatrices[i] = (i < (int)offsetMatrices.size())
            ? MatrixMultiply(offsetMatrices[i], globalPose[i])
            : globalPose[i];
    }

    return boneMatrices; // RVO applies — no copy in practice
}

Material Renderer::ResolveMaterialForMesh(int meshIndex, const Model& model,
                                          const std::vector<MaterialSlot>& materialSlotOverrides)
{
    Material material = model.materials[model.meshMaterial[meshIndex]];

    for (const auto& slot : materialSlotOverrides)
    {
        bool match = false;
        if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex)
        {
            match = true;
        }
        else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex])
        {
            match = true;
        }

        if (match)
        {
            material.maps[MATERIAL_MAP_ALBEDO].color = slot.Material.AlbedoColor;
            if (slot.Material.OverrideAlbedo && !slot.Material.AlbedoPath.empty())
            {
                if (Project::GetActive())
                {
                    auto textureAsset =
                        Project::GetActive()->GetAssetManager()->Get<TextureAsset>(slot.Material.AlbedoPath);
                    if (textureAsset && textureAsset->IsReady())
                    {
                        material.maps[MATERIAL_MAP_ALBEDO].texture = textureAsset->GetTexture();
                    }
                }
            }
            break;
        }
    }
    return material;
}

void Renderer::BindShaderUniforms(ShaderAsset* activeShader, const std::vector<Matrix>& boneMatrices,
                                  const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!activeShader)
    {
        return;
    }

    if (!boneMatrices.empty())
    {
        int count = (int)boneMatrices.size();
        if (count > 128)
        {
            count = 128; // Shader limit
        }
        activeShader->SetMatrices("boneMatrices", boneMatrices.data(), count);
    }
    else
    {
        // Provide identity matrices for the first few bones to avoid state leakage if the shader is used for static meshes
        static Matrix identityMatrices[4] = {MatrixIdentity(), MatrixIdentity(), MatrixIdentity(), MatrixIdentity()};
        activeShader->SetMatrices("boneMatrices", identityMatrices, 4);
    }

    // Apply custom uniforms from ShaderComponent
    for (const auto& u : shaderUniformOverrides)
    {
        if (u.Type == 0)
        {
            activeShader->SetFloat(u.Name, u.Value[0]);
        }
        else if (u.Type == 1)
        {
            activeShader->SetVec2(u.Name, {u.Value[0], u.Value[1]});
        }
        else if (u.Type == 2)
        {
            activeShader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]});
        }
        else if (u.Type == 3)
        {
            activeShader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
        }
        else if (u.Type == 4)
        {
            activeShader->SetColor(u.Name, Color{(unsigned char)(u.Value[0] * 255), (unsigned char)(u.Value[1] * 255),
                                                 (unsigned char)(u.Value[2] * 255), (unsigned char)(u.Value[3] * 255)});
        }
    }
}


void Renderer::BindMaterialUniforms(ShaderAsset* activeShader, const Material& material, int meshIndex,
                                    const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
{
    if (!activeShader)
    {
        return;
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

    // Emissive defaults
    Color colEmissive = material.maps[MATERIAL_MAP_EMISSION].color;
    float emissiveIntensity = 0.0f;

    // Sync with custom MaterialInstance overrides
    for (const auto& slot : materialSlotOverrides)
    {
        bool match = false;
        if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex)
        {
            match = true;
        }
        else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex])
        {
            match = true;
        }
        if (match)
        {
            emissiveIntensity = slot.Material.EmissiveIntensity;
            if (slot.Material.OverrideEmissive)
            {
                colEmissive = slot.Material.EmissiveColor;
            }
            metalness = slot.Material.Metalness;
            roughness = slot.Material.Roughness;
            break;
        }
    }

    activeShader->SetFloat("metalness", metalness);
    activeShader->SetFloat("roughness", roughness);

    // Fallback: If emissive color is set but intensity is 0, default to 1.0
    if (emissiveIntensity == 0.0f && (colEmissive.r > 0 || colEmissive.g > 0 || colEmissive.b > 0))
    {
        emissiveIntensity = 1.0f;
    }

    activeShader->SetColor("colEmissive", colEmissive);
    activeShader->SetFloat("emissiveIntensity", emissiveIntensity);

    // Map Roughness to Shininess for legacy parts (if any)
    float shininess = (1.0f - roughness) * 128.0f;
    if (shininess < 1.0f)
    {
        shininess = 1.0f;
    }
    activeShader->SetFloat("shininess", shininess);
}
// Generate cubemap texture from HDR texture (Direct port from raylib example)
TextureCubemap Renderer::GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format)
{
    TextureCubemap cubemap = {0};

    rlDisableBackfaceCulling(); // Disable backface culling to render inside the cube

    // STEP 1: Setup framebuffer
    unsigned int rbo = rlLoadTextureDepth(size, size, true);
    cubemap.id = rlLoadTextureCubemap(0, size, format, 1);

    unsigned int fbo = rlLoadFramebuffer();
    rlFramebufferAttach(fbo, rbo, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X, 0);

    // Check if framebuffer is complete with attachments (valid)
    if (rlFramebufferComplete(fbo))
    {
        CH_CORE_INFO("Renderer::GenTextureCubemap: FBO [ID {}] created successfully", fbo);
    }
    else
    {
        CH_CORE_ERROR("Renderer::GenTextureCubemap: FBO failed!");
        return {0};
    }

    // STEP 2: Draw to framebuffer
    rlEnableShader(shader.id);

    // Explicitly find locations to avoid relying on hardcoded ones
    int locProj = GetShaderLocation(shader, "matProjection");
    int locView = GetShaderLocation(shader, "matView");

    // Define projection matrix and send it to shader
    Matrix matFboProjection = MatrixPerspective(90.0 * DEG2RAD, 1.0, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    rlSetUniformMatrix(locProj, matFboProjection);

    // Define view matrix for every side of the cubemap
    Matrix fboViews[6] = {
        MatrixLookAt({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f,  0.0f}),
        MatrixLookAt({0.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
        MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
        MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
        MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
        MatrixLookAt({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f})};

    rlViewport(0, 0, size, size); // Set viewport to current fbo dimensions

    // Activate and enable texture for drawing to cubemap faces
    rlActiveTextureSlot(0);
    rlEnableTexture(panorama.id);

    for (int i = 0; i < 6; i++)
    {
        rlSetUniformMatrix(locView, fboViews[i]);
        rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
        rlEnableFramebuffer(fbo);

        rlClearScreenBuffers();
        rlLoadDrawCube();
    }

    // STEP 3: Unload framebuffer and reset state
    rlDisableShader();
    rlDisableTexture();
    rlDisableFramebuffer();
    rlUnloadFramebuffer(fbo);

    // Reset viewport dimensions to default
    rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());
    rlEnableBackfaceCulling();

    cubemap.width = size;
    cubemap.height = size;
    cubemap.mipmaps = 1;
    cubemap.format = format;

    return cubemap;
}
} // namespace CHEngine
