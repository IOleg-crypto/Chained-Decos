#include "renderer.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <filesystem>
#include <raymath.h>
#include <rlgl.h>

namespace CH
{
Renderer::LightState Renderer::s_LightState;
Camera3D Renderer::s_CurrentCamera;

void Renderer::Init()
{
    s_LightState.lightingShader = LoadShader(PROJECT_ROOT_DIR "/assets/shaders/lighting.vs",
                                             PROJECT_ROOT_DIR "/assets/shaders/lighting.fs");
    s_LightState.lightDirLoc = GetShaderLocation(s_LightState.lightingShader, "lightDir");
    s_LightState.lightColorLoc = GetShaderLocation(s_LightState.lightingShader, "lightColor");
    s_LightState.ambientLoc = GetShaderLocation(s_LightState.lightingShader, "ambient");

    SetDirectionalLight({-1.0f, -1.0f, -1.0f}, WHITE);
    SetAmbientLight(0.3f);

    // Skybox initialization
    s_LightState.skyboxShader = LoadShader(PROJECT_ROOT_DIR "/assets/shaders/skybox.vs",
                                           PROJECT_ROOT_DIR "/assets/shaders/skybox.fs");
    s_LightState.skyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    s_LightState.skyboxCube.materials[0].shader = s_LightState.skyboxShader;

    s_LightState.skyboxVflippedLoc = GetShaderLocation(s_LightState.skyboxShader, "vflipped");
    s_LightState.skyboxDoGammaLoc = GetShaderLocation(s_LightState.skyboxShader, "doGamma");
    s_LightState.skyboxFragGammaLoc = GetShaderLocation(s_LightState.skyboxShader, "fragGamma");
    s_LightState.skyboxExposureLoc = GetShaderLocation(s_LightState.skyboxShader, "exposure");
    s_LightState.skyboxBrightnessLoc = GetShaderLocation(s_LightState.skyboxShader, "brightness");
    s_LightState.skyboxContrastLoc = GetShaderLocation(s_LightState.skyboxShader, "contrast");

    int environmentMapLoc = GetShaderLocation(s_LightState.skyboxShader, "environmentMap");
    if (environmentMapLoc >= 0)
    {
        s_LightState.skyboxShader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
        int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
        SetShaderValue(s_LightState.skyboxShader, environmentMapLoc, envMapValue,
                       SHADER_UNIFORM_INT);
    }

    // Panorama initialization
    s_LightState.panoramaShader = LoadShader(PROJECT_ROOT_DIR "/assets/shaders/skybox.vs",
                                             PROJECT_ROOT_DIR "/assets/shaders/skybox_panorama.fs");
    s_LightState.panoDoGammaLoc = GetShaderLocation(s_LightState.panoramaShader, "doGamma");
    s_LightState.panoFragGammaLoc = GetShaderLocation(s_LightState.panoramaShader, "fragGamma");
    s_LightState.panoExposureLoc = GetShaderLocation(s_LightState.panoramaShader, "exposure");
    s_LightState.panoBrightnessLoc = GetShaderLocation(s_LightState.panoramaShader, "brightness");
    s_LightState.panoContrastLoc = GetShaderLocation(s_LightState.panoramaShader, "contrast");
}

void Renderer::Shutdown()
{
    UnloadShader(s_LightState.lightingShader);
    UnloadShader(s_LightState.skyboxShader);
    UnloadModel(s_LightState.skyboxCube);
}

void Renderer::BeginScene(const Camera3D &camera)
{
    s_CurrentCamera = camera;
    BeginMode3D(camera);
}

void Renderer::EndScene()
{
    EndMode3D();
}

void Renderer::DrawGrid(int slices, float spacing)
{
    ::DrawGrid(slices, spacing);
}

void Renderer::DrawLine(Vector3 start, Vector3 end, Color color)
{
    ::DrawLine3D(start, end, color);
}

void Renderer::DrawModel(const std::string &path, const Matrix &transform, Color tint)
{
    Model model = AssetManager::LoadModel(path);
    if (model.meshCount > 0)
    {
        // Apply our lighting shader to all materials in the model
        for (int i = 0; i < model.materialCount; i++)
        {
            model.materials[i].shader = s_LightState.lightingShader;
        }

        Matrix originalTransform = model.transform;
        model.transform = transform;
        ::DrawModel(model, {0, 0, 0}, 1.0f, tint);
        model.transform = originalTransform;
    }
}

void Renderer::DrawModel(const std::string &path, const Matrix &transform,
                         const MaterialComponent &material)
{
    Model model = AssetManager::LoadModel(path);
    if (model.meshCount > 0)
    {
        // Apply our lighting shader to all materials in the model
        for (int i = 0; i < model.materialCount; i++)
        {
            model.materials[i].shader = s_LightState.lightingShader;

            // Apply material properties (TEMPORARY - will affect all instances of this model)
            // TODO: Use a per-instance material system
            model.materials[i].maps[MATERIAL_MAP_ALBEDO].color = material.AlbedoColor;

            if (!material.AlbedoPath.empty())
            {
                Texture2D tex = AssetManager::LoadTexture(material.AlbedoPath);
                if (tex.id > 0)
                {
                    model.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = tex;
                }
            }
        }

        Matrix originalTransform = model.transform;
        model.transform = transform;
        ::DrawModel(model, {0, 0, 0}, 1.0f, WHITE);
        model.transform = originalTransform;
    }
}

void Renderer::DrawScene(Scene *scene)
{
    // Draw Skybox

    DrawSkybox(scene->GetSkybox(), s_CurrentCamera);

    // Draw models
    {
        auto view = scene->GetRegistry().view<TransformComponent, ModelComponent>();
        for (auto entity : view)
        {
            auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

            Entity e{entity, scene};
            if (e.HasComponent<MaterialComponent>())
            {
                DrawModel(model.ModelPath, transform.GetTransform(),
                          e.GetComponent<MaterialComponent>());
            }
            else
            {
                DrawModel(model.ModelPath, transform.GetTransform(), model.Tint);
            }
        }
    }

    // Draw Spawn Zones (Gizmo style)

    auto view = scene->GetRegistry().view<TransformComponent, SpawnComponent>();
    for (auto entity : view)
    {
        auto [transform, spawn] = view.get<TransformComponent, SpawnComponent>(entity);

        // Draw a translucent cyan box for the spawn zone
        Vector3 pos = transform.Translation;
        Vector3 size = spawn.ZoneSize;

        if (spawn.RenderSpawnZoneInScene)
        {
            ::DrawCubeWires(pos, size.x, size.y, size.z, Color{0, 255, 255, 255});
            ::DrawCube(pos, size.x, size.y, size.z, ColorAlpha(Color{0, 255, 255, 255}, 0.2f));
        }
    }

    // Draw Colliders (Gizmo style)
    {
        auto view = scene->GetRegistry().view<TransformComponent, ColliderComponent>();
        for (auto entity : view)
        {
            auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);

            Vector3 min = Vector3Add(transform.Translation, collider.Offset);
            Vector3 size = collider.Size;
            Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

            Color color = collider.IsColliding ? RED : GREEN;
            ::DrawCubeWires(center, size.x, size.y, size.z, color);
        }
    }
}

