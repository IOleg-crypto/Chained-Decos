#include "scene_renderer.h"
#include "engine/assets/asset.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/graphics/api/renderer_api.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/texture_utility.h"
#include "engine/graphics/pipeline/renderer2d.h"
#include "engine/graphics/pipeline/renderer3d.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/camera_component.h"
#include "engine/scene/components/light_component.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/scene/components/primitive_component.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/components/transform_component.h"
#include "engine/scene/entity.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "engine/graphics/pipeline/passes/geometry_pass.h"
#include "engine/graphics/pipeline/passes/skybox_pass.h"
#include "engine/graphics/pipeline/passes/shadow_pass.h"
#include "engine/graphics/pipeline/passes/composite_pass.h"



namespace Chained
{

// --- Obsolete local utilities removed ---

static glm::vec4 ColorToVec4(const Color& c)
{
    return { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
}

SceneRenderer::SceneRenderer()
{
    m_Lighting.LightSSBO = StorageBuffer::Create(sizeof(RenderLight) * LightingData::MaxLights);
    m_Lighting.LightsDirty = true;

    AddPass(std::make_unique<ShadowPass>());
    AddPass(std::make_unique<SkyboxPass>());
    AddPass(std::make_unique<GeometryPass>());
    AddPass(std::make_unique<CompositePass>());

}

void SceneRenderer::AddPass(std::unique_ptr<IRenderPass> pass)
{
    pass->Init();
    m_RenderPasses.push_back(std::move(pass));
}

std::optional<Camera3D> SceneRenderer::GetActiveCamera(entt::registry& reg)
{
    auto view = reg.view<CameraComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto [camera, transform] = view.get<CameraComponent, TransformComponent>(entity);
        if (camera.Primary)
        {
            Camera3D activeCamera;

            // Use WorldTransform instead of local translation/rotation for parented cameras
            const glm::mat4& worldTransform = transform.WorldTransform;

            // Extract position from world transform
            activeCamera.Position = glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            // Calculate world forward and up vectors
            glm::vec3 worldForward = glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f));
            glm::vec3 worldUp = glm::vec3(worldTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

            glm::vec3 forward = glm::normalize(worldForward - activeCamera.Position);

            // Provide a safe fallback if forward is somehow zero
            if (glm::length2(forward) < 0.0001f)
            {
                forward = glm::vec3(0.0f, 0.0f, -1.0f);
            }

            activeCamera.Target = activeCamera.Position + forward;
            activeCamera.Up = glm::normalize(worldUp - activeCamera.Position);

            // SceneCamera stores FOV in radians, raylib expects degrees for Camera3D
            activeCamera.FovY = glm::degrees(camera.Camera.GetPerspectiveVerticalFOV());
            activeCamera.Projection = (int)camera.Camera.GetProjectionType();

            return activeCamera;
        }
    }
    return std::nullopt;
}

Entity SceneRenderer::GetPrimaryCameraEntity(entt::registry& reg, entt::registry* registryPtr)
{
    auto view = reg.view<CameraComponent>();
    for (auto entity : view)
    {
        if (view.get<CameraComponent>(entity).Primary)
        {
            return {entity, registryPtr};
        }
    }
    return {};
}

