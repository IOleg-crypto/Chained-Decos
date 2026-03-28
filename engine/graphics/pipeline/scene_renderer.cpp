#include "scene_renderer.h"
#include "engine/audio/audio.h"
#include "engine/core/application.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/ch_assert.h"
#include "engine/core/profiler.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/importers/mesh_importer.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/renderer2d.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/light_component.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <unordered_map>
#include <unordered_set>

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
    glm::mat4 worldTransform = GetWorldTransform(registry, entity);
    return glm::vec3(worldTransform[3]);
}

void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                const SceneRenderOptions& options)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_ASSERT(Renderer::IsInitialized(), "Renderer not initialized!");
    CH_CORE_ASSERT(scene, "Scene is null!");

    // 1. Initial State
    glEnable(GL_DEPTH_TEST);

    // 2. Environmental setup
    auto environment = scene->GetSettings().Environment;
    if (!environment)
    {
        environment = options.EnvironmentOverride;
    }

    // 3. Render Passes
    float exposure = 1.0f;
    float gamma = 2.2f;

    if (environment)
    {
        const auto& envSettings = environment->GetSettings();
        Renderer::Get().ApplyEnvironment(envSettings);
        exposure = envSettings.Lighting.Exposure;
        gamma = envSettings.Lighting.Gamma;
    }
    else
    {
        CH_CORE_WARN_ONCE("SceneRenderer: No environment asset for scene!");
    }

    Renderer::Get().UpdateTime(Timestep((float)glfwGetTime()));

    // --- Update Profiler Stats ---
    m_CurrentStats = {};
    m_CurrentStats.EntityCount = (uint32_t)scene->GetRegistry().storage<entt::entity>().size();

    // 2. Scene rendering flow
    Audio::Get().SetListenerPosition(camera.Position, glm::normalize(camera.Target - camera.Position), camera.Up);
    Renderer::Get().BeginScene(camera, nearClip, farClip);
    {
        if (environment)
        {
            if (environment->GetSettings().Skybox.TexturePath.empty())
            {
                CH_CORE_WARN_ONCE("SceneRenderer: Environment exists but Skybox.TexturePath is empty!");
            }
            Renderer::Get().DrawSkybox(environment->GetSettings().Skybox, camera);
        }
        else
        {
            CH_CORE_WARN_ONCE("SceneRenderer: No environment asset for scene!");
        }

        RenderModels(scene, camera, nearClip, farClip, timestep, options);

        RenderDebug(scene, camera, options);

        // RenderSprites(scene); // Disabled 2D system for now

        RenderEditorIcons(scene, camera);
    }
    Renderer::Get().EndScene();

    // 4. Report final stats
    Profiler::UpdateStats(m_CurrentStats);
}

SceneRenderer::InstanceKey::InstanceKey(const std::string& path, const std::vector<MaterialSlot>& mats)
{
    auto hashCombine = [](size_t& seed, size_t hash) { seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2); };

    Hash = std::hash<std::string>{}(path);

    for (const auto& m : mats)
    {
        hashCombine(Hash, std::hash<int>{}((int)m.Target));
        hashCombine(Hash, std::hash<int>{}(m.Index));
        hashCombine(Hash, std::hash<std::string>{}(m.Material.AlbedoPath));
        hashCombine(Hash, std::hash<std::string>{}(m.Material.ShaderPath));
    }
}

void SceneRenderer::RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                 const SceneRenderOptions& options)
{
    auto& registry = scene->GetRegistry();

    // 1. Frustum Extraction
    Frustum frustum;
    {
        float width = (float)Application::Get().GetWindow().GetWidth();
        float height = (float)Application::Get().GetWindow().GetHeight();
        float aspect = (width > 0 && height > 0) ? width / height : 1.0f;

        glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
        glm::mat4 projection =
            (camera.Projection == 0)
                ? glm::perspective(glm::radians(camera.Fovy), aspect, nearClip, farClip)
                : glm::ortho(-aspect * camera.Fovy, aspect * camera.Fovy, -camera.Fovy, camera.Fovy, nearClip, farClip);

        glm::mat4 matVP = projection * view;
        frustum.Extract(matVP);
    }

    // 2. Prepare Lights
    PrepareLights(registry, frustum);

    // 3. Pass A: Collect visible entities
    std::vector<AnimatedEntry> animatedEntries;
    std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash> instanceGroups;
    CollectRenderItems(registry, frustum, animatedEntries, instanceGroups);

    // 4. Pass B: Draw animated individually
    DrawAnimatedEntities(animatedEntries, options);

    // 5. Pass C: Draw static groups (instanced if >=2, single otherwise)
    DrawStaticEntities(instanceGroups);
}

