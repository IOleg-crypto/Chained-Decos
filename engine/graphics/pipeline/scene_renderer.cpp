#include "scene_renderer.h"
#include "engine/audio/audio.h"
#include "engine/core/application.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/ch_assert.h"
#include "engine/core/profiler.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/loaders/model_loader.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/texture_utility.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/light_component.h"
#include "engine/scene/components/mesh_component.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

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

void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                const SceneRenderOptions& options)
{
    CH_PROFILE_FUNCTION();
    if (!Renderer::IsInitialized() || !scene)
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);

    auto environment = scene->GetSettings().Environment;
    if (!environment) environment = options.EnvironmentOverride;

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
                auto texture = AssetManager::Get().Get<TextureAsset>(settings.TexturePath);
                if (texture && texture->GetState() == AssetState::Ready)
                {
                    int skyboxMode = std::clamp(settings.Mode, 0, 2);
                    uint32_t texId = texture->GetTexture()->GetRendererID();
                    auto& renderer = Renderer::Get();
                    auto& rd = renderer.GetData();

                    if (skyboxMode == 2)
                    {
                        if (!rd.Skybox.CachedCubemap || rd.Skybox.CachedCubemapPath != settings.TexturePath)
                        {
                            auto genShader = rd.Shaders->Get("CubemapGen");
                            if (genShader && rd.Skybox.SkyboxCubeModel && !rd.Skybox.SkyboxCubeModel->Meshes.empty())
                            {
                                rd.Skybox.CachedCubemap = TextureUtility::GenTextureCubemap(
                                    genShader->GetShader()->GetRendererID(), texId, 1024, rd.Skybox.SkyboxCubeModel->Meshes[0]);
                                rd.Skybox.CachedCubemapPath = settings.TexturePath;
                                rd.Skybox.SourceTextureId = texId;
                            }
                        }
                        if (rd.Skybox.CachedCubemap) texId = rd.Skybox.CachedCubemap->GetRendererID();
                    }
                    renderer.DrawSkybox(texId, skyboxMode, texture->IsHDR(), settings.Exposure, settings.Brightness, settings.Contrast, camera);
                }
            }
        }

        RenderModels(scene, camera, nearClip, farClip, timestep, options);
        RenderDebug(scene, camera, options);
        RenderEditorIcons(scene, camera);
    }
    Renderer::Get().EndScene();

    Profiler::UpdateStats(m_CurrentStats);
}

void SceneRenderer::RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                 const SceneRenderOptions& options)
{
    auto& registry = scene->GetRegistry();
    Frustum frustum;
    {
        float w = (float)Application::Get().GetWindow().GetWidth();
        float h = (float)Application::Get().GetWindow().GetHeight();
        float aspect = (w > 0 && h > 0) ? w / h : 1.0f;
        glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
        glm::mat4 proj = (camera.Projection == 0)
            ? glm::perspective(glm::radians(camera.Fovy), aspect, nearClip, farClip)
            : glm::ortho(-aspect * camera.Fovy, aspect * camera.Fovy, -camera.Fovy, camera.Fovy, nearClip, farClip);
        frustum.Extract(proj * view);
    }

    PrepareLights(registry, frustum);

    std::vector<AnimatedEntry> animatedEntries;
    CollectAndRenderItems(registry, frustum, animatedEntries);
    DrawAnimatedEntities(animatedEntries, options);
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
        if (light.Type != LightType::Directional && !frustum.IsSphereVisible(worldPos, light.Radius)) continue;

        RenderLight rl;
        rl.color = {light.LightColor.r / 255.0f, light.LightColor.g / 255.0f, light.LightColor.b / 255.0f, light.LightColor.a / 255.0f};
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
                                          std::vector<AnimatedEntry>& animatedEntries)
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
        
        AssetState state = modelAsset->GetState();
        if (state != AssetState::Ready)
        {
            continue;
        }

        if (!frustum.IsBoxVisible(modelAsset->GetBoundingBox(), transform.WorldTransform)) continue;

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

        if (registry.all_of<AnimationComponent>(entity))
        {
            AnimatedEntry entry;
            entry.asset = modelAsset;
            entry.worldTransform = transform.WorldTransform;
            entry.materials = mesh.Materials;
            entry.shaderOverride = shaderOver;
            entry.customUniforms = uniforms;
            entry.animation = registry.get<AnimationComponent>(entity);
            animatedEntries.push_back(std::move(entry));
        }
        else
        {
            DrawModel(modelAsset, transform.WorldTransform, mesh.Materials, {}, shaderOver, uniforms);
        }
    }

    auto primView = registry.view<TransformComponent, PrimitiveComponent>();
    for (auto entity : primView)
    {
        auto [transform, primitive] = primView.get<TransformComponent, PrimitiveComponent>(entity);
        if (primitive.Type == PrimitiveType::None) continue;

        if (!primitive.Asset || primitive.Dirty)
        {
            const char* paths[] = {"", ":cube:", ":sphere:", ":plane:", ":cylinder:", ":cone:", ":torus:", ":knot:", ":hemisphere:"};
            int idx = (int)primitive.Type;
            if (idx > 0 && idx < 9)
            {
                ProceduralParameters p; p.Radius = primitive.Radius; p.Height = primitive.Height;
                p.Slices = primitive.Slices; p.Stacks = primitive.Stacks; p.Dimensions = primitive.Dimensions;
                Model m = ModelLoader::GenerateProceduralModel(paths[idx], p);
                if (!m.Meshes.empty())
                {
                    if (!primitive.Asset) { primitive.Asset = std::make_shared<ModelAsset>(); primitive.Asset->SetPath(paths[idx]); }
                    primitive.Asset->SetModel(m);
                    primitive.Asset->SetState(AssetState::Ready);
                }
                primitive.Dirty = false;
            }
        }

        if (!primitive.Asset || primitive.Asset->GetState() != AssetState::Ready) continue;
        if (!frustum.IsBoxVisible(primitive.Asset->GetBoundingBox(), transform.WorldTransform)) continue;

        std::shared_ptr<ShaderAsset> shaderOver;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& sc = registry.get<ShaderComponent>(entity);
            if (sc.Enabled && !sc.ShaderPath.empty()) shaderOver = am.Get<ShaderAsset>(sc.ShaderPath);
        }
        DrawModel(primitive.Asset, transform.WorldTransform, {}, {}, shaderOver);
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries, const SceneRenderOptions& options)
{
    for (const auto& entry : animatedEntries)
    {
        DrawModel(entry.asset, entry.worldTransform, entry.materials, {}, entry.shaderOverride, entry.customUniforms);
    }
}