void SceneRenderer::RenderScene(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera,
                                float nearClip, float farClip, const SceneRenderOptions& options)
{
    CH_PROFILE_FUNCTION();

    RenderCommand::EnableDepthTest();

    auto environment = settings.Environment;
    if (!environment)
    {
        environment = options.EnvironmentOverride;
    }

    if (environment)
    {
        const auto& envSettings = environment->GetSettings();
        m_Lighting.CurrentLighting = envSettings.Lighting;
        m_Lighting.CurrentFog = envSettings.Fog;
        m_CurrentEnv = envSettings;
    }

    Renderer::UpdateTime(Timestep((float)glfwGetTime()));

    m_CurrentStats = {};
    m_CurrentStats.EntityCount = (uint32_t)registry.storage<entt::entity>().size();


    Renderer::BeginScene(camera, nearClip, farClip);

    // Update Light SSBO
    if (m_Lighting.LightsDirty && m_Lighting.LightSSBO)
    {
        m_Lighting.LightSSBO->SetData(m_Lighting.Lights, sizeof(RenderLight) * LightingData::MaxLights);
        m_Lighting.LightsDirty = false;
    }
    if (m_Lighting.LightSSBO)
    {
        m_Lighting.LightSSBO->BindBase(0);
    }

    RenderContext ctx {
        registry,
        settings,
        camera,
        options,
        nearClip,
        farClip,
        this
    };

    float w = (float)Renderer::GetViewportWidth();
    float h = (float)Renderer::GetViewportHeight();
    float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
    glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
    glm::mat4 proj =
        (camera.Projection == 0)
            ? glm::perspective(glm::radians(camera.FovY), aspect, nearClip, farClip)
            : glm::ortho(-aspect * camera.FovY, aspect * camera.FovY, -camera.FovY, camera.FovY, nearClip, farClip);
    Frustum frustum = FromMatrix(proj * view);

    PrepareLights(registry, frustum);

    m_OpaqueQueue.clear();
    m_TransparentQueue.clear();

    CollectAndRenderItems(registry, frustum, camera.Position);

    // Sort transparent queue back-to-front once for all passes
    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(),
              [](const auto& a, const auto& b) { return a.Distance > b.Distance; });

    for (auto& pass : m_RenderPasses)
    {
        pass->Execute(ctx);
    }

    RenderSprites(registry, camera);
    RenderDebug(registry, settings, camera, options);
    if (options.ShowEditorIcons)
    {
        RenderEditorIcons(registry, settings, camera);
    }

    Renderer::EndScene();


    Profiler::UpdateStats(m_CurrentStats);
}

void SceneRenderer::RenderSprites(entt::registry& registry, const Camera3D& camera)
{
    CH_PROFILE_FUNCTION();
    auto view = registry.view<TransformComponent, SpriteComponent>();

    struct SpriteEntry
    {
        entt::entity Entity;
        int ZOrder;
    };

    std::vector<SpriteEntry> sortedSprites;
    for (auto entity : view)
    {
        sortedSprites.push_back(SpriteEntry{entity, registry.get<SpriteComponent>(entity).ZOrder});
    }

    std::sort(sortedSprites.begin(), sortedSprites.end(), [](const SpriteEntry& a, const SpriteEntry& b) {
        if (a.ZOrder != b.ZOrder)
        {
            return a.ZOrder < b.ZOrder;
        }
        return a.Entity < b.Entity;
    });

    for (const auto& entry : sortedSprites)
    {
        auto& transform = registry.get<TransformComponent>(entry.Entity);
        auto& sprite = registry.get<SpriteComponent>(entry.Entity);

        if (sprite.TexturePath.empty())
        {
            continue;
        }

        auto handle = AssetManager::Get().ImportAsset(sprite.TexturePath);
        auto textureAsset = AssetManager::Get().GetAsset<TextureAsset>(handle);
        if (textureAsset && textureAsset->IsReady() && textureAsset->GetTexture())
        {
            Renderer2D::DrawSprite(textureAsset->GetTexture()->GetRendererID(), transform.WorldTransform, ColorToVec4(sprite.Tint), sprite.FlipX, sprite.FlipY);
        }
    }
}


