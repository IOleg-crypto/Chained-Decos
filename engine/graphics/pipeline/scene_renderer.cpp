#include "scene_renderer.h"
#include "engine/assets/asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "imgui.h"
#include "engine/core/platform.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "engine/graphics/pipeline/passes/composite_pass.h"
#include "engine/graphics/pipeline/passes/geometry_pass.h"
#include "engine/graphics/pipeline/passes/shadow_pass.h"
#include "engine/graphics/pipeline/passes/skybox_pass.h"

namespace Chained
{

// --- Obsolete local utilities removed ---

static glm::vec4 ColorToVec4(const Color& c)
{
    return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
}

SceneRenderer::SceneRenderer()
{
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

            activeCamera.Projection = camera.Camera.GetProjectionType();

            activeCamera.FovDegrees = glm::degrees(camera.Camera.GetPerspectiveVerticalFOV());
            activeCamera.OrthographicSize = camera.Camera.GetOrthographicSize();
            activeCamera.NearClip = (camera.Camera.GetProjectionType() == Camera::ProjectionType::Perspective)
                ? camera.Camera.GetPerspectiveNearClip()
                : camera.Camera.GetOrthographicNearClip();
            activeCamera.FarClip = (camera.Camera.GetProjectionType() == Camera::ProjectionType::Perspective)
                ? camera.Camera.GetPerspectiveFarClip()
                : camera.Camera.GetOrthographicFarClip();

            // Compute ViewMatrix
            activeCamera.ViewMatrix = glm::lookAt(activeCamera.Position, activeCamera.Target, activeCamera.Up);

            // Compute ProjectionMatrix using viewport aspect ratio
            auto* renderer = ServiceLocator::TryGet<Renderer>();
            float aspect = 1.0f;
            if (renderer)
            {
                uint32_t vw = renderer->GetViewportWidth();
                uint32_t vh = renderer->GetViewportHeight();
                if (vh > 0) aspect = static_cast<float>(vw) / static_cast<float>(vh);
            }
            if (activeCamera.Projection == ProjectionType::Perspective)
            {
                activeCamera.ProjectionMatrix = glm::perspective(
                    glm::radians(activeCamera.FovDegrees), aspect, activeCamera.NearClip, activeCamera.FarClip);
            }
            else
            {
                float orthoSize = activeCamera.OrthographicSize;
                activeCamera.ProjectionMatrix = glm::ortho(
                    -aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize,
                    activeCamera.NearClip, activeCamera.FarClip);
            }

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

    GraphicsDevice::Get().EnableDepthTest();

    auto environment = options.EnvironmentOverride ? options.EnvironmentOverride : settings.Environment;

    if (environment)
    {
        const auto& envSettings = environment->GetSettings();
        auto& rendererLighting = ServiceLocator::Get<Renderer>()->GetData().Lighting;
        rendererLighting.CurrentLighting = envSettings.Lighting;
        rendererLighting.CurrentFog = envSettings.Fog;
        m_CurrentEnv = envSettings;
    }

    ServiceLocator::Get<Renderer>()->UpdateTime(Timestep(Platform::GetTime()));

    m_CurrentStats = {};
    m_CurrentStats.EntityCount = (uint32_t)registry.storage<entt::entity>().size();

    glm::mat4 view = camera.ViewMatrix;
    glm::mat4 proj = camera.ProjectionMatrix;
    Frustum frustum = FromMatrix(proj * view);

    // PrepareLights BEFORE BeginScene so the SSBO contains current-frame data
    // (BeginScene uploads the SSBO to the GPU).
    PrepareLights(registry, frustum);

    ServiceLocator::Get<Renderer>()->BeginScene(camera, nearClip, farClip);

    RenderContext ctx{registry, settings, camera, options, nearClip, farClip, this};

    m_OpaqueQueue.clear();
    m_TransparentQueue.clear();

    CollectAndRenderItems(registry, frustum, camera.Position);

    // Sort transparent queue back-to-front once for all passes
    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(),
              [](const auto& a, const auto& b) { return a.Distance > b.Distance; });