void SceneRenderer::PrepareLights(entt::registry& registry, const Frustum& frustum)
{
    Renderer::Get().ClearLights();

    // Reset to environment default if no directional light found in scene
    LightingSettings mainLight = Renderer::Get().GetData().Lighting.CurrentLighting;
    bool foundDirectional = false;

    int lightCount = 0;
    auto lightView = registry.view<LightComponent>();
    for (auto entity : lightView)
    {
        auto& light = lightView.get<LightComponent>(entity);
        glm::mat4 worldTransform = GetWorldTransform(registry, entity);
        glm::vec3 worldPos = glm::vec3(worldTransform[3]);

        // Extract forward direction from matrix (column 2 is -Z in Raylib/standard right-handed)
        // Wait, Raylib's forward is {0, 0, 1}? No, usually it's -Z.
        // Let's use the same logic as before but with GLM
        glm::vec3 localDir = {0, -1, 0};
        glm::vec3 worldDir = glm::normalize(glm::vec3(worldTransform * glm::vec4(localDir, 0.0f)));

        if (light.Type == LightType::Directional && !foundDirectional)
        {
            mainLight.Direction = {worldDir.x, worldDir.y, worldDir.z};
            mainLight.LightColor = light.LightColor;
            foundDirectional = true;
        }

        if (lightCount < LightingData::MaxLights)
        {
            if (light.Type != LightType::Directional && !frustum.IsSphereVisible(worldPos, light.Radius))
            {
                continue;
            }

            RenderLight rl;
            rl.color.r = light.LightColor.r / 255.0f;
            rl.color.g = light.LightColor.g / 255.0f;
            rl.color.b = light.LightColor.b / 255.0f;
            rl.color.a = light.LightColor.a / 255.0f;

            rl.position = worldPos;
            rl.intensity = (light.Intensity <= 0.0f) ? 1.0f : light.Intensity;
            rl.radius = light.Radius;
            rl.innerCutoff = light.InnerCutoff;
            rl.outerCutoff = light.OuterCutoff;
            rl.type = (int)light.Type;
            rl.enabled = 1;

            if (light.Type == LightType::Spot)
            {
                rl.direction = worldDir;
            }

            Renderer::Get().SetLight(lightCount++, rl);
        }
    }

    if (foundDirectional)
    {
        Renderer::Get().SetMainLight(mainLight);
    }

    Renderer::Get().SetLightCount(lightCount);
}

