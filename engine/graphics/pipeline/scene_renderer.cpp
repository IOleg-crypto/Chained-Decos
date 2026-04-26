#include "scene_renderer.h"
#include "engine/audio/audio.h"
#include "engine/core/application.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/ch_assert.h"
#include "engine/core/profiler.h"
#include "engine/graphics/api/renderer_api.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/loaders/model_loader.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/components/shader_component.h"
#include "engine/graphics/pipeline/texture_utility.h"
#include "engine/graphics/texture_system.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/camera_component.h"
#include "engine/scene/components/game_components.h"
#include "engine/scene/components/light_component.h"
#include "engine/scene/components/mesh_component.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_set>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{

glm::mat4 SceneRenderer::GetWorldTransform(entt::registry& registry, entt::entity entity)
{
    if (registry.all_of<TransformComponent>(entity))
    {
        return registry.get<TransformComponent>(entity).WorldTransform;
    }
    return glm::mat4(1.0f);
}

glm::vec3 SceneRenderer::GetWorldPosition(entt::registry& registry, entt::entity entity)
{
    return glm::vec3(GetWorldTransform(registry, entity)[3]);
}

void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip,
                                const SceneRenderOptions& options)
{
    CH_PROFILE_FUNCTION();
    if (!Renderer::IsInitialized() || !scene)
    {
        return;
    }

    RenderCommand::EnableDepthTest();

    auto environment = scene->GetSettings().Environment;
    if (!environment)
    {
        environment = options.EnvironmentOverride;
    }

    if (environment)
    {
        Renderer::Get().ApplyEnvironment(environment->GetSettings());
    }

    Renderer::Get().UpdateTime(Timestep((float)glfwGetTime()));

    m_CurrentStats = {};
    m_CurrentStats.EntityCount = (uint32_t)scene->GetRegistry().storage<entt::entity>().size();

    Audio::Get().SetListenerPosition(camera.Position, glm::normalize(camera.Target - camera.Position), camera.Up);

    Renderer::Get().BeginScene(camera, nearClip, farClip);
    {
        if (environment)
        {
            const auto& envSettings = environment->GetSettings();
            const auto& settings = envSettings.Skybox;
            if (!settings.TexturePath.empty())
            {
                auto textureHandle = TextureSystem::Get().LoadTexture(settings.TexturePath);
                auto texture = TextureSystem::Get().GetTexture(textureHandle);
                if (texture)
                {
                    int skyboxMode = std::clamp(settings.Mode, 0, 2);
                    uint32_t texId = texture->GetRendererID();
                    auto& renderer = Renderer::Get();
                    auto& rd = renderer.GetData();

                    if (skyboxMode == 2)
                    {
                        if (!rd.Skybox.CachedCubemap || rd.Skybox.CachedCubemapPath != settings.TexturePath)
                        {
                            auto genShader = rd.Shaders->LoadOrGet("CubemapGen", "engine/resources/shaders/cubemap.chshader");
                            if (genShader && rd.Skybox.SkyboxCubeModel && !rd.Skybox.SkyboxCubeModel->Meshes.empty())
                            {
                                rd.Skybox.CachedCubemap =
                                    TextureUtility::GenTextureCubemap(genShader->GetShader()->GetRendererID(), texId,
                                                                      1024, rd.Skybox.SkyboxCubeModel->Meshes[0]);
                                rd.Skybox.CachedCubemapPath = settings.TexturePath;
                                rd.Skybox.SourceTextureId = texId;
                            }
                        }
                        if (rd.Skybox.CachedCubemap)
                        {
                            texId = rd.Skybox.CachedCubemap->GetRendererID();
                        }
                    }
                    renderer.DrawSkybox(texId, skyboxMode, TextureSystem::Get().IsHDR(textureHandle), settings.Exposure, settings.Brightness,
                                        settings.Contrast, camera);
                }
            }
        }

        RenderModels(scene, camera, nearClip, farClip);
        RenderSprites(scene, camera);
        RenderDebug(scene, camera, options);
        if (options.ShowEditorIcons)
        {
            RenderEditorIcons(scene, camera);
        }
    }
    Renderer::Get().EndScene();

    Profiler::UpdateStats(m_CurrentStats);
}