    for (auto& pass : m_RenderPasses)
    {
        pass->Execute(ctx);

        // After ShadowPass: propagate shadow state to the Renderer so geometry shaders can sample the shadow map
        if (pass->GetName() == "ShadowPass")
        {
            auto* shadowPass = static_cast<ShadowPass*>(pass.get());
            auto& rendererData = ServiceLocator::Get<Renderer>()->GetData();
            rendererData.ShadowsEnabled = shadowPass->HasShadows();
            rendererData.LightSpaceMatrix = shadowPass->GetLightSpaceMatrix();
            if (shadowPass->HasShadows() && shadowPass->GetShadowMap())
            {
                rendererData.ShadowMapTextureID = shadowPass->GetShadowMap()->GetDepthAttachmentRendererID();
            }
            else
            {
                rendererData.ShadowMapTextureID = 0;
            }
        }
    }

    RenderSprites(registry, camera);
    RenderDebug(registry, settings, camera, options);

    ServiceLocator::Get<Renderer>()->EndScene();

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

        auto handle = ServiceLocator::Get<AssetManager>()->LoadAsset(sprite.TexturePath, TextureAsset::GetStaticType());
        auto textureAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>(sprite.TexturePath);
        if (textureAsset && textureAsset->IsReady() && textureAsset->GetTexture())
        {
            ServiceLocator::Get<Renderer>()->DrawSprite(textureAsset->GetTexture()->GetNativeHandle(),
                                                        transform.WorldTransform, ColorToVec4(sprite.Tint),
                                                        sprite.FlipX, sprite.FlipY);
        }
    }
}

void SceneRenderer::PrepareLights(entt::registry& registry, const Frustum& frustum)
{
    auto& lighting = ServiceLocator::Get<Renderer>()->GetData().Lighting;

    for (auto& l : lighting.Lights)
    {
        l.enabled = 0;
    }
    lighting.LightCount = 0;
    lighting.LightsDirty = true;

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

        if (lightCount >= LightingData::MaxLights)
        {
            break;
        }

        lighting.Lights[lightCount++] = rl;
        lighting.LightsDirty = true;
    }
    lighting.LightCount = lightCount;
}