void SceneRenderer::CollectRenderItems(entt::registry& registry, const Frustum& frustum,
                                       std::vector<AnimatedEntry>& animatedEntries,
                                       std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups)
{
    std::unordered_set<ModelAsset*> updatedAssets;

    // 3. Collect Models
    auto view = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

        if (model.ModelPath.empty())
        {
            continue;
        }

        auto modelAsset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetState() != AssetState::Ready)
        {
            continue;
        }

        // 1. Precise Frustum Culling
        const glm::mat4& worldTransform = transform.WorldTransform;
        BoundingBox aabb = modelAsset->GetBoundingBox();

        if (!frustum.IsBoxVisible(aabb, worldTransform))
        {
            continue;
        }

        // 2. Optimized Asset Update (Once per unique asset per frame)
        if (updatedAssets.find(modelAsset.get()) == updatedAssets.end())
        {
            modelAsset->OnUpdate();
            updatedAssets.insert(modelAsset.get());
        }

        std::shared_ptr<ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        bool hasShaderOverride = false;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& shaderComp = registry.get<ShaderComponent>(entity);
            if (shaderComp.Enabled && !shaderComp.ShaderPath.empty())
            {
                shaderOverride = AssetManager::Get().Get<ShaderAsset>(shaderComp.ShaderPath);
                customUniforms = shaderComp.Uniforms;
                hasShaderOverride = true;
            }
        }

        const bool isAnimated = registry.all_of<AnimationComponent>(entity);

        if (isAnimated)
        {
            AnimatedEntry entry;
            entry.asset = modelAsset;
            entry.worldTransform = worldTransform;
            entry.materials = model.Materials;
            entry.shaderOverride = shaderOverride;
            entry.customUniforms = customUniforms;
            entry.animation = registry.get<AnimationComponent>(entity);
            animatedEntries.push_back(std::move(entry));
        }
        else if (!hasShaderOverride)
        {
            InstanceKey key{model.ModelPath, model.Materials};
            auto& group = instanceGroups[key];
            if (group.transforms.empty())
            {
                group.asset = modelAsset;
                group.materials = model.Materials;
            }
            group.transforms.push_back(worldTransform);
        }
        else
        {
            DrawModel(modelAsset, worldTransform, model.Materials, {}, shaderOverride, customUniforms);
        }
    }

    // 4. Collect Primitives
    auto primitiveView = registry.view<TransformComponent, PrimitiveComponent>();
    for (auto entity : primitiveView)
    {
        auto [transform, primitive] = primitiveView.get<TransformComponent, PrimitiveComponent>(entity);

        if (primitive.Type == PrimitiveType::None)
        {
            continue;
        }

        // Lazy load/cache primitive asset or regenerate if dirty
        if ((!primitive.Asset || primitive.Dirty))
        {
            const char* primitivePaths[] = {
                "", ":cube:", ":sphere:", ":plane:", ":cylinder:", ":cone:", ":torus:", ":knot:", ":hemisphere:"};
            int typeIdx = (int)primitive.Type;
            if (typeIdx > 0 && typeIdx < (int)std::size(primitivePaths))
            {
                ProceduralParameters params;
                params.Radius = primitive.Radius;
                params.InnerRadius = primitive.InnerRadius;
                params.Height = primitive.Height;
                params.Slices = primitive.Slices;
                params.Stacks = primitive.Stacks;
                params.Dimensions = primitive.Dimensions;

                // Create a temporary model from updated parameters
                Model model = MeshImporter::GenerateProceduralModel(primitivePaths[typeIdx], params);
                if (model.Meshes.size() > 0)
                {
                    if (!primitive.Asset)
                    {
                        primitive.Asset = std::make_shared<ModelAsset>();
                        primitive.Asset->SetPath(primitivePaths[typeIdx]);
                    }
                    else
                    {
                        // Unload existing mesh data if regenerating
                        // UnloadModel(primitive.Asset->GetModel()); // Removed Raylib call
                    }

                    primitive.Asset->GetModel() = model;
                    primitive.Asset->SetState(AssetState::Ready);
                }
                primitive.Dirty = false;
            }
        }

        if (!primitive.Asset || primitive.Asset->GetState() != AssetState::Ready)
        {
            continue;
        }

        const glm::mat4& worldTransform = transform.WorldTransform;
        BoundingBox aabb = primitive.Asset->GetBoundingBox();

        if (!frustum.IsBoxVisible(aabb, worldTransform))
        {
            continue;
        }

        // 2. Optimized Asset Update
        if (updatedAssets.find(primitive.Asset.get()) == updatedAssets.end())
        {
            primitive.Asset->OnUpdate();
            updatedAssets.insert(primitive.Asset.get());
        }

        // Handle shader override for primitives too
        std::shared_ptr<ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        bool hasShaderOverride = false;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& shaderComp = registry.get<ShaderComponent>(entity);
            if (shaderComp.Enabled && !shaderComp.ShaderPath.empty())
            {
                shaderOverride = AssetManager::Get().Get<ShaderAsset>(shaderComp.ShaderPath);
                customUniforms = shaderComp.Uniforms;
                hasShaderOverride = true;
            }
        }

        // Primitives are never animated (for now)
        if (!hasShaderOverride)
        {
            std::string pathKey = "primitive_" + std::to_string((int)primitive.Type);
            InstanceKey key{pathKey, {}}; // No material slots for primitives yet
            auto& group = instanceGroups[key];
            if (group.transforms.empty())
            {
                group.asset = primitive.Asset;
                group.materials = {};
            }
            group.transforms.push_back(transform.WorldTransform);
        }
        else
        {
            DrawModel(primitive.Asset, transform.WorldTransform, {}, {}, shaderOverride, customUniforms);
        }
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries,
                                         const SceneRenderOptions& options)
{
    // 1. Виносимо те, що не змінюється, за межі циклу (економія обчислень)
    float targetFPS = options.TargetFPS > 0.0f ? options.TargetFPS : 60.0f;
    float invFrameTime = targetFPS; // Це математично те саме, що 1.0f / (1.0f / targetFPS)

    for (const auto& entry : animatedEntries) // Додав const для безпеки
    {
        // 2. Множення замість ділення працює швидше
        float fractionalFrame = (float)entry.animation.CurrentFrame + (entry.animation.FrameTimeCounter * invFrameTime);

        int targetAnim = -1;
        float targetFractionalFrame = 0.0f;
        float blendWeight = 0.0f;

        if (entry.animation.Blending)
        {
            targetAnim = entry.animation.TargetAnimationIndex;
            targetFractionalFrame =
                (float)entry.animation.TargetFrame + (entry.animation.FrameTimeCounter * invFrameTime);

            // 3. Захист від ділення на нуль (уникаємо NaN) та обмеження ваги від 0 до 1
            if (entry.animation.BlendDuration > 0.0001f)
            {
                blendWeight = entry.animation.BlendTimer / entry.animation.BlendDuration;
                blendWeight = std::clamp(blendWeight, 0.0f, 1.0f);
            }
        }

        // Compute animation pose (now returns glm::mat4)
        std::vector<glm::mat4> boneMatrices = entry.asset->ComputeAnimationPose(
            entry.animation.CurrentAnimationIndex, fractionalFrame, targetAnim, targetFractionalFrame, blendWeight);

        DrawModel(entry.asset, entry.worldTransform, entry.materials, boneMatrices, entry.shaderOverride,
                  entry.customUniforms);
    }
}
void SceneRenderer::DrawStaticEntities(std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups)
{
    for (auto& [key, group] : instanceGroups)
    {
        if (group.transforms.size() >= 2)
        {
            // Use instanced rendering if we have multiple instances
            Renderer::Get().DrawMeshInstanced(group.asset->GetModel().Meshes[0],
                                              ResolveMaterialForMesh(0, group.asset->GetModel(), group.materials),
                                              group.transforms);
        }
        else
        {
            for (const auto& transform : group.transforms)
            {
                DrawModel(group.asset, transform, group.materials);
            }
        }
    }
}