void SceneRenderer::PrepareLights(entt::registry& registry, const Frustum& frustum)
{
    for (auto& l : m_Lighting.Lights) {
        l.enabled = 0;
    }
    m_Lighting.LightCount = 0;
    m_Lighting.LightsDirty = true;
    
    int lightCount = 0;
    auto view = registry.view<LightComponent>();
    for (auto entity : view)
    {
        auto& light = view.get<LightComponent>(entity);
        glm::vec3 worldPos = Entity(entity, &registry).GetWorldPosition();
        if (light.Type != LightType::Directional && !IsSphereVisible(frustum, worldPos, light.Radius))
        {
            continue;
        }

        RenderLight rl;
        rl.position = worldPos;
        rl.color = ColorToVec4(light.LightColor);
        rl.intensity = light.Intensity;
        rl.radius = light.Radius;
        rl.type = (int)light.Type;
        rl.direction = Entity(entity, &registry).GetForward();
        rl.innerCutoff = light.InnerCutoff;
        rl.outerCutoff = light.OuterCutoff;
        rl.enabled = 1;

        if (lightCount >= LightingData::MaxLights) break;
        
        m_Lighting.Lights[lightCount++] = rl;
        m_Lighting.LightsDirty = true;
    }
    m_Lighting.LightCount = lightCount;
}

void SceneRenderer::CollectAndRenderItems(entt::registry& registry, const Frustum& frustum, const glm::vec3& cameraPos)
{
    auto meshView = registry.view<TransformComponent, ModelComponent>();
    auto* assets = &AssetManager::Get();

    for (auto entity : meshView)
    {
        auto [transform, mesh] = meshView.get<TransformComponent, ModelComponent>(entity);
        if (mesh.ModelPath.empty())
        {
            continue;
        }

        auto handle = assets->ImportAsset(mesh.ModelPath);
        auto modelAsset = assets->GetAsset<ModelAsset>(handle);
        if (!modelAsset)
        {
            continue;
        }

        // Logic handled by AssetResolutionSystem

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

        // Transform AABB center+extents to world space for frustum culling
        glm::vec3 localCenter = (bbox.Max + bbox.Min) * 0.5f;
        glm::vec3 localExtents = (bbox.Max - bbox.Min) * 0.5f;
        
        const glm::mat4& worldTransform = transform.WorldTransform;
        glm::vec3 worldCenter = glm::vec3(worldTransform * glm::vec4(localCenter, 1.0f));
        glm::vec3 worldExtents = {
            std::abs(worldTransform[0][0]) * localExtents.x +
                std::abs(worldTransform[1][0]) * localExtents.y +
                std::abs(worldTransform[2][0]) * localExtents.z,
            std::abs(worldTransform[0][1]) * localExtents.x +
                std::abs(worldTransform[1][1]) * localExtents.y +
                std::abs(worldTransform[2][1]) * localExtents.z,
            std::abs(worldTransform[0][2]) * localExtents.x +
                std::abs(worldTransform[1][2]) * localExtents.y +
                std::abs(worldTransform[2][2]) * localExtents.z,
        };
        if (!IsBoxVisible(frustum, worldCenter, worldExtents))
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
                auto handle = assets ? assets->ImportAsset(sc.ShaderPath) : AssetHandle(0);
                shaderOver = assets ? assets->GetAsset<ShaderAsset>(handle) : nullptr;
                uniforms = sc.Uniforms;
            }
        }

        std::vector<Material> materials;
        if (registry.all_of<ModelComponent>(entity)) // Default from asset if not overridden
        {
            materials = modelAsset->GetMaterials();
        }

        RenderItem item;
        item.Asset = modelAsset.get();
        item.Transform = transform.WorldTransform;
        item.Materials = materials;
        item.ShaderOverride = shaderOver.get();
        item.CustomUniforms = uniforms;

        if (registry.all_of<AnimationComponent>(entity))
        {
            auto& anim = registry.get<AnimationComponent>(entity);
            if (anim.CurrentAnimationIndex >= 0)
            {
                item.BoneMatrices = modelAsset->GetBoneMatrices(anim.CurrentAnimationIndex, anim.CurrentFrame);
            }
        }

        // Determine transparency and push to correct queue
        bool hasOpaque = false;
        bool hasTransparent = false;
        const auto& model = modelAsset->GetModel();
        for (int i = 0; i < (int)model.Meshes.size(); ++i)
        {
            Material mat = ResolveMaterialForMesh(i, model, materials, modelAsset.get());
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

        if (hasTransparent)
        {
            m_TransparentQueue.push_back(item);
        }
        if (hasOpaque)
        {
            m_OpaqueQueue.push_back(std::move(item));
        }
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
                Model m = GeometryGenerator::GenerateProceduralModel(paths[idx], p);
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
        const BoundingBox primBBox = primitive.Asset->GetBoundingBox();
        glm::vec3 primCenter = (primBBox.Max + primBBox.Min) * 0.5f;
        glm::vec3 primExtents = (primBBox.Max - primBBox.Min) * 0.5f;
        // Get from frustum file
        if (!IsBoxVisible(frustum, primCenter, primExtents))
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
                auto handle = assets ? assets->ImportAsset(sc.ShaderPath) : AssetHandle(0);
                shaderOver = assets ? assets->GetAsset<ShaderAsset>(handle) : nullptr;
                uniforms = sc.Uniforms;
            }
        }
        RenderItem item;
        item.Asset = primitive.Asset.get();
        item.Transform = transform.WorldTransform;
        item.Materials = primitive.Asset->GetMaterials();
        item.ShaderOverride = shaderOver.get();
        item.CustomUniforms = uniforms;

        // Primitives are usually opaque, but let's check
        bool hasOpaque = false;
        bool hasTransparent = false;
        const auto& model = primitive.Asset->GetModel();
        for (int i = 0; i < (int)model.Meshes.size(); ++i)
        {
            Material mat = ResolveMaterialForMesh(i, model, {}, primitive.Asset.get());
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

        if (hasTransparent)
        {
            m_TransparentQueue.push_back(item);
        }
        if (hasOpaque)
        {
            m_OpaqueQueue.push_back(std::move(item));
        }
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries)
{
    for (const auto& entry : animatedEntries)
    {
        std::vector<glm::mat4> boneMatrices;
        if (entry.Animation.CurrentAnimationIndex >= 0 && entry.Asset)
        {
            boneMatrices =
                entry.Asset->GetBoneMatrices(entry.Animation.CurrentAnimationIndex, entry.Animation.CurrentFrame);
        }
        DrawModel(entry.Asset, entry.WorldTransform, boneMatrices, {}, entry.ShaderOverride,
                  entry.CustomUniforms);
    }
}