void SceneRenderer::CollectAndRenderItems(entt::registry& registry, const Frustum& frustum, const glm::vec3& cameraPos)
{
    auto meshView = registry.view<TransformComponent, ModelComponent>();
    auto* assets = ServiceLocator::Get<AssetManager>();

    // --- Debug counters (one per RenderScene call) ---
    int dbg_total = 0, dbg_emptyPath = 0, dbg_nullAsset = 0, dbg_notReady = 0, dbg_culled = 0, dbg_queued = 0;

    for (auto entity : meshView)
    {
        auto [transform, mesh] = meshView.get<TransformComponent, ModelComponent>(entity);
        dbg_total++;
        if (mesh.ModelPath.empty())
        {
            dbg_emptyPath++;
            continue;
        }

        auto handle = assets->LoadAsset(mesh.ModelPath, ModelAsset::GetStaticType());
        auto modelAsset = assets->Get<ModelAsset>(mesh.ModelPath);
        if (!modelAsset)
        {
            dbg_nullAsset++;
            continue;
        }

        // Logic handled by AssetResolutionSystem

        AssetState state = modelAsset->GetState();
        if (state != AssetState::Ready)
        {
            dbg_notReady++;
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
            std::abs(worldTransform[0][0]) * localExtents.x + std::abs(worldTransform[1][0]) * localExtents.y +
                std::abs(worldTransform[2][0]) * localExtents.z,
            std::abs(worldTransform[0][1]) * localExtents.x + std::abs(worldTransform[1][1]) * localExtents.y +
                std::abs(worldTransform[2][1]) * localExtents.z,
            std::abs(worldTransform[0][2]) * localExtents.x + std::abs(worldTransform[1][2]) * localExtents.y +
                std::abs(worldTransform[2][2]) * localExtents.z,
        };
        if (!IsBoxVisible(frustum, worldCenter, worldExtents))
        {
            dbg_culled++;
            continue;
        }

        std::shared_ptr<ShaderAsset> shaderOver;
        std::vector<ShaderUniform> uniforms;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& sc = registry.get<ShaderComponent>(entity);
            if (sc.Enabled && !sc.ShaderPath.empty())
            {
                auto handle = assets->LoadAsset(sc.ShaderPath, ShaderAsset::GetStaticType());
                shaderOver = assets->Get<ShaderAsset>(sc.ShaderPath);
                uniforms = sc.Uniforms;
            }
        }

        std::vector<Material> materials = mesh.Materials;

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
            dbg_queued++;
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
        glm::vec3 primLocalCenter = (primBBox.Max + primBBox.Min) * 0.5f;
        glm::vec3 primLocalExtents = (primBBox.Max - primBBox.Min) * 0.5f;

        // Transform AABB center+extents to world space for frustum culling
        const glm::mat4& primWorldTransform = transform.WorldTransform;
        glm::vec3 primCenter = glm::vec3(primWorldTransform * glm::vec4(primLocalCenter, 1.0f));
        glm::vec3 primExtents = {
            std::abs(primWorldTransform[0][0]) * primLocalExtents.x + std::abs(primWorldTransform[1][0]) * primLocalExtents.y +
                std::abs(primWorldTransform[2][0]) * primLocalExtents.z,
            std::abs(primWorldTransform[0][1]) * primLocalExtents.x + std::abs(primWorldTransform[1][1]) * primLocalExtents.y +
                std::abs(primWorldTransform[2][1]) * primLocalExtents.z,
            std::abs(primWorldTransform[0][2]) * primLocalExtents.x + std::abs(primWorldTransform[1][2]) * primLocalExtents.y +
                std::abs(primWorldTransform[2][2]) * primLocalExtents.z,
        };
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
                auto handle =
                    ServiceLocator::Get<AssetManager>()->LoadAsset(sc.ShaderPath, ShaderAsset::GetStaticType());
                shaderOver = ServiceLocator::Get<AssetManager>()->Get<ShaderAsset>(sc.ShaderPath);
                uniforms = sc.Uniforms;
            }
        }
        std::vector<Material> materials;

        RenderItem item;
        item.Asset = primitive.Asset.get();
        item.Transform = transform.WorldTransform;
        item.Materials = materials;
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
        DrawModel(entry.Asset, entry.WorldTransform, boneMatrices, {}, entry.ShaderOverride, entry.CustomUniforms);
    }
}