void SceneRenderer::RenderDebug(Scene* scene, const Camera3D& camera, const SceneRenderOptions& options)
{
    if (!options.ShowDebugColliders && !options.ShowDebugCollisionModelBox && !options.ShowDebugSpawnZones &&
        !options.DrawGrid)
    {
        return;
    }
    auto& registry = scene->GetRegistry();

    glDisable(GL_DEPTH_TEST);

    if (options.DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
    {
        const auto& grid = scene->GetSettings().Grid;
        Renderer::Get().DrawInfiniteGrid(camera, grid.Spacing, {200, 200, 200, 255});
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

    glEnable(GL_DEPTH_TEST);
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

        const glm::mat4& worldTransform = transform.WorldTransform;
        // Green = no collision, Red = currently colliding
        // RED and LIME are Raylib residues. Replacing with inline colors.
        glm::vec4 debugColor =
            collider.IsColliding ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

        if (collider.Type == ColliderType::Box)
        {
            BoundingBox worldAABB = CalculateColliderWorldAABB(collider, worldTransform);
            glm::vec3 center = {(worldAABB.Min.x + worldAABB.Max.x) * 0.5f, (worldAABB.Min.y + worldAABB.Max.y) * 0.5f,
                                (worldAABB.Min.z + worldAABB.Max.z) * 0.5f};
            glm::vec3 size = {worldAABB.Max.x - worldAABB.Min.x, worldAABB.Max.y - worldAABB.Min.y,
                              worldAABB.Max.z - worldAABB.Min.z};
            Renderer::Get().DrawCubeWires(glm::translate(glm::mat4(1.0f), center), size, debugColor);
        }
        else if (collider.Type == ColliderType::Sphere)
        {
            glm::mat4 sphereTransform =
                worldTransform *
                glm::translate(glm::mat4(1.0f), {collider.Offset.x, collider.Offset.y, collider.Offset.z});
            Renderer::Get().DrawSphereWires(sphereTransform, collider.Radius, debugColor);
        }
        else if (collider.Type == ColliderType::Capsule)
        {
            glm::mat4 capsuleTransform =
                worldTransform *
                glm::translate(glm::mat4(1.0f), {collider.Offset.x, collider.Offset.y, collider.Offset.z});
            Renderer::Get().DrawCapsuleWires(capsuleTransform, collider.Radius, collider.Height, debugColor);
        }
        else if (collider.Type == ColliderType::Mesh)
        {
            // Draw the world-space AABB of the mesh collider instead of per-triangle wireframe.
            // Per-triangle mode was producing unreadable visual noise when scene had many mesh colliders.
            if (!collider.ModelPath.empty())
            {
                auto model = AssetManager::Get().Get<ModelAsset>(collider.ModelPath);
                if (model && model->GetState() == AssetState::Ready)
                {
                    BoundingBox localBox = model->GetBoundingBox();

                    // Transform local AABB corners to world space and re-compute a world AABB
                    Vector3 corners[8] = {
                        {localBox.Min.x, localBox.Min.y, localBox.Min.z},
                        {localBox.Max.x, localBox.Min.y, localBox.Min.z},
                        {localBox.Min.x, localBox.Max.y, localBox.Min.z},
                        {localBox.Max.x, localBox.Max.y, localBox.Min.z},
                        {localBox.Min.x, localBox.Min.y, localBox.Max.z},
                        {localBox.Max.x, localBox.Min.y, localBox.Max.z},
                        {localBox.Min.x, localBox.Max.y, localBox.Max.z},
                        {localBox.Max.x, localBox.Max.y, localBox.Max.z},
                    };

                    glm::vec3 wMin = {FLT_MAX, FLT_MAX, FLT_MAX};
                    glm::vec3 wMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
                    for (const auto& c : corners)
                    {
                        glm::vec4 wp = worldTransform * glm::vec4(c.x, c.y, c.z, 1.0f);
                        wMin = glm::min(wMin, glm::vec3(wp));
                        wMax = glm::max(wMax, glm::vec3(wp));
                    }

                    glm::vec3 center = (wMin + wMax) * 0.5f;
                    glm::vec3 size = wMax - wMin;
                    // Cyan for mesh colliders to distinguish them from box colliders
                    glm::vec4 meshDebugColor =
                        collider.IsColliding ? glm::vec4(1, 0, 0, 1) : glm::vec4(0.4f, 0.8f, 1.0f, 1.0f);
                    Renderer::Get().DrawCubeWires(glm::translate(glm::mat4(1.0f), center), size,
                                                  {meshDebugColor.r / 255.0f, meshDebugColor.g / 255.0f,
                                                   meshDebugColor.b / 255.0f, meshDebugColor.a / 255.0f});

                    // Also draw individual mesh wireframes with depth test ON so they
                    // don't render through walls — gives shape detail without visual chaos.
                    glEnable(GL_DEPTH_TEST);
                    const auto& rayModel = model->GetModel();
                    glm::vec4 wireCol =
                        collider.IsColliding ? glm::vec4(1.0f, 0.3f, 0.3f, 0.5f) : glm::vec4(0.0f, 0.7f, 0.8f, 0.4f);
                    for (const auto& inst : model->GetInstances())
                    {
                        glm::mat4 finalTransform = worldTransform * inst.localTransform;
                        Renderer::Get().DrawMeshWire(rayModel.Meshes[inst.meshIndex], wireCol, finalTransform);
                    }

                    glDisable(GL_DEPTH_TEST);
                }
            }
        }

        m_CurrentStats.ColliderCount++;
    }
}

void SceneRenderer::DrawCollisionModelBoxDebug(entt::registry& registry, const SceneRenderOptions& options)
{
    // 1. Використовуємо посилання (&), щоб не копіювати матриці та структури щоразу
    auto view = registry.view<TransformComponent, ColliderComponent>();

    for (auto entity : view)
    {
        const auto& [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);

        if (!collider.Enabled)
        {
            continue;
        }

        // 2. ОПТИМІЗАЦІЯ: Беремо матрицю прямо з отриманого компонента transform
        // Не потрібно викликати GetWorldTransform(registry, entity) повторно.
        const glm::mat4& worldTransform = transform.WorldTransform;

        BoundingBox worldAABB = CalculateColliderWorldAABB(collider, worldTransform);

        // 3. Розраховуємо розмір та центр
        glm::vec3 size = {worldAABB.Max.x - worldAABB.Min.x, worldAABB.Max.y - worldAABB.Min.y,
                          worldAABB.Max.z - worldAABB.Min.z};

        // Перевірка, чи бокс не "вивернутий" (min > max)
        if (size.x >= 0 && size.y >= 0 && size.z >= 0)
        {
            glm::vec3 center = {(worldAABB.Min.x + worldAABB.Max.x) * 0.5f, (worldAABB.Min.y + worldAABB.Max.y) * 0.5f,
                                (worldAABB.Min.z + worldAABB.Max.z) * 0.5f};

            // Порада: можна міняти колір, якщо об'єкт зіткнувся (як у DrawColliderDebug)
            glm::vec4 debugColor =
                collider.IsColliding ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(1.0f, 0.0f, 0.0f, 0.39f);

            Renderer::Get().DrawCubeWires(glm::translate(glm::mat4(1.0f), center), size, debugColor);
        }
    }
}

BoundingBox SceneRenderer::CalculateColliderWorldAABB(const ColliderComponent& collider,
                                                      const glm::mat4& worldTransform)
{
    BoundingBox box = {};
    glm::vec3 minLocal = {collider.Offset.x, collider.Offset.y, collider.Offset.z};
    glm::vec3 maxLocal = minLocal + glm::vec3{collider.Size.x, collider.Size.y, collider.Size.z};

    glm::vec3 corners[8] = {{minLocal.x, minLocal.y, minLocal.z}, {maxLocal.x, minLocal.y, minLocal.z},
                            {minLocal.x, maxLocal.y, minLocal.z}, {maxLocal.x, maxLocal.y, minLocal.z},
                            {minLocal.x, minLocal.y, maxLocal.z}, {maxLocal.x, minLocal.y, maxLocal.z},
                            {minLocal.x, maxLocal.y, maxLocal.z}, {maxLocal.x, maxLocal.y, maxLocal.z}};

    glm::vec3 minWorld = {FLT_MAX, FLT_MAX, FLT_MAX};
    glm::vec3 maxWorld = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 worldPt = worldTransform * glm::vec4(corners[i], 1.0f);
        minWorld = glm::min(minWorld, glm::vec3(worldPt));
        maxWorld = glm::max(maxWorld, glm::vec3(worldPt));
    }
    box.Min = {minWorld.x, minWorld.y, minWorld.z};
    box.Max = {maxWorld.x, maxWorld.y, maxWorld.z};

    return box;
}