void SceneRenderer::RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip)
{
    auto& registry = scene->GetRegistry();
    Frustum frustum;
    {
        float w = (float)Application::Get().GetWindow().GetWidth();
        float h = (float)Application::Get().GetWindow().GetHeight();
        float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
        glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
        glm::mat4 proj =
            (camera.Projection == 0)
                ? glm::perspective(glm::radians(camera.Fovy), aspect, nearClip, farClip)
                : glm::ortho(-aspect * camera.Fovy, aspect * camera.Fovy, -camera.Fovy, camera.Fovy, nearClip, farClip);
        frustum.Extract(proj * view);
    }

    PrepareLights(registry, frustum);

    m_OpaqueQueue.clear();
    m_TransparentQueue.clear();

    RenderCommand::SetBlendMode(true);
    RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);

    CollectAndRenderItems(registry, frustum, camera.Position);

    // Sort transparent queue back-to-front
    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), [](const RenderItem& a, const RenderItem& b) {
        return a.Distance > b.Distance;
    });

    // 1. Opaque Pass
    for (const auto& item : m_OpaqueQueue)
    {
        DrawModel(item.Asset, item.Transform, item.Materials, item.BoneMatrices, item.ShaderOverride, item.CustomUniforms, RenderPass::Opaque);
    }

    // 2. Transparent Pass
    RenderCommand::DisableDepthMask();
    for (const auto& item : m_TransparentQueue)
    {
        DrawModel(item.Asset, item.Transform, item.Materials, item.BoneMatrices, item.ShaderOverride, item.CustomUniforms, RenderPass::Transparent);
    }
    RenderCommand::EnableDepthMask();
    RenderCommand::SetBlendMode(false);
}

void SceneRenderer::PrepareLights(entt::registry& registry, const Frustum& frustum)
{
    Renderer::Get().ClearLights();
    int lightCount = 0;
    auto view = registry.view<LightComponent>();
    for (auto entity : view)
    {
        auto& light = view.get<LightComponent>(entity);
        glm::vec3 worldPos = GetWorldPosition(registry, entity);
        if (light.Type != LightType::Directional && !frustum.IsSphereVisible(worldPos, light.Radius))
        {
            continue;
        }

        RenderLight rl;
        rl.color = {light.LightColor.r / 255.0f, light.LightColor.g / 255.0f, light.LightColor.b / 255.0f,
                    light.LightColor.a / 255.0f};
        rl.position = worldPos;
        rl.intensity = (light.Intensity > 0.0f) ? light.Intensity : 0.0f;
        rl.radius = light.Radius;
        rl.type = (int)light.Type;
        rl.enabled = 1;
        Renderer::Get().SetLight(lightCount++, rl);
    }
    Renderer::Get().SetLightCount(lightCount);
}