void SceneRenderer::DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const glm::mat4& transform,
                              const std::vector<MaterialSlot>& materialSlotOverrides,
                              const std::vector<glm::mat4>& boneMatrices,
                              const std::shared_ptr<ShaderAsset>& shaderOverride,
                              const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready) return;

    auto& model = modelAsset->GetModel();
    auto& renderer = Renderer::Get();
    auto activeShader = shaderOverride ? shaderOverride : (renderer.GetShaderLibrary().Exists("Lighting") ? renderer.GetShaderLibrary().Get("Lighting") : nullptr);

    if (!activeShader || !activeShader->GetShader()) return;

    for (const auto& inst : modelAsset->GetInstances())
    {
        int i = inst.meshIndex;
        if (i < 0 || i >= (int)model.Meshes.size()) continue;

        m_CurrentStats.DrawCalls++;
        m_CurrentStats.MeshCount++;
        m_CurrentStats.PolyCount += model.Meshes[i].TriangleCount;

        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);
        
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

Material SceneRenderer::ResolveMaterialForMesh(int meshIndex, const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
{
    Material material = model.Materials[model.Meshes[meshIndex].MaterialIndex];
    for (const auto& slot : materialSlotOverrides)
    {
        bool match = (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex) ||
                     (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.Meshes[meshIndex].MaterialIndex);
        if (match)
        {
            material.AlbedoColor = {slot.Material.AlbedoColor.r / 255.0f, slot.Material.AlbedoColor.g / 255.0f, slot.Material.AlbedoColor.b / 255.0f, slot.Material.AlbedoColor.a / 255.0f};
            if (slot.Material.OverrideAlbedo && !slot.Material.AlbedoPath.empty())
            {
                auto tex = AssetManager::Get().Get<TextureAsset>(slot.Material.AlbedoPath);
                if (tex && tex->IsReady()) material.AlbedoMap = tex->GetTexture()->GetRendererID();
            }
            material.EmissiveIntensity = slot.Material.EmissiveIntensity;
            material.Metalness = slot.Material.Metalness;
            material.Roughness = slot.Material.Roughness;
            break;
        }
    }
    return material;
}

void SceneRenderer::BindShaderUniforms(ShaderAsset* shaderAsset, const std::vector<glm::mat4>& boneMatrices, const std::vector<ShaderUniform>& overrides)
{
    auto shader = shaderAsset->GetShader();
    shader->Bind();
    if (!boneMatrices.empty())
    {
        shader->SetMatrices("boneMatrices", boneMatrices.data(), std::min((int)boneMatrices.size(), 128));
    }
    for (const auto& u : overrides)
    {
        if (u.Type == 0) shader->SetFloat(u.Name, u.Value[0]);
        else if (u.Type == 1) shader->SetVec2(u.Name, {u.Value[0], u.Value[1]});
        else if (u.Type == 2) shader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]});
        else if (u.Type >= 3) shader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
    }
}