void SceneRenderer::DrawSpawnDebug(entt::registry& registry, const SceneRenderOptions& options)
{
    auto view = registry.view<TransformComponent, SpawnComponent>();
    for (auto entity : view)
    {
        auto [transform, spawn] = view.get<TransformComponent, SpawnComponent>(entity);
        if (spawn.RenderSpawnZoneInScene)
        {
            glm::mat4 worldTransform = GetWorldTransform(registry, entity);
            Renderer::Get().DrawCubeWires(worldTransform, {spawn.ZoneSize.x, spawn.ZoneSize.y, spawn.ZoneSize.z},
                                          {1.0f, 1.0f, 0.0f, 0.78f});
        }
    }
}

void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera)
{
    auto& registry = scene->GetRegistry();
    auto& assetManager = AssetManager::Get();

    if (m_EditorResources.LightIconId == 0)
    {
        auto texture = assetManager.Get<TextureAsset>("resources/icons/light_bulb.png");
        if (texture && texture->IsReady())
        {
            m_EditorResources.LightIconId = texture->GetTexture().id;
        }
    }
    if (m_EditorResources.SpawnIconId == 0)
    {
        auto texture = assetManager.Get<TextureAsset>("resources/icons/leaf_icon.png");
        if (texture && texture->IsReady())
        {
            m_EditorResources.SpawnIconId = texture->GetTexture().id;
        }
    }
    if (m_EditorResources.CameraIconId == 0)
    {
        auto texture = assetManager.Get<TextureAsset>("resources/icons/camera_icon.png");
        if (texture && texture->IsReady())
        {
            m_EditorResources.CameraIconId = texture->GetTexture().id;
        }
    }

    glDisable(GL_DEPTH_TEST);
    {
        auto view = registry.view<TransformComponent, LightComponent>();
        for (auto entity : view)
        {
            glm::vec3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_EditorResources.LightIconId, worldPos, 1.5f, {1, 1, 1, 1});
        }
    }
    {
        auto view = registry.view<TransformComponent, SpawnComponent>();
        for (auto entity : view)
        {
            glm::vec3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_EditorResources.SpawnIconId, worldPos, 1.5f, {1, 1, 1, 1});
        }
    }
    {
        auto view = registry.view<TransformComponent, CameraComponent>();
        for (auto entity : view)
        {
            glm::vec3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_EditorResources.CameraIconId, worldPos, 1.5f, {1, 1, 1, 1});
        }
    }
    glEnable(GL_DEPTH_TEST);
}