void Renderer::SetDirectionalLight(Vector3 direction, Color color)
{
    s_LightState.lightDir = direction;
    s_LightState.lightColor = color;

    float dir[3] = {direction.x, direction.y, direction.z};
    float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    SetShaderValue(s_LightState.lightingShader, s_LightState.lightDirLoc, dir, SHADER_UNIFORM_VEC3);
    SetShaderValue(s_LightState.lightingShader, s_LightState.lightColorLoc, col,
                   SHADER_UNIFORM_VEC4);
}

void Renderer::SetAmbientLight(float intensity)
{
    s_LightState.ambient = intensity;
    SetShaderValue(s_LightState.lightingShader, s_LightState.ambientLoc, &intensity,
                   SHADER_UNIFORM_FLOAT);
}

void Renderer::DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera)
{
    if (skybox.TexturePath.empty())
        return;

    Texture2D tex = AssetManager::LoadTexture(skybox.TexturePath);
    if (tex.id == 0)
        return;

    // We don't have an easy way to check if it's a cubemap in Raylib Texture struct,
    // so we assume PNG/JPG/BMP are 2D panoramas and .hdr/special cubemaps are cubemaps.
    // Actually, for now let's just use the panorama shader for everything that isn't
    // explicitly a cubemap.
    bool usePanorama = true;
    std::string ext = std::filesystem::path(skybox.TexturePath).extension().string();
    if (ext == ".hdr")
        usePanorama = false; // HDRs are often cubemaps in some setups, but here let's be careful

    // Actually, let's just try to be smart: if it's a regular LoadTexture, it's 2D.
    // If we'll have a LoadCubemap later that sets a flag, we'll use that.

    Shader shader = usePanorama ? s_LightState.panoramaShader : s_LightState.skyboxShader;
    s_LightState.skyboxCube.materials[0].shader = shader;

    // Set texture to material
    SetMaterialTexture(&s_LightState.skyboxCube.materials[0],
                       usePanorama ? MATERIAL_MAP_ALBEDO : MATERIAL_MAP_CUBEMAP, tex);

    // Update shader uniforms
    int doGamma = 1;
    float fragGamma = 2.2f;

    if (usePanorama)
    {
        SetShaderValue(shader, s_LightState.panoDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_LightState.panoFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_LightState.panoExposureLoc, &skybox.Exposure,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_LightState.panoBrightnessLoc, &skybox.Brightness,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_LightState.panoContrastLoc, &skybox.Contrast,
                       SHADER_UNIFORM_FLOAT);
    }
    else
    {
        int vflipped = 0;
        SetShaderValue(shader, s_LightState.skyboxVflippedLoc, &vflipped, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_LightState.skyboxDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_LightState.skyboxFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_LightState.skyboxExposureLoc, &skybox.Exposure,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_LightState.skyboxBrightnessLoc, &skybox.Brightness,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_LightState.skyboxContrastLoc, &skybox.Contrast,
                       SHADER_UNIFORM_FLOAT);
    }

    // Disable backface culling for inside cube rendering
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // The vertex shader handles removing translation
    ::DrawModel(s_LightState.skyboxCube, {0, 0, 0}, 1.0f, WHITE);

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void Renderer::BeginUI()
{
    // Currently BeginDrawing is handled by Application,
    // but we might want custom UI state here.
}

void Renderer::EndUI()
{
}
} // namespace CH