void SceneRenderer::BindMaterialUniforms(ShaderAsset* shaderAsset, const Material& material, int meshIndex, const Model& model, const std::vector<MaterialSlot>& overrides)
{
    auto shader = shaderAsset->GetShader();
    shader->Bind();

    auto resolveMap = [](uint32_t currentId, const std::string& path) -> uint32_t {
        if (currentId > 0) return currentId;
        // Embedded textures (path starts with '*') are never in the AssetManager.
        // Their GPU ID is stored directly in AlbedoMap/NormalMap etc. by model_asset.cpp.
        if (path.empty() || (!path.empty() && path.front() == '*')) return 0;
        auto tex = AssetManager::Get().Get<TextureAsset>(path);
        if (tex && tex->IsReady()) return tex->GetTexture()->GetRendererID();
        return 0;
    };

    uint32_t albedoMap = resolveMap(material.AlbedoMap, material.AlbedoPath);
    uint32_t normalMap = resolveMap(material.NormalMap, material.NormalPath);
    uint32_t metallicMap = resolveMap(material.MetallicRoughnessMap, material.MetallicRoughnessPath);
    uint32_t emissiveMap = resolveMap(material.EmissiveMap, material.EmissivePath);
    uint32_t occlusionMap = resolveMap(material.OcclusionMap, material.OcclusionPath);

    // 1. Albedo (texture0, Unit 0)
    if (albedoMap > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoMap);
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
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, metallicMap);
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
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMap);
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
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, metallicMap);
        shader->SetInt("texture3", 3);
    }

    // 5. Occlusion (texture4, Unit 4)
    if (occlusionMap > 0)
    {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, occlusionMap);
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
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, emissiveMap);
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
    if (!options.ShowDebugColliders && !options.ShowDebugCollisionModelBox && !options.ShowDebugSpawnZones && !options.DrawGrid) return;
    auto& registry = scene->GetRegistry();
    glDisable(GL_DEPTH_TEST);
    if (options.DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
        Renderer::Get().DrawInfiniteGrid(camera, scene->GetSettings().Grid.Spacing, {200, 200, 200, 255});
    if (options.ShowDebugColliders) DrawColliderDebug(registry, options);
    if (options.ShowDebugCollisionModelBox) DrawCollisionModelBoxDebug(registry, options);
    if (options.ShowDebugSpawnZones) DrawSpawnDebug(registry, options);
    glEnable(GL_DEPTH_TEST);
}

void SceneRenderer::DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options)
{
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
        if (!collider.Enabled) continue;
        glm::vec4 color = collider.IsColliding ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 1, 0, 1);
        if (collider.Type == ColliderType::Box)
        {
            BoundingBox b = CalculateColliderWorldAABB(collider, transform.WorldTransform);
            glm::vec3 center = (b.Min + b.Max) * 0.5f;
            Renderer::Get().DrawCubeWires(glm::translate(glm::mat4(1.0f), center), b.Max - b.Min, color);
        }
        else if (collider.Type == ColliderType::Sphere)
            Renderer::Get().DrawSphereWires(transform.WorldTransform * glm::translate(glm::mat4(1.0f), collider.Offset), collider.Radius, color);
    }
}

void SceneRenderer::DrawCollisionModelBoxDebug(entt::registry& registry, const SceneRenderOptions& options) {}
void SceneRenderer::DrawSpawnDebug(entt::registry& registry, const SceneRenderOptions& options) {}
void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera) {}

BoundingBox SceneRenderer::CalculateColliderWorldAABB(const ColliderComponent& collider, const glm::mat4& worldTransform)
{
    glm::vec3 halfExtents = collider.Size * 0.5f;
    glm::vec3 min = collider.Offset - halfExtents;
    glm::vec3 max = collider.Offset + halfExtents;
    glm::vec3 corners[8] = { {min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z},
                             {min.x, min.y, max.z}, {max.x, min.y, max.z}, {min.x, max.y, max.z}, {max.x, max.y, max.z} };
    BoundingBox result = { {FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX} };
    for (int i = 0; i < 8; i++)
    {
        glm::vec3 worldCorner = glm::vec3(worldTransform * glm::vec4(corners[i], 1.0f));
        result.Min = glm::min(result.Min, worldCorner);
        result.Max.x = (worldCorner.x > result.Max.x) ? worldCorner.x : result.Max.x;
        result.Max.y = (worldCorner.y > result.Max.y) ? worldCorner.y : result.Max.y;
        result.Max.z = (worldCorner.z > result.Max.z) ? worldCorner.z : result.Max.z;
    }
    return result;
}

} // namespace CHEngine