void SceneRenderer::DrawModel(Chained::ModelAsset* modelAsset, const glm::mat4& transform,
                               const std::vector<glm::mat4>& boneMatrices,
                               const std::vector<Chained::Material>& materials,
                               Chained::ShaderAsset* shaderOverride,
                               const std::vector<Chained::ShaderUniform>& shaderUniformOverrides,
                               Chained::RenderPassStage pass)
{
    if (!modelAsset || modelAsset->GetState() != Chained::AssetState::Ready)
    {
        return;
    }

        auto& model = modelAsset->GetModel();
        auto activeShader = shaderOverride ? shaderOverride
                            : (Renderer::GetShaderLibrary().Exists("Lighting")
                                ? Renderer::GetShaderLibrary().Get("Lighting").get()
                                : nullptr);

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
            CH_CORE_INFO("[SceneRenderer] Applying shader override: '{}' to model: '{}'", shaderOverride->GetPath(),
                         modelAsset->GetPath());
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

        Material material = ResolveMaterialForMesh(i, model, materials, modelAsset);

        bool isTransparent = material.Transparent || material.AlbedoColor.a < 0.99f;
        if (pass == RenderPassStage::Opaque && isTransparent)
        {
            continue;
        }
        if (pass == RenderPassStage::Transparent && !isTransparent)
        {
            continue;
        }

        BindShaderUniforms(activeShader, boneMatrices, shaderUniformOverrides);
        BindMaterialUniforms(activeShader, material, i, model);

        uint32_t originalID = material.ShaderID;
        material.ShaderID = activeShader->GetShader()->GetRendererID();

        bool useSkinning = !boneMatrices.empty();
        activeShader->GetShader()->Bind();
        activeShader->GetShader()->SetInt("useSkinning", useSkinning ? 1 : 0);

        Renderer3D::DrawMesh(model.Meshes[i], material, useSkinning ? transform : transform * inst.localTransform);
        material.ShaderID = originalID;
    }
}