void SceneRenderer::CollectAndRenderItems(entt::registry& registry, const Frustum& frustum,
                                          const glm::vec3& cameraPos)
{
    auto meshView = registry.view<TransformComponent, ModelComponent>();
    auto& am = AssetManager::Get();

    for (auto entity : meshView)
    {
        auto [transform, mesh] = meshView.get<TransformComponent, ModelComponent>(entity);
        if (mesh.ModelPath.empty())
        {
            continue;
        }

        auto modelAsset = am.Get<ModelAsset>(mesh.ModelPath);
        if (!modelAsset)
        {
            continue;
        }

        // Sync materials if not already initialized
        mesh.SyncMaterials(modelAsset->GetID());

        AssetState state = modelAsset->GetState();
        if (state != AssetState::Ready)
        {
            continue;
        }

        BoundingBox bbox = modelAsset->GetBoundingBox();
        // Expand bounding box for animated entities to prevent aggressive culling
        if (registry.all_of<AnimationComponent>(entity))
        {
            glm::vec3 center = (bbox.Max + bbox.Min) * 0.5f;
            glm::vec3 size = (bbox.Max - bbox.Min);
            size *= 2.0f; // Double the size for animation range
            bbox.Min = center - size * 0.5f;
            bbox.Max = center + size * 0.5f;
        }

        if (!frustum.IsBoxVisible(bbox, transform.WorldTransform))
        {
            continue;
        }

        std::shared_ptr<ShaderAsset> shaderOver;
        std::vector<ShaderUniform> uniforms;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& sc = registry.get<ShaderComponent>(entity);
            if (sc.Enabled && !sc.ShaderPath.empty())
            {
                shaderOver = am.Get<ShaderAsset>(sc.ShaderPath);
                uniforms = sc.Uniforms;
            }
        }

        std::vector<MaterialSlot> materials = mesh.Materials;
        if (registry.all_of<MaterialComponent>(entity))
        {
            materials = registry.get<MaterialComponent>(entity).Materials;
        }

        RenderItem item;
        item.Asset = modelAsset;
        item.Transform = transform.WorldTransform;
        item.Materials = materials;
        item.ShaderOverride = shaderOver;
        item.CustomUniforms = uniforms;
        
        if (registry.all_of<AnimationComponent>(entity))
        {
            auto& anim = registry.get<AnimationComponent>(entity);
            if (anim.CurrentAnimationIndex >= 0)
                item.BoneMatrices = modelAsset->GetBoneMatrices(anim.CurrentAnimationIndex, anim.CurrentFrame);
        }

        // Determine transparency and push to correct queue
        bool hasOpaque = false;
        bool hasTransparent = false;
        const auto& model = modelAsset->GetModel();
        for (int i = 0; i < (int)model.Meshes.size(); ++i)
        {
            Material mat = ResolveMaterialForMesh(i, model, materials, modelAsset);
            if (mat.Transparent || mat.AlbedoColor.a < 0.99f)
            {
                hasTransparent = true;
            }
            else
            {
                hasOpaque = true;
            }
        }

        // Calculate distance for transparent sorting
        // Note: we can use a simpler position extraction for speed
        glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
        item.Distance = glm::length(cameraPos - worldPos);

        if (hasTransparent) m_TransparentQueue.push_back(item);
        if (hasOpaque) m_OpaqueQueue.push_back(std::move(item));
    }

    auto primView = registry.view<TransformComponent, PrimitiveComponent>();
    for (auto entity : primView)
    {
        auto [transform, primitive] = primView.get<TransformComponent, PrimitiveComponent>(entity);
        if (primitive.Type == PrimitiveType::None)
        {
            continue;
        }

        if (!primitive.Asset || primitive.Dirty)
        {
            const char* paths[] = {
                "", ":cube:", ":sphere:", ":plane:", ":cylinder:", ":cone:", ":torus:", ":knot:", ":hemisphere:"};
            int idx = (int)primitive.Type;
            if (idx > 0 && idx < 9)
            {
                ProceduralParameters p;
                p.Radius = primitive.Radius;
                p.Height = primitive.Height;
                p.Slices = primitive.Slices;
                p.Stacks = primitive.Stacks;
                p.Dimensions = primitive.Dimensions;
                Model m = ModelLoader::GenerateProceduralModel(paths[idx], p);
                if (!m.Meshes.empty())
                {
                    if (!primitive.Asset)
                    {
                        primitive.Asset = std::make_shared<ModelAsset>();
                        primitive.Asset->SetPath(paths[idx]);
                    }
                    primitive.Asset->SetModel(m);
                    primitive.Asset->SetState(AssetState::Ready);
                }
                primitive.Dirty = false;
            }
        }

        if (!primitive.Asset || primitive.Asset->GetState() != AssetState::Ready)
        {
            continue;
        }
        if (!frustum.IsBoxVisible(primitive.Asset->GetBoundingBox(), transform.WorldTransform))
        {
            continue;
        }

        std::shared_ptr<ShaderAsset> shaderOver;
        std::vector<ShaderUniform> uniforms;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& sc = registry.get<ShaderComponent>(entity);
            if (sc.Enabled && !sc.ShaderPath.empty())
            {
                shaderOver = am.Get<ShaderAsset>(sc.ShaderPath);
                uniforms = sc.Uniforms;
            }
        }
        RenderItem item;
        item.Asset = primitive.Asset;
        item.Transform = transform.WorldTransform;
        item.ShaderOverride = shaderOver;
        item.CustomUniforms = uniforms;

        // Primitives are usually opaque, but let's check
        bool hasOpaque = false;
        bool hasTransparent = false;
        const auto& model = primitive.Asset->GetModel();
        for (int i = 0; i < (int)model.Meshes.size(); ++i)
        {
            Material mat = ResolveMaterialForMesh(i, model, {}, primitive.Asset);
            if (mat.Transparent || mat.AlbedoColor.a < 0.99f)
            {
                hasTransparent = true;
            }
            else
            {
                hasOpaque = true;
            }
        }

        glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
        item.Distance = glm::length(cameraPos - worldPos);

        if (hasTransparent) m_TransparentQueue.push_back(item);
        if (hasOpaque) m_OpaqueQueue.push_back(std::move(item));
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries)
{
    for (const auto& entry : animatedEntries)
    {
        std::vector<glm::mat4> boneMatrices;
        if (entry.animation.CurrentAnimationIndex >= 0 && entry.asset)
        {
            boneMatrices = entry.asset->GetBoneMatrices(entry.animation.CurrentAnimationIndex, entry.animation.CurrentFrame);
        }
        DrawModel(entry.asset, entry.worldTransform, entry.materials, boneMatrices, entry.shaderOverride, entry.customUniforms);
    }
}