void SceneRenderer::DrawModel(Chained::ModelAsset* modelAsset, const glm::mat4& transform,
                              const std::vector<glm::mat4>& boneMatrices,
                              const std::vector<Chained::Material>& materials, Chained::ShaderAsset* shaderOverride,
                              const std::vector<Chained::ShaderUniform>& shaderUniformOverrides,
                              Chained::RenderPassStage pass)
{
    if (!modelAsset || modelAsset->GetState() != Chained::AssetState::Ready)
    {
        return;
    }

    auto& model = modelAsset->GetModel();
    std::string fallbackName = boneMatrices.empty() ? "Lighting" : "Skinned";
    auto activeShader = shaderOverride
                            ? shaderOverride
                            : (ServiceLocator::Get<Renderer>()->GetShaderLibrary().Exists(fallbackName)
                                   ? ServiceLocator::Get<Renderer>()->GetShaderLibrary().Get(fallbackName).get()
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
        material.ShaderID = activeShader->GetShader()->GetNativeHandle();

        activeShader->GetShader()->Bind();
        ServiceLocator::Get<Renderer>()->DrawMesh(model.Meshes[i], material,
                                                  transform * inst.localTransform);
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

    int matIdx = model.Meshes[meshIndex].MaterialIndex;

    if (matIdx >= 0 && matIdx < (int)materials.size())
    {
        return materials[matIdx];
    }

    if (modelAsset && modelAsset->IsReady())
    {
        const auto& assetMaterials = modelAsset->GetMaterials();
        if (matIdx >= 0 && matIdx < (int)assetMaterials.size())
        {
            return assetMaterials[matIdx];
        }
    }

    if (matIdx < 0 || matIdx >= (int)model.Materials.size())
    {
        return Material();
    }

    return model.Materials[matIdx];
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

    // Use shared lighting uniforms from Renderer
    ServiceLocator::Get<Renderer>()->SetLightingUniforms(shaderAsset);

    if (!boneMatrices.empty())
    {
        shader->SetMatrices("boneMatrices", boneMatrices.data(), std::min((int)boneMatrices.size(), 128));
    }

    
    for (const auto& u : shaderUniformOverrides)
    {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, float>)
                {
                    shader->SetFloat(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, glm::vec2>)
                {
                    shader->SetVec2(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, glm::vec3>)
                {
                    shader->SetVec3(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, glm::vec4>)
                {
                    shader->SetVec4(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, Chained::Color>)
                {
                    
                    glm::vec4 colorVec = {arg.r / 255.0f, arg.g / 255.0f, arg.b / 255.0f, arg.a / 255.0f};
                    shader->SetVec4(u.Name, colorVec);
                }
            },
            u.Value);
    }
}
void SceneRenderer::BindMaterialUniforms(ShaderAsset* shaderAsset, const Material& material, int meshIndex,
                                         const Model& model)
{
    auto shader = shaderAsset->GetShader();
    shader->Bind();

    auto resolveMap = [](const std::shared_ptr<Texture>& currentTex, const std::string& path) -> uint32_t {
        if (currentTex)
        {
            return currentTex->GetNativeHandle();
        }
        if (path.empty() || path.front() == '*')
        {
            return 0;
        }

        auto texAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>(path);
        if (texAsset && texAsset->GetTexture())
        {
            return texAsset->GetTexture()->GetNativeHandle();
        }
        return 0;
    };

    uint32_t albedoMap = resolveMap(material.AlbedoMap, material.AlbedoPath);
    uint32_t normalMap = resolveMap(material.NormalMap, material.NormalPath);
    uint32_t metallicMap = resolveMap(material.MetallicRoughnessMap, material.MetallicRoughnessPath);
    uint32_t emissiveMap = resolveMap(material.EmissiveMap, material.EmissivePath);
    uint32_t occlusionMap = resolveMap(material.OcclusionMap, material.OcclusionPath);

    // 1. Albedo (Texture Unit 0)
    if (albedoMap > 0)
    {
        GraphicsDevice::Get().SetTexture(0, albedoMap);
        shader->SetInt("texture0", 0);
        shader->SetInt("useTexture", 1);
    }
    else
    {
        shader->SetInt("useTexture", 0);
    }
    shader->SetVec4("colDiffuse", material.AlbedoColor);

    // 2. Metallic-Roughness Packed Map (Texture Unit 1)

    if (metallicMap > 0)
    {
        GraphicsDevice::Get().SetTexture(1, metallicMap);
        shader->SetInt("texture1", 1);
        shader->SetInt("useMetallicMap", 1);
        shader->SetInt("useRoughnessMap", 1);
    }
    else
    {
        shader->SetInt("useMetallicMap", 0);
        shader->SetInt("useRoughnessMap", 0);
    }

    // 3. Normal Map (Texture Unit 2)
    if (normalMap > 0)
    {
        GraphicsDevice::Get().SetTexture(2, normalMap);
        shader->SetInt("texture2", 2);
        shader->SetInt("useNormalMap", 1);
    }
    else
    {
        shader->SetInt("useNormalMap", 0);
    }

    // 4. Occlusion Map (Texture Unit 4)
    if (occlusionMap > 0)
    {
        GraphicsDevice::Get().SetTexture(4, occlusionMap);
        shader->SetInt("texture4", 4);
        shader->SetInt("useOcclusionMap", 1);
    }
    else
    {
        shader->SetInt("useOcclusionMap", 0);
    }

    // 5. Emissive Map (Texture Unit 5)
    if (emissiveMap > 0)
    {
        GraphicsDevice::Get().SetTexture(5, emissiveMap);
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
    auto guard = PipelineStateGuard::Capture();
    guard.WithDepthTest().WithBlend().WithPolygonMode();

    // Setup for debug drawing
    GraphicsDevice::Get().DisableDepthTest();
    GraphicsDevice::Get().SetBlendEnabled(true);
    GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);

    // Polygon offset no longer needed since depth test is OFF
    GraphicsDevice::Get().SetPolygonOffset(false, 0.0f, 0.0f);

    if (options.SetCollisionWireframeMode == 1)
    {
        GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Line);
    }
    else
    {
        GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Fill);
    }

    if (options.ShowDebugColliders)
    {
        DrawColliderDebug(registry, options);
    }

    if (options.ShowDebugCollisionModelBox)
    {
        DrawCollisionModelBoxDebug(registry);
    }

    if (options.DrawGrid)
    {
        auto& grid = settings.Grid;
        DebugRenderer::DrawInfiniteGrid(camera, grid.Spacing, {0.5f, 0.5f, 0.5f, 1.0f});
    }

    DebugRenderer::Flush();
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

            glm::vec4 color =
                collider.IsColliding ? glm::vec4(1.0f, 0.0f, 0.0f, 0.6f) : glm::vec4(0.0f, 1.0f, 0.0f, 0.6f);
            if (isWireframe)
            {
                color.a = 1.0f;
            }

            if (collider.Type == ColliderType::Box || collider.Type == ColliderType::Sphere ||
                collider.Type == ColliderType::Capsule)
            {

                glm::vec3 entityScale(glm::length(glm::vec3(transform.WorldTransform[0])),
                                      glm::length(glm::vec3(transform.WorldTransform[1])),
                                      glm::length(glm::vec3(transform.WorldTransform[2])));

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
                    float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                    DebugRenderer::DrawSphereWires(baseTransform, collider.Radius * maxScale, color, isWireframe);
                }
                else if (collider.Type == ColliderType::Capsule)
                {

                    float radiusScale = glm::max(entityScale.x, entityScale.z);
                    DebugRenderer::DrawCapsuleWires(baseTransform, collider.Radius * radiusScale,
                                                    collider.Height * entityScale.y, color, isWireframe);
                }
            }
            else if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
            {
                auto modelAsset = ServiceLocator::Get<AssetManager>()->Get<ModelAsset>(collider.ModelPath);
                if (modelAsset && modelAsset->IsReady())
                {
                    glm::mat4 meshTrans = transform.WorldTransform * glm::translate(glm::mat4(1.0f), collider.Offset);
                    const auto& model = modelAsset->GetModel();
                    for (const auto& inst : modelAsset->GetInstances())
                    {
                        glm::mat4 finalMat = meshTrans * inst.localTransform;
                        if (inst.meshIndex >= 0 && inst.meshIndex < model.Meshes.size())
                        {
                            DebugRenderer::DrawMeshWire(model.Meshes[inst.meshIndex], color, finalMat, isWireframe);
                        }
                    }
                }
            }
        }
    };

    if (drawSolid)
    {
        GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Fill);
        drawPass(false);
    }

    if (drawWire)
    {
        GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Line);
        drawPass(true);
    }

    GraphicsDevice::Get().SetPolygonMode(GraphicsDevice::PolygonMode::Fill);
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
            auto model = ServiceLocator::Get<AssetManager>()->Get<ModelAsset>(collider.ModelHandle);
            if (model && model->IsReady())
            {
                const BoundingBox& localBox = model->GetBoundingBox();
                glm::vec3 center = (localBox.Min + localBox.Max) * 0.5f;
                glm::vec3 size = localBox.Max - localBox.Min;

                // Note: transform.WorldTransform already includes entity scale,
                // but the localBox is also scaled if the importer applied scale?
                // Usually localBox is untransformed.
                DebugRenderer::DrawCubeWires(transform.WorldTransform * glm::translate(glm::mat4(1.0f), center), size,
                                             color);
            }
        }
    }
}

} // namespace Chained