Chained::Material SceneRenderer::ResolveMaterialForMesh(int meshIndex, const Chained::Model& model,
                                               const std::vector<Chained::Material>& materials,
                                               Chained::ModelAsset* modelAsset)
{
    if (meshIndex < 0 || meshIndex >= (int)model.Meshes.size())
    {
        return {};
    }

    // Use overrides if available
    if (meshIndex < (int)materials.size())
    {
        return materials[meshIndex];
    }

    int matIdx = model.Meshes[meshIndex].MaterialIndex;
    if (matIdx < 0 || matIdx >= (int)model.Materials.size())
    {
        return Material();
    }

    Material material = model.Materials[matIdx];
    return material;
}

void SceneRenderer::BindShaderUniforms(Chained::ShaderAsset* shaderAsset, const std::vector<glm::mat4>& boneMatrices,
                                        const std::vector<Chained::ShaderUniform>& shaderUniformOverrides)
{
    if (!shaderAsset || !shaderAsset->GetShader())
    {
        return;
    }

    auto shader = shaderAsset->GetShader();
    shader->Bind();

    auto& rd = Renderer::GetData();
    const auto& lighting = m_Lighting.CurrentLighting;

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
    shader->SetInt("uLightCount", m_Lighting.LightCount);
    shader->SetFloat("uExposure", lighting.Exposure);
    shader->SetFloat("uGamma", lighting.Gamma);

    // Apply Fog
    const auto& fog = m_Lighting.CurrentFog;
    int fogEnabled = fog.Enabled ? 1 : 0;
    glm::vec4 fogColor = {fog.FogColor.r / 255.0f, fog.FogColor.g / 255.0f, fog.FogColor.b / 255.0f, fog.FogColor.a / 255.0f};
    shader->SetInt("fogEnabled", fogEnabled);
    shader->SetVec4("fogColor", fogColor);
    shader->SetFloat("fogDensity", fog.Density);
    shader->SetFloat("fogStart", fog.Start);
    shader->SetFloat("fogEnd", fog.End);
    shader->SetInt("fogMode", (int)fog.Mode);

    if (!boneMatrices.empty())
    {
        shader->SetMatrices("boneMatrices", boneMatrices.data(), std::min((int)boneMatrices.size(), 128));
    }
    for (const auto& u : shaderUniformOverrides)
    {
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
                                         const Model& model)
{
    auto shader = shaderAsset->GetShader();
    shader->Bind();

    auto resolveMap = [this](uint32_t currentId, AssetHandle handle) -> uint32_t {
        if (currentId > 0)
        {
            return currentId;
        }
        auto tex = AssetManager::Get().GetAsset<TextureAsset>(handle);
        if (tex && tex->IsReady() && tex->GetTexture())
        {
            return tex->GetTexture()->GetRendererID();
        }
        return 0;
    };

    uint32_t albedoMap = resolveMap(material.AlbedoMap, material.AlbedoHandle);
    uint32_t normalMap = resolveMap(material.NormalMap, material.NormalHandle);
    uint32_t metallicMap = resolveMap(material.MetallicRoughnessMap, material.MetallicRoughnessHandle);
    uint32_t emissiveMap = resolveMap(material.EmissiveMap, material.EmissiveHandle);
    uint32_t occlusionMap = resolveMap(material.OcclusionMap, material.OcclusionHandle);

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
    if (emissiveMap > 0)
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

void SceneRenderer::RenderDebug(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera,
                                const SceneRenderOptions& options)
{
    if (!options.ShowDebugColliders && !options.ShowDebugCollisionModelBox && !options.ShowDebugSpawnZones &&
        !options.DrawGrid)
    {
        return;
    }

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



    if (options.ShowDebugColliders)
    {
        DrawColliderDebug(registry, options);
    }

    if (options.ShowDebugCollisionModelBox)
    {
        DrawCollisionModelBoxDebug(registry);
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
    bool drawWire = (mode == 0 || mode == 2);

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
                    DebugRenderer::DrawCubeWires(baseTransform, collider.Size * entityScale, color, isWireframe);
                }
                else if (collider.Type == ColliderType::Sphere)
                {
                    // For sphere, we use the maximum component of the entity scale for the overall radius multiplier
                    float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                    DebugRenderer::DrawSphereWires(baseTransform, collider.Radius * maxScale, color, isWireframe);
                }
                else if (collider.Type == ColliderType::Capsule)
                {
                    float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                    DebugRenderer::DrawCapsuleWires(
                        baseTransform, collider.Radius * maxScale, collider.Height * entityScale.y, color, isWireframe);
                }
            }
            else if (collider.Type == ColliderType::Mesh)
            {
                AssetHandle modelHandle = collider.ModelHandle;

                // Fallback to visual mesh if AutoCalculate is enable and handle is 0
                if (modelHandle == 0 && collider.AutoCalculate && registry.all_of<ModelComponent>(entity))
                {
                    auto& mc = registry.get<ModelComponent>(entity);
                    auto handle = AssetManager::Get().ImportAsset(mc.ModelPath);
                    auto asset = AssetManager::Get().GetAsset<ModelAsset>(handle);
                    if (asset)
                    {
                        modelHandle = asset->GetID();
                    }
                }

                if (modelHandle != 0)
                {
                    auto modelAsset = AssetManager::Get().GetAsset<ModelAsset>(modelHandle);
                    if (modelAsset && modelAsset->GetState() == AssetState::Ready)
                    {
                        const auto& model = modelAsset->GetModel();
                        const auto& instances = modelAsset->GetInstances();

                        for (const auto& inst : instances)
                        {
                            if (inst.meshIndex >= 0 && inst.meshIndex < (int)model.Meshes.size())
                            {
                                DebugRenderer::DrawMeshWire(
                                    model.Meshes[inst.meshIndex], color, transform.WorldTransform * inst.localTransform,
                                    isWireframe);
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
            auto model = AssetManager::Get().GetAsset<ModelAsset>(collider.ModelHandle);
            if (model && model->IsReady())
            {
                const BoundingBox& localBox = model->GetBoundingBox();
                glm::vec3 center = (localBox.Min + localBox.Max) * 0.5f;
                glm::vec3 size = localBox.Max - localBox.Min;

                // Note: transform.WorldTransform already includes entity scale,
                // but the localBox is also scaled if the importer applied scale?
                // Usually localBox is untransformed.
                DebugRenderer::DrawCubeWires(
                    transform.WorldTransform * glm::translate(glm::mat4(1.0f), center), size, color);
            }
        }
    }
}


void SceneRenderer::RenderEditorIcons(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera)
{
    CH_PROFILE_FUNCTION();
    
    // We can use a simple constant set of icons or load them from the engine resources
    // For now, let's just draw some billboards
    
    auto lightView = registry.view<LightComponent, TransformComponent>();
    for (auto entity : lightView)
    {
        auto [light, transform] = lightView.get<LightComponent, TransformComponent>(entity);
        Renderer2D::DrawBillboard(camera, 0, transform.WorldTransform[3], 0.5f, ColorToVec4(light.LightColor));
    }
    
    auto cameraView = registry.view<CameraComponent, TransformComponent>();
    for (auto entity : cameraView)
    {
        auto [cam, transform] = cameraView.get<CameraComponent, TransformComponent>(entity);
        Renderer2D::DrawBillboard(camera, 0, transform.WorldTransform[3], 0.5f, glm::vec4(0.2f, 0.5f, 1.0f, 1.0f));
    }
}


} // namespace Chained