void SceneRenderer::DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const glm::mat4& transform,
                              const std::vector<MaterialSlot>& materialSlotOverrides,
                              const std::vector<glm::mat4>& boneMatrices,
                              const std::shared_ptr<ShaderAsset>& shaderOverride,
                              const std::vector<ShaderUniform>& shaderUniformOverrides,
                              RenderPass pass)
{
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready)
    {
        return;
    }

    auto& model = modelAsset->GetModel();
    auto& renderer = Renderer::Get();
    auto activeShader =
        shaderOverride
            ? shaderOverride
            : (renderer.GetShaderLibrary().Exists("Lighting") ? renderer.GetShaderLibrary().Get("Lighting") : nullptr);

    if (!activeShader || !activeShader->GetShader())
    {
        return;
    }

    if (shaderOverride)
    {
        static std::unordered_set<std::string> s_LoggedPairs;
        std::string key = shaderOverride->GetPath() + "|" + modelAsset->GetPath();
        if (s_LoggedPairs.insert(key).second)
        {
            CH_CORE_INFO("[SceneRenderer] Applying shader override: '{}' to model: '{}'",
                         shaderOverride->GetPath(), modelAsset->GetPath());
        }
    }

    for (const auto& inst : modelAsset->GetInstances())
    {
        int i = inst.meshIndex;
        if (i < 0 || i >= (int)model.Meshes.size())
        {
            continue;
        }

        m_CurrentStats.DrawCalls++;
        m_CurrentStats.MeshCount++;

        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides, modelAsset);

        bool isTransparent = material.Transparent || material.AlbedoColor.a < 0.99f;
        if (pass == RenderPass::Opaque && isTransparent) continue;
        if (pass == RenderPass::Transparent && !isTransparent) continue;

        BindShaderUniforms(activeShader.get(), boneMatrices, shaderUniformOverrides);
        BindMaterialUniforms(activeShader.get(), material, i, model, materialSlotOverrides);

        uint32_t originalID = material.ShaderID;
        material.ShaderID = activeShader->GetShader()->GetRendererID();

        bool useSkinning = !boneMatrices.empty();
        activeShader->GetShader()->Bind();
        activeShader->GetShader()->SetInt("useSkinning", useSkinning ? 1 : 0);

        renderer.DrawMesh(model.Meshes[i], material, useSkinning ? transform : transform * inst.localTransform);
        material.ShaderID = originalID;
    }
}

Material SceneRenderer::ResolveMaterialForMesh(int meshIndex, const Model& model,
                                               const std::vector<MaterialSlot>& materialSlotOverrides,
                                               const std::shared_ptr<ModelAsset>& modelAsset)
{
    Material material = model.Materials[model.Meshes[meshIndex].MaterialIndex];
    for (const auto& slot : materialSlotOverrides)
    {
        bool match =
            (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex) ||
            (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.Meshes[meshIndex].MaterialIndex);
        if (match)
        {
            material.AlbedoColor = {slot.Material.AlbedoColor.r / 255.0f, slot.Material.AlbedoColor.g / 255.0f,
                                    slot.Material.AlbedoColor.b / 255.0f, slot.Material.AlbedoColor.a / 255.0f};
            if (slot.Material.OverrideAlbedo && !slot.Material.AlbedoPath.empty())
            {
                // Check if this is an embedded texture (path starts with '*')
                if (modelAsset && slot.Material.AlbedoPath.front() == '*')
                {
                    uint32_t embeddedTexID = modelAsset->GetEmbeddedTextureID(slot.Material.AlbedoPath);
                    if (embeddedTexID > 0)
                    {
                        material.AlbedoMap = embeddedTexID;
                    }
                }
                else
                {
                    auto textureHandle = TextureSystem::Get().LoadTexture(slot.Material.AlbedoPath);
                    material.AlbedoMap = TextureSystem::Get().GetRendererID(textureHandle);
                }
            }
            material.EmissiveIntensity = slot.Material.EmissiveIntensity;
            material.Metalness = slot.Material.Metalness;
            material.Roughness = slot.Material.Roughness;
            material.Transparent = slot.Material.Transparent;
            material.Alpha = slot.Material.Alpha;
            break;
        }
    }
    return material;
}

void SceneRenderer::BindShaderUniforms(ShaderAsset* shaderAsset, const std::vector<glm::mat4>& boneMatrices,
                                       const std::vector<ShaderUniform>& overrides)
{
    auto shader = shaderAsset->GetShader();
    shader->Bind();

    auto& rd = Renderer::Get().GetData();
    const auto& lighting = rd.Lighting.CurrentLighting;

    glm::vec4 lightColor = {lighting.LightColor.r / 255.0f, lighting.LightColor.g / 255.0f,
                            lighting.LightColor.b / 255.0f, lighting.LightColor.a / 255.0f};
    glm::vec4 skyColor = lightColor;
    skyColor.w = lighting.Ambient * 0.35f;

    shader->SetVec3("viewPos", rd.CurrentCameraPosition);
    shader->SetFloat("uTime", rd.Time);
    shader->SetFloat("uMode", rd.DiagnosticMode);
    shader->SetVec3("lightDir", lighting.Direction);
    shader->SetVec4("lightColor", lightColor);
    shader->SetFloat("ambient", lighting.Ambient);
    shader->SetVec4("skyAmbientColor", skyColor);
    shader->SetInt("uLightCount", rd.LightCount);
    shader->SetFloat("uExposure", lighting.Exposure);
    shader->SetFloat("uGamma", lighting.Gamma);

    if (!boneMatrices.empty())
    {
        shader->SetMatrices("boneMatrices", boneMatrices.data(), std::min((int)boneMatrices.size(), 128));
    }
    for (const auto& u : overrides)
    {
        if (ShaderComponent::IsSystemUniform(u.Name)) continue; // Skip reserved engine uniforms
        if (u.Type == 0)
        {
            shader->SetFloat(u.Name, u.Value[0]);
        }
        else if (u.Type == 1)
        {
            shader->SetVec2(u.Name, {u.Value[0], u.Value[1]});
        }
        else if (u.Type == 2)
        {
            shader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]});
        }
        else if (u.Type >= 3)
        {
            shader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
        }
    }
}