void SceneRenderer::RenderSprites(Scene* scene)
{
    CH_CORE_ASSERT(scene, "Scene is null!");
    CH_CORE_ASSERT(Renderer2D::Get().IsInitialized(), "Renderer2D not initialized!");
    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, SpriteComponent>();

    std::vector<entt::entity> sortedEntities;
    for (auto entity : view)
    {
        sortedEntities.push_back(entity);
    }

    if (sortedEntities.empty())
    {
        return;
    }

    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<SpriteComponent>(a).ZOrder < view.get<SpriteComponent>(b).ZOrder;
    });

    Renderer2D::Get().BeginCanvas();
    for (auto entityID : sortedEntities)
    {
        auto& sprite = view.get<SpriteComponent>(entityID);

        if (sprite.TexturePath.empty())
        {
            continue;
        }

        if (!sprite.Texture)
        {
            sprite.Texture = AssetManager::Get().Get<TextureAsset>(sprite.TexturePath);
        }

        glm::vec3 worldPos = GetWorldPosition(registry, entityID);

        Renderer2D::Get().DrawSprite(
            glm::vec2(worldPos.x, worldPos.y), glm::vec2(1.0f, 1.0f), 0.0f, sprite.Texture,
            {sprite.Tint.r / 255.0f, sprite.Tint.g / 255.0f, sprite.Tint.b / 255.0f, sprite.Tint.a / 255.0f});
    }
    Renderer2D::Get().EndCanvas();
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

    Model& model = modelAsset->GetModel();
    auto& renderer = Renderer::Get();
    auto activeShader =
        shaderOverride
            ? shaderOverride
            : (renderer.GetShaderLibrary().Exists("Lighting") ? renderer.GetShaderLibrary().Get("Lighting") : nullptr);

    const auto& instances = modelAsset->GetInstances();
    for (const auto& inst : instances)
    {
        int i = inst.meshIndex;
        if (i < 0 || i >= (int)model.Meshes.size())
        {
            continue;
        }

        // instances are already in model-local space.
        glm::mat4 meshWorldTransform = transform * inst.localTransform;

        m_CurrentStats.DrawCalls++;
        m_CurrentStats.MeshCount++;
        m_CurrentStats.PolyCount += model.Meshes[i].TriangleCount;

        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);

        if (activeShader)
        {
            BindShaderUniforms(activeShader.get(), boneMatrices, shaderUniformOverrides);
            BindMaterialUniforms(activeShader.get(), material, i, model, materialSlotOverrides);

            uint32_t originalShader = material.ShaderID;
            material.ShaderID = activeShader->GetShader().id;

            // CRITICAL@FIX: If using bone matrices, they already include the local node transform.
            bool useSkinning = !boneMatrices.empty();
            activeShader->SetInt("useSkinning", useSkinning ? 1 : 0);
            
            renderer.DrawMesh(model.Meshes[i], material, useSkinning ? transform : meshWorldTransform);

            material.ShaderID = originalShader;
        }
        else
        {
            renderer.DrawMesh(model.Meshes[i], material, meshWorldTransform);
        }
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
                    material.AlbedoMap = tex->GetTexture().id;
                }
            }

            // Update other properties as needed
            material.EmissiveIntensity = slot.Material.EmissiveIntensity;
            material.Metalness = slot.Material.Metalness;
            material.Roughness = slot.Material.Roughness;
            break;
        }
    }
    return material;
}

