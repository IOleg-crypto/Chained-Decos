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
#include "engine/scene/components/camera_component.h"
#include "engine/scene/components/game_components.h"
#include "engine/scene/components/light_component.h"
#include "engine/scene/components/mesh_component.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
                    renderer.DrawSkybox(texId, skyboxMode, texture->IsHDR(), settings.Exposure, settings.Brightness,
                                        settings.Contrast, camera);
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
        float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
        glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
        glm::mat4 proj =
            (camera.Projection == 0)
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

        // Sync materials if not already initialized
        mesh.SyncMaterials(modelAsset->GetID());

        AssetState state = modelAsset->GetState();
        if (state != AssetState::Ready)
        {
            continue;
        }

        if (!frustum.IsBoxVisible(modelAsset->GetBoundingBox(), transform.WorldTransform))
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

        if (registry.all_of<AnimationComponent>(entity))
        {
            AnimatedEntry entry;
            entry.asset = modelAsset;
            entry.worldTransform = transform.WorldTransform;
            entry.materials = materials;
            entry.shaderOverride = shaderOver;
            entry.customUniforms = uniforms;
            entry.animation = registry.get<AnimationComponent>(entity);
            animatedEntries.push_back(std::move(entry));
        }
        else
        {
            DrawModel(modelAsset, transform.WorldTransform, materials, {}, shaderOver, uniforms);
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
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& sc = registry.get<ShaderComponent>(entity);
            if (sc.Enabled && !sc.ShaderPath.empty())
            {
                shaderOver = am.Get<ShaderAsset>(sc.ShaderPath);
            }
        }
        DrawModel(primitive.Asset, transform.WorldTransform, {}, {}, shaderOver);
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries,
                                         const SceneRenderOptions& options)
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
                              const std::vector<ShaderUniform>& shaderUniformOverrides)
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

    for (const auto& inst : modelAsset->GetInstances())
    {
        int i = inst.meshIndex;
        if (i < 0 || i >= (int)model.Meshes.size())
        {
            continue;
        }

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

Material SceneRenderer::ResolveMaterialForMesh(int meshIndex, const Model& model,
                                               const std::vector<MaterialSlot>& materialSlotOverrides)
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
                auto tex = AssetManager::Get().Get<TextureAsset>(slot.Material.AlbedoPath);
                if (tex && tex->IsReady())
                {
                    material.AlbedoMap = tex->GetTexture()->GetRendererID();
                }
            }
            material.EmissiveIntensity = slot.Material.EmissiveIntensity;
            material.Metalness = slot.Material.Metalness;
            material.Roughness = slot.Material.Roughness;
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
    if (!boneMatrices.empty())
    {
        shader->SetMatrices("boneMatrices", boneMatrices.data(), std::min((int)boneMatrices.size(), 128));
    }
    for (const auto& u : overrides)
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
        auto tex = AssetManager::Get().Get<TextureAsset>(path);
        if (tex && tex->IsReady())
        {
            return tex->GetTexture()->GetRendererID();
        }
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
    if (!options.ShowDebugColliders && !options.ShowDebugCollisionModelBox && !options.ShowDebugSpawnZones &&
        !options.DrawGrid)
    {
        return;
    }

    auto& registry = scene->GetRegistry();

    // Save current GL state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);

    // Setup for debug drawing
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    if (options.SetCollisionWireframeMode == 1)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (options.DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
    {
        Renderer::Get().DrawInfiniteGrid(camera, scene->GetSettings().Grid.Spacing, {0.8f, 0.8f, 0.8f, 1.0f});
    }

    if (options.ShowDebugColliders)
    {
        DrawColliderDebug(registry, options);
    }

    if (options.ShowDebugCollisionModelBox)
    {
        DrawCollisionModelBoxDebug(registry, options);
    }

    if (options.ShowDebugSpawnZones)
    {
        DrawSpawnDebug(registry, options);
    }

    // Restore GL state
    if (depthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (blendEnabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
    glBindVertexArray(0); // Unbind VAO
}

void SceneRenderer::DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options)
{
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
        if (!collider.Enabled)
        {
            continue;
        }
        glm::vec4 color = collider.IsColliding ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 1, 0, 1);
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
                Renderer::Get().DrawCubeWires(baseTransform, collider.Size * entityScale, color);
            }
            else if (collider.Type == ColliderType::Sphere)
            {
                // For sphere, we use the maximum component of the entity scale for the overall radius multiplier
                float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                Renderer::Get().DrawSphereWires(baseTransform, collider.Radius * maxScale, color);
            }
            else if (collider.Type == ColliderType::Capsule)
            {
                float maxScale = glm::max(entityScale.x, glm::max(entityScale.y, entityScale.z));
                Renderer::Get().DrawCapsuleWires(baseTransform, collider.Radius * maxScale,
                                                 collider.Height * entityScale.y, color);
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
                    bool useWireframe = (options.SetCollisionWireframeMode == 0);
                    for (const auto& inst : instances)
                    {
                        if (inst.meshIndex >= 0 && inst.meshIndex < (int)model.Meshes.size())
                        {
                            Renderer::Get().DrawMeshWire(model.Meshes[inst.meshIndex], color,
                                                         transform.WorldTransform * inst.localTransform, useWireframe);
                        }
                    }
                }
            }
        }
    }
}

void SceneRenderer::DrawCollisionModelBoxDebug(entt::registry& registry, const SceneRenderOptions& options)
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

void SceneRenderer::DrawSpawnDebug(entt::registry& registry, const SceneRenderOptions& options)
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
    // Render special entity icons (cameras, lights, etc.)
    auto& registry = scene->GetRegistry();

    // Draw camera icons
    auto cameraView = registry.view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView)
    {
        auto [transform, camera] = cameraView.get<TransformComponent, CameraComponent>(entity);
        glm::vec4 cameraColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
        glm::vec3 iconSize = glm::vec3(0.1f);
        Renderer::Get().DrawCubeWires(transform.WorldTransform, iconSize, cameraColor);
    }

    // Draw light icons
    auto lightView = registry.view<TransformComponent, LightComponent>();
    for (auto entity : lightView)
    {
        auto [transform, light] = lightView.get<TransformComponent, LightComponent>(entity);
        glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow

        if (light.Type == LightType::Directional)
        {
            // Draw arrow for directional light
            glm::vec3 pos = glm::vec3(transform.WorldTransform[3]);
            glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.5f;
            Renderer::Get().DrawLine(pos, pos + dir, lightColor);
        }
        else if (light.Type == LightType::Point)
        {
            // Draw sphere for point light
            Renderer::Get().DrawSphereWires(transform.WorldTransform, light.Radius * 0.1f, lightColor);
        }
        else if (light.Type == LightType::Spot)
        {
            // Draw cone for spot light
            Renderer::Get().DrawSphereWires(transform.WorldTransform, light.Radius * 0.05f, lightColor);
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

} // namespace CHEngine