void SceneRenderer::BindMaterialUniforms(ShaderAsset* shaderAsset, const Material& material, int meshIndex,
                                         const Model& model, const std::vector<MaterialSlot>& overrides)
{
    auto shader = shaderAsset->GetShader();
    shader->Bind();

    auto resolveMap = [](uint32_t currentId, const std::string& path) -> uint32_t {
        if (currentId > 0)
        {
            return currentId;
        }
        // Embedded textures (path starts with '*') are never in the AssetManager.
        // Their GPU ID is stored directly in AlbedoMap/NormalMap etc. by model_asset.cpp.
        if (path.empty() || (!path.empty() && path.front() == '*'))
        {
            return 0;
        }
        auto textureHandle = TextureSystem::Get().LoadTexture(path);
        return TextureSystem::Get().GetRendererID(textureHandle);
    };

    uint32_t albedoMap = resolveMap(material.AlbedoMap, material.AlbedoPath);
    uint32_t normalMap = resolveMap(material.NormalMap, material.NormalPath);
    uint32_t metallicMap = resolveMap(material.MetallicRoughnessMap, material.MetallicRoughnessPath);
    uint32_t emissiveMap = resolveMap(material.EmissiveMap, material.EmissivePath);
    uint32_t occlusionMap = resolveMap(material.OcclusionMap, material.OcclusionPath);

    // 1. Albedo (texture0, Unit 0)
    if (albedoMap > 0)
    {
        RenderCommand::SetTexture(0, albedoMap);
        shader->SetInt("texture0", 0);
        shader->SetInt("useTexture", 1);
    }
    else
    {
        shader->SetInt("useTexture", 0);
    }
    shader->SetVec4("colDiffuse", material.AlbedoColor);

    // 2. Metallic (texture1, Unit 1)
    if (metallicMap > 0)
    {
        RenderCommand::SetTexture(1, metallicMap);
        shader->SetInt("texture1", 1);
        shader->SetInt("useMetallicMap", 1);
        shader->SetInt("useRoughnessMap", 1); // GLTF packed map
    }
    else
    {
        shader->SetInt("useMetallicMap", 0);
        shader->SetInt("useRoughnessMap", 0);
    }

    // 3. Normal (texture2, Unit 2)
    if (normalMap > 0)
    {
        RenderCommand::SetTexture(2, normalMap);
        shader->SetInt("texture2", 2);
        shader->SetInt("useNormalMap", 1);
    }
    else
    {
        shader->SetInt("useNormalMap", 0);
    }

    // 4. Roughness (texture3, Unit 3) - Often same as metallicMap in GLTF
    if (metallicMap > 0)
    {
        RenderCommand::SetTexture(3, metallicMap);
        shader->SetInt("texture3", 3);
    }

    // 5. Occlusion (texture4, Unit 4)
    if (occlusionMap > 0)
    {
        RenderCommand::SetTexture(4, occlusionMap);
        shader->SetInt("texture4", 4);
        shader->SetInt("useOcclusionMap", 1);
    }
    else
    {
        shader->SetInt("useOcclusionMap", 0);
    }

    // 6. Emissive (texture5, Unit 5)
    if (emissiveMap > 1)
    {
        RenderCommand::SetTexture(5, emissiveMap);
        shader->SetInt("texture5", 5);
        shader->SetInt("useEmissiveTexture", 1);
    }
    else
    {
        shader->SetInt("useEmissiveTexture", 0);
    }

    shader->SetFloat("metalness", material.Metalness);
    shader->SetFloat("roughness", material.Roughness);
    shader->SetVec4("colEmissive", material.EmissiveColor);
    shader->SetFloat("emissiveIntensity", material.EmissiveIntensity);
}