void SceneRenderer::BindShaderUniforms(ShaderAsset* activeShader, const std::vector<glm::mat4>& boneMatrices,
                                       const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!activeShader)
    {
        return;
    }
    if (!boneMatrices.empty())
    {
        int count = std::min((int)boneMatrices.size(), 128);
        activeShader->SetMatrices("boneMatrices", (const Matrix*)glm::value_ptr(boneMatrices[0]), count);
    }
    else
    {
        // For static meshes in animated models using fallback bones, we must provide identities
        // for all potential bone indices (up to 128 as per shader)
        static std::vector<glm::mat4> identities;
        if (identities.empty())
        {
            identities.assign(128, glm::mat4(1.0f));
        }
        activeShader->SetMatrices("boneMatrices", identities.data(), 128);
    }

    for (const auto& u : shaderUniformOverrides)
    {
        switch (u.Type)
        {
        case 0:
            activeShader->SetFloat(u.Name, u.Value[0]);
            break;
        case 1:
            activeShader->SetVec2(u.Name, {u.Value[0], u.Value[1]});
            break;
        case 2:
            activeShader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]});
            break;
        case 3:
            activeShader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
            break;
        case 4:
            activeShader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
            break;
        }
    }
}

void SceneRenderer::BindMaterialUniforms(ShaderAsset* activeShader, const Material& material, int meshIndex,
                                         const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
{
    if (!activeShader)
    {
        return;
    }
    activeShader->SetInt("useTexture", material.AlbedoMap > 0 ? 1 : 0);
    activeShader->SetVec4("colDiffuse", material.AlbedoColor);

    activeShader->SetInt("useNormalMap", material.NormalMap > 0 ? 1 : 0);
    activeShader->SetInt("useMetallicMap", material.MetallicRoughnessMap > 0 ? 1 : 0);
    activeShader->SetInt("useRoughnessMap", material.MetallicRoughnessMap > 0 ? 1 : 0);
    activeShader->SetInt("useOcclusionMap", material.OcclusionMap > 0 ? 1 : 0);
    activeShader->SetInt("useEmissiveTexture", material.EmissiveMap > 0 ? 1 : 0);

    float metalness = material.Metalness;
    float roughness = material.Roughness;
    glm::vec4 colEmissive = material.EmissiveColor;
    float emissiveIntensity = material.EmissiveIntensity;

    activeShader->SetFloat("metalness", metalness);
    activeShader->SetFloat("roughness", roughness);
    if (emissiveIntensity == 0.0f && (colEmissive.r > 0 || colEmissive.g > 0 || colEmissive.b > 0))
    {
        emissiveIntensity = 1.0f;
    }

    activeShader->SetVec4("colEmissive", colEmissive);
    activeShader->SetFloat("emissiveIntensity", emissiveIntensity);
}
} // namespace CHEngine