void SceneRenderer::RenderDebug(Scene* scene, const Camera3D& camera, const SceneRenderOptions& options)
{
    if (!options.ShowDebugColliders && !options.ShowDebugCollisionModelBox && !options.ShowDebugSpawnZones &&
        !options.DrawGrid)
    {
        return;
    }

    auto& registry = scene->GetRegistry();

    // Save current state
    bool depthTestEnabled = RenderCommand::IsDepthTestEnabled();
    bool blendEnabled = RenderCommand::IsBlendEnabled();

    // Setup for debug drawing
    RenderCommand::DisableDepthTest();
    RenderCommand::SetBlendMode(true);
    RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);
    
    // Polygon offset no longer needed since depth test is OFF
    RenderCommand::SetPolygonOffset(false, 0.0f, 0.0f);

    if (options.SetCollisionWireframeMode == 1)
    {
        RenderCommand::SetPolygonMode(RendererAPI::PolygonMode::Line);
    }
    else
    {
        RenderCommand::SetPolygonMode(RendererAPI::PolygonMode::Fill);
    }

    // if (options.DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
    // {
    //     Renderer::Get().DrawInfiniteGrid(camera, scene->GetSettings().Grid.Spacing, {0.8f, 0.8f, 0.8f, 1.0f});
    // }

    if (options.ShowDebugColliders)
    {
        DrawColliderDebug(registry, options);
    }

    if (options.ShowDebugCollisionModelBox)
    {
        DrawCollisionModelBoxDebug(registry);
    }

    if (options.ShowDebugSpawnZones)
    {
        DrawSpawnDebug(registry);
    }

    // Restore state
    if (depthTestEnabled)
    {
        RenderCommand::EnableDepthTest();
    }
    else
    {
        RenderCommand::DisableDepthTest();
    }

    RenderCommand::SetBlendMode(blendEnabled);
    RenderCommand::SetPolygonOffset(false);
}

void SceneRenderer::DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options)
{
    int mode = options.SetCollisionWireframeMode;
    bool drawSolid = (mode == 1 || mode == 2);
    bool drawWire  = (mode == 0 || mode == 2);

    auto drawPass = [&](bool isWireframe) {
        auto view = registry.view<TransformComponent, ColliderComponent>();
        for (auto entity : view)
        {
            auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
            if (!collider.Enabled)
            {
                continue;
            }
            
            glm::vec4 color = collider.IsColliding ? glm::vec4(1, 0, 0, 0.6f) : glm::vec4(0, 1, 0, 0.6f);
            if (isWireframe)
            {
                color.a = 1.0f; // Ensure wires are fully opaque
            }

            if (collider.Type == ColliderType::Box || collider.Type == ColliderType::Sphere ||
                collider.Type == ColliderType::Capsule)
            {
                // Extract scale from WorldTransform column lengths
                glm::vec3 entityScale(glm::length(glm::vec3(transform.WorldTransform[0])),
                                      glm::length(glm::vec3(transform.WorldTransform[1])),
                                      glm::length(glm::vec3(transform.WorldTransform[2])));

                // Build a rotation-only transform (remove scale from matrix)
                glm::mat4 rotTrans = transform.WorldTransform;
                if (entityScale.x > 0.0001f)
                {
                    rotTrans[0] = glm::vec4(glm::vec3(rotTrans[0]) / entityScale.x, 0.0f);
                }
                if (entityScale.y > 0.0001f)
                {
                    rotTrans[1] = glm::vec4(glm::vec3(rotTrans[1]) / entityScale.y, 0.0f);
                }
                if (entityScale.z > 0.0001f)
                {
                    rotTrans[2] = glm::vec4(glm::vec3(rotTrans[2]) / entityScale.z, 0.0f);
                }

                glm::mat4 baseTransform = rotTrans * glm::translate(glm::mat4(1.0f), collider.Offset);

                if (collider.Type == ColliderType::Box)
                {
                    Renderer::Get().DrawCubeWires(baseTransform, collider.Size * entityScale, color, isWireframe);
                }
                else if (collider.Type == ColliderType::Sphere)
                {
                    // For sphere, we use the maximum component of the entity scale for the overall radius multiplier
                    float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                    Renderer::Get().DrawSphereWires(baseTransform, collider.Radius * maxScale, color, isWireframe);
                }
                else if (collider.Type == ColliderType::Capsule)
                {
                    float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                    Renderer::Get().DrawCapsuleWires(baseTransform, collider.Radius * maxScale,
                                                     collider.Height * entityScale.y, color, isWireframe);
                }
            }
            else if (collider.Type == ColliderType::Mesh)
            {
                AssetHandle modelHandle = collider.ModelHandle;

                // Fallback to visual mesh if AutoCalculate is enable and handle is 0
                if (modelHandle == 0 && collider.AutoCalculate && registry.all_of<ModelComponent>(entity))
                {
                    auto& mc = registry.get<ModelComponent>(entity);
                    auto asset = AssetManager::Get().Get<ModelAsset>(mc.ModelPath);
                    if (asset)
                    {
                        modelHandle = asset->GetID();
                    }
                }

                if (modelHandle != 0)
                {
                    auto modelAsset = AssetManager::Get().Get<ModelAsset>(modelHandle);
                    if (modelAsset && modelAsset->IsReady())
                    {
                        const auto& model = modelAsset->GetModel();
                        const auto& instances = modelAsset->GetInstances();
                        
                        for (const auto& inst : instances)
                        {
                            if (inst.meshIndex >= 0 && inst.meshIndex < (int)model.Meshes.size())
                            {
                                Renderer::Get().DrawMeshWire(model.Meshes[inst.meshIndex], color,
                                                             transform.WorldTransform * inst.localTransform, isWireframe);
                            }
                        }
                    }
                }
            }
        }
    };

    // First draw solid geometries
    if (drawSolid)
    {
        drawPass(false);
    }
    
    // Then draw wireframe overlays on top
    if (drawWire)
    {
        drawPass(true);
    }
}

void SceneRenderer::DrawCollisionModelBoxDebug(entt::registry& registry)
{
    // Draw bounding boxes for collision model box entities
    // This shows mesh collider bounding boxes
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
        if (!collider.Enabled)
        {
            continue;
        }

        // Only render for mesh colliders
        if (collider.Type != ColliderType::Mesh)
        {
            continue;
        }

        glm::vec4 color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for collision model boxes

        if (collider.ModelHandle != 0)
        {
            auto model = AssetManager::Get().Get<ModelAsset>(collider.ModelHandle);
            if (model && model->IsReady())
            {
                const BoundingBox& localBox = model->GetBoundingBox();
                glm::vec3 center = (localBox.Min + localBox.Max) * 0.5f;
                glm::vec3 size = localBox.Max - localBox.Min;

                // Note: transform.WorldTransform already includes entity scale,
                // but the localBox is also scaled if the importer applied scale?
                // Usually localBox is untransformed.
                Renderer::Get().DrawCubeWires(transform.WorldTransform * glm::translate(glm::mat4(1.0f), center), size,
                                              color);
            }
        }
    }
}

void SceneRenderer::DrawSpawnDebug(entt::registry& registry)
{
    // Draw spawn zones
    auto view = registry.view<TransformComponent, SpawnComponent>();
    for (auto entity : view)
    {
        auto [transform, spawn] = view.get<TransformComponent, SpawnComponent>(entity);
        if (!spawn.IsActive)
        {
            continue;
        }

        glm::vec4 color = glm::vec4(1.0f, 0.65f, 0.0f, 1.0f); // Orange for spawn zones

        // Draw zone as wireframe cube
        Renderer::Get().DrawCubeWires(transform.WorldTransform * glm::translate(glm::mat4(1.0f), spawn.SpawnPoint),
                                      spawn.ZoneSize, color);
    }
}

void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera)
{
    auto& registry = scene->GetRegistry();
    const glm::vec3 activeCameraPos = camera.Position;

    auto tryLoadIcon = [&](const char* path, unsigned int& cachedId) {
        if (cachedId != 0)
        {
            return;
        }

        auto textureHandle = TextureSystem::Get().LoadTexture(path);
        cachedId = TextureSystem::Get().GetRendererID(textureHandle);
    };

    tryLoadIcon("engine/resources/icons/camera_icon.png", m_EditorResources.CameraIconId);
    tryLoadIcon("engine/resources/icons/light_bulb.png", m_EditorResources.LightIconId);
    tryLoadIcon("engine/resources/icons/leaf_icon.png", m_EditorResources.SpawnIconId);

    auto iconSizeFromDistance = [&](const glm::vec3& worldPos, float minSize, float maxSize, float scale) {
        const float distanceToCamera = glm::distance(worldPos, activeCameraPos);
        return std::clamp(distanceToCamera * scale, minSize, maxSize);
    };

    // Camera icons
    auto cameraView = registry.view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView)
    {
        auto [transform, cameraComp] = cameraView.get<TransformComponent, CameraComponent>(entity);
        (void)cameraComp;

        const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
        if (glm::distance(iconPos, activeCameraPos) < 0.25f)
        {
            continue;
        }

        const float iconSize = iconSizeFromDistance(iconPos, 0.10f, 0.70f, 0.040f);
        const glm::vec4 cameraTint = glm::vec4(0.65f, 0.95f, 1.0f, 0.95f);
        if (m_EditorResources.CameraIconId != 0)
        {
            Renderer::Get().DrawBillboard(camera, m_EditorResources.CameraIconId, iconPos, iconSize, cameraTint);
        }
        else
        {
            Renderer::Get().DrawCubeWires(transform.WorldTransform, glm::vec3(iconSize * 0.35f), cameraTint);
        }
    }

    // Light icons
    if (scene->GetSettings().DebugFlags.DrawLights)
    {
        auto lightView = registry.view<TransformComponent, LightComponent>();
        for (auto entity : lightView)
        {
            auto [transform, light] = lightView.get<TransformComponent, LightComponent>(entity);
            const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
            const float iconSize = iconSizeFromDistance(iconPos, 0.10f, 0.85f, 0.045f);

            glm::vec4 lightTint = {
                light.LightColor.r / 255.0f,
                light.LightColor.g / 255.0f,
                light.LightColor.b / 255.0f,
                0.95f,
            };

            if (m_EditorResources.LightIconId != 0)
            {
                Renderer::Get().DrawBillboard(camera, m_EditorResources.LightIconId, iconPos, iconSize, lightTint);
                if (light.Type == LightType::Directional)
                {
                    glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.45f;
                    Renderer::Get().DrawLine(iconPos, iconPos + dir, lightTint);
                }
            }
            else if (light.Type == LightType::Directional)
            {
                glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.5f;
                Renderer::Get().DrawLine(iconPos, iconPos + dir, lightTint);
            }
            else if (light.Type == LightType::Point)
            {
                Renderer::Get().DrawSphereWires(transform.WorldTransform, light.Radius * 0.1f, lightTint);
            }
            else if (light.Type == LightType::Spot)
            {
                Renderer::Get().DrawSphereWires(transform.WorldTransform, light.Radius * 0.05f, lightTint);
            }
        }
    }

    // Spawn icons
    if (scene->GetSettings().DebugFlags.DrawSpawnZones)
    {
        auto spawnView = registry.view<TransformComponent, SpawnComponent>();
        for (auto entity : spawnView)
        {
            auto [transform, spawn] = spawnView.get<TransformComponent, SpawnComponent>(entity);
            if (!spawn.IsActive)
            {
                continue;
            }

            const glm::vec3 iconPos =
                glm::vec3(transform.WorldTransform * glm::vec4(spawn.SpawnPoint, 1.0f));
            const float iconSize = iconSizeFromDistance(iconPos, 0.09f, 0.60f, 0.036f);
            const glm::vec4 spawnTint = glm::vec4(1.0f, 0.75f, 0.20f, 0.95f);

            if (m_EditorResources.SpawnIconId != 0)
            {
                Renderer::Get().DrawBillboard(camera, m_EditorResources.SpawnIconId, iconPos, iconSize, spawnTint);
            }
        }
    }
}

BoundingBox SceneRenderer::CalculateColliderWorldAABB(const ColliderComponent& collider,
                                                      const glm::mat4& worldTransform)
{
    glm::vec3 halfExtents = collider.Size * 0.5f;
    glm::vec3 min = collider.Offset - halfExtents;
    glm::vec3 max = collider.Offset + halfExtents;
    glm::vec3 corners[8] = {{min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z},
                            {min.x, min.y, max.z}, {max.x, min.y, max.z}, {min.x, max.y, max.z}, {max.x, max.y, max.z}};
    BoundingBox result = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
    for (int i = 0; i < 8; i++)
    {
        glm::vec3 worldCorner = glm::vec3(worldTransform * glm::vec4(corners[i], 1.0f));
        result.Min.x = (worldCorner.x < result.Min.x) ? worldCorner.x : result.Min.x;
        result.Min.y = (worldCorner.y < result.Min.y) ? worldCorner.y : result.Min.y;
        result.Min.z = (worldCorner.z < result.Min.z) ? worldCorner.z : result.Min.z;
        result.Max.x = (worldCorner.x > result.Max.x) ? worldCorner.x : result.Max.x;
        result.Max.y = (worldCorner.y > result.Max.y) ? worldCorner.y : result.Max.y;
        result.Max.z = (worldCorner.z > result.Max.z) ? worldCorner.z : result.Max.z;
    }
    return result;
}

void SceneRenderer::RenderSprites(Scene* scene, const Camera3D& camera)
{
    CH_PROFILE_FUNCTION();
    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, SpriteComponent>();

    struct SpriteEntry
    {
        entt::entity Entity;
        int ZOrder;
    };

    std::vector<SpriteEntry> sortedSprites;
    for (auto entity : view)
    {
        sortedSprites.push_back({entity, view.get<SpriteComponent>(entity).ZOrder});
    }

    std::sort(sortedSprites.begin(), sortedSprites.end(), [](const SpriteEntry& a, const SpriteEntry& b) {
        if (a.ZOrder != b.ZOrder)
            return a.ZOrder < b.ZOrder;
        return a.Entity < b.Entity;
    });

    for (const auto& entry : sortedSprites)
    {
        auto& transform = view.get<TransformComponent>(entry.Entity);
        auto& sprite = view.get<SpriteComponent>(entry.Entity);

        if (sprite.TexturePath.empty())
            continue;

        auto textureId = TextureSystem::Get().GetRendererID(
            TextureSystem::Get().LoadTexture(sprite.TexturePath));

        if (textureId != 0)
        {
            Renderer::Get().DrawSprite(textureId, transform.WorldTransform,
                                       {sprite.Tint.r / 255.0f, sprite.Tint.g / 255.0f, sprite.Tint.b / 255.0f,
                                        sprite.Tint.a / 255.0f},
                                       sprite.FlipX, sprite.FlipY);
        }
    }
}

} // namespace CHEngine
