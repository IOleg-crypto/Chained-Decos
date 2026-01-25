#include "render.h"
#include "engine/core/profiler.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/shader_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "raylib.h"
#include <filesystem>
#include <raymath.h>
#include <rlgl.h>

namespace CHEngine
{
RendererState Render::s_State;

void Render::Init()
{
    s_State.LightingShader = Assets::LoadShader("engine:shaders/lighting.chshader");

    if (s_State.LightingShader)
    {
        s_State.LightDirLoc = s_State.LightingShader->GetLocation("lightDir");
        s_State.LightColorLoc = s_State.LightingShader->GetLocation("lightColor");
        s_State.AmbientLoc = s_State.LightingShader->GetLocation("ambient");

        for (int i = 0; i < 8; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "].";
            s_State.LightLocs[i].position = s_State.LightingShader->GetLocation(base + "position");
            s_State.LightLocs[i].color = s_State.LightingShader->GetLocation(base + "color");
            s_State.LightLocs[i].radius = s_State.LightingShader->GetLocation(base + "radius");
            s_State.LightLocs[i].radiance = s_State.LightingShader->GetLocation(base + "radiance");
            s_State.LightLocs[i].falloff = s_State.LightingShader->GetLocation(base + "falloff");
            s_State.LightLocs[i].enabled = s_State.LightingShader->GetLocation(base + "enabled");
        }
    }

    float ambient = 0.3f;
    if (Project::GetActive())
        ambient = Project::GetActive()->GetConfig().Render.AmbientIntensity;

    SetDirectionalLight({-1.0f, -1.0f, -1.0f}, WHITE);
    SetAmbientLight(ambient);

    // Skybox initialization
    s_State.SkyboxShader = Assets::LoadShader("engine:shaders/skybox.chshader");
    s_State.SkyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    if (s_State.SkyboxShader)
    {
        auto &shader = s_State.SkyboxShader->GetShader();
        s_State.SkyboxCube.materials[0].shader = shader;
        s_State.SkyboxVflippedLoc = s_State.SkyboxShader->GetLocation("vflipped");
        s_State.SkyboxDoGammaLoc = s_State.SkyboxShader->GetLocation("doGamma");
        s_State.SkyboxFragGammaLoc = s_State.SkyboxShader->GetLocation("fragGamma");
        s_State.SkyboxExposureLoc = s_State.SkyboxShader->GetLocation("exposure");
        s_State.SkyboxBrightnessLoc = s_State.SkyboxShader->GetLocation("brightness");
        s_State.SkyboxContrastLoc = s_State.SkyboxShader->GetLocation("contrast");

        int environmentMapLoc = s_State.SkyboxShader->GetLocation("environmentMap");
        if (environmentMapLoc >= 0)
        {
            shader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
            int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
            SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
        }
    }

    // Panorama initialization
    s_State.PanoramaShader = Assets::LoadShader("engine:shaders/panorama.chshader");
    if (s_State.PanoramaShader)
    {
        s_State.PanoDoGammaLoc = s_State.PanoramaShader->GetLocation("doGamma");
        s_State.PanoFragGammaLoc = s_State.PanoramaShader->GetLocation("fragGamma");
        s_State.PanoExposureLoc = s_State.PanoramaShader->GetLocation("exposure");
        s_State.PanoBrightnessLoc = s_State.PanoramaShader->GetLocation("brightness");
        s_State.PanoContrastLoc = s_State.PanoramaShader->GetLocation("contrast");
    }
}

void Render::Shutdown()
{
    s_State.LightingShader = nullptr;
    s_State.SkyboxShader = nullptr;
    s_State.PanoramaShader = nullptr;
    UnloadModel(s_State.SkyboxCube);
}

void Render::BeginScene(const Camera3D &camera)
{
    s_State.ActiveCamera = camera;
    BeginMode3D(camera);
}

void Render::EndScene()
{
    EndMode3D();
}

void Render::BeginToTexture(RenderTexture2D target)
{
    BeginTextureMode(target);
}

void Render::EndToTexture()
{
    EndTextureMode();
}

void Render::DrawLine(Vector3 start, Vector3 end, Color color)
{
    ::DrawLine3D(start, end, color);
}

void Render::DrawModel(const std::string &path, const Matrix &transform, Color tint)
{
    MaterialInstance material;
    material.AlbedoColor = tint;
    DrawModel(path, transform, material);
}

void Render::DrawModel(const std::string &path, const Matrix &transform,
                       const std::vector<MaterialSlot> &overrides)
{
    auto asset = Assets::Get<ModelAsset>(path);
    if (!asset)
        return;

    DrawModel(asset, transform, overrides);
}

void Render::DrawModel(std::shared_ptr<ModelAsset> asset, const Matrix &transform,
                       const std::vector<MaterialSlot> &overrides)
{
    if (!asset)
        return;

    Model &model = asset->GetModel();
    rlPushMatrix();

    // Calculate final transform: Asset Root Transform * Entity Transform
    Matrix finalTransform = MatrixMultiply(model.transform, transform);
    rlMultMatrixf(MatrixToFloat(finalTransform));

    // Track statistics
    ProfilerStats stats;
    stats.DrawCalls++;
    stats.MeshCount += model.meshCount;
    for (int i = 0; i < model.meshCount; i++)
        stats.PolyCount += model.meshes[i].triangleCount;
    Profiler::UpdateStats(stats);

    for (int i = 0; i < model.meshCount; i++)
    {
        int matIndex = model.meshMaterial[i];
        // Use local copy of material to avoid polluting shared asset
        Material mat = model.materials[matIndex];

        if (s_State.LightingShader)
            mat.shader = s_State.LightingShader->GetShader();

        // Apply overrides if any match the slot index or name
        for (const auto &slot : overrides)
        {
            bool matches = (slot.Index == -1); // Global override
            if (slot.Target == MaterialSlotTarget::MaterialIndex)
                matches |= (slot.Index == matIndex);
            else if (slot.Target == MaterialSlotTarget::MeshIndex)
                matches |= (slot.Index == i);

            if (matches)
            {
                const auto &material = slot.Material;
                if (material.OverrideAlbedo)
                    mat.maps[MATERIAL_MAP_ALBEDO].color = material.AlbedoColor;

                if (!material.AlbedoPath.empty())
                {
                    auto texAsset = Assets::Get<TextureAsset>(material.AlbedoPath);
                    if (texAsset)
                        mat.maps[MATERIAL_MAP_ALBEDO].texture = texAsset->GetTexture();
                }

                if (material.OverrideNormal && !material.NormalMapPath.empty())
                {
                    auto texAsset = Assets::Get<TextureAsset>(material.NormalMapPath);
                    if (texAsset)
                        mat.maps[MATERIAL_MAP_NORMAL].texture = texAsset->GetTexture();
                }

                if (material.OverrideMetallicRoughness && !material.MetallicRoughnessPath.empty())
                {
                    auto texAsset = Assets::Get<TextureAsset>(material.MetallicRoughnessPath);
                    if (texAsset)
                        mat.maps[MATERIAL_MAP_ROUGHNESS].texture = texAsset->GetTexture();
                }

                if (material.OverrideEmissive && !material.EmissivePath.empty())
                {
                    auto texAsset = Assets::Get<TextureAsset>(material.EmissivePath);
                    if (texAsset)
                        mat.maps[MATERIAL_MAP_EMISSION].texture = texAsset->GetTexture();
                }
            }
        }

        CH_CORE_TRACE("Render: Drawing submesh {} of {} with material index {}", i, model.meshCount,
                      matIndex);
        DrawMesh(model.meshes[i], mat, MatrixIdentity());
    }

    rlPopMatrix();
}

void Render::DrawModel(const std::string &path, const Matrix &transform,
                       const MaterialInstance &material)
{
    std::vector<MaterialSlot> overrides;
    MaterialSlot slot;
    slot.Index = -1; // Override all slots
    slot.Material = material;
    overrides.push_back(slot);
    DrawModel(path, transform, overrides);
}

void Render::SetDirectionalLight(Vector3 direction, Color color)
{
    s_State.CurrentLightDir = direction;
    s_State.CurrentLightColor = color;

    float dir[3] = {direction.x, direction.y, direction.z};
    float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    if (s_State.LightingShader)
    {
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.LightDirLoc, dir,
                       SHADER_UNIFORM_VEC3);
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.LightColorLoc, col,
                       SHADER_UNIFORM_VEC4);
    }
}

void Render::SetAmbientLight(float intensity)
{
    s_State.CurrentAmbientIntensity = intensity;
    if (s_State.LightingShader)
    {
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.AmbientLoc, &intensity,
                       SHADER_UNIFORM_FLOAT);
    }
}

void Render::DrawScene(Scene *scene, const DebugRenderFlags *debugFlags)
{
    CH_PROFILE_FUNCTION();
    if (!scene)
        return;

    auto &registry = scene->GetRegistry();

    // Scene statistics
    ProfilerStats sceneStats;
    sceneStats.EntityCount = (uint32_t)registry.storage<entt::entity>().size();

    registry.view<ColliderComponent>().each(
        [&](auto entity, auto &col)
        {
            sceneStats.ColliderCount++;
            sceneStats.ColliderTypeCounts[(int)col.Type]++;
        });
    Profiler::UpdateStats(sceneStats);

    // Render loop ...
}

void Render::DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera)
{
    if (skybox.TexturePath.empty())
        return;

    auto texAsset = Assets::Get<TextureAsset>(skybox.TexturePath);
    if (!texAsset)
        return;

    Texture2D &tex = texAsset->GetTexture();

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

    std::shared_ptr<ShaderAsset> shaderAsset =
        usePanorama ? s_State.PanoramaShader : s_State.SkyboxShader;
    if (!shaderAsset)
        return;

    auto &shader = shaderAsset->GetShader();
    s_State.SkyboxCube.materials[0].shader = shader;

    // Set texture to material
    SetMaterialTexture(&s_State.SkyboxCube.materials[0],
                       usePanorama ? MATERIAL_MAP_ALBEDO : MATERIAL_MAP_CUBEMAP, tex);

    // Update shader uniforms
    int doGamma = 1;
    float fragGamma = 2.2f;

    if (usePanorama)
    {
        SetShaderValue(shader, s_State.PanoDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_State.PanoFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_State.PanoExposureLoc, &skybox.Exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_State.PanoBrightnessLoc, &skybox.Brightness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_State.PanoContrastLoc, &skybox.Contrast, SHADER_UNIFORM_FLOAT);
    }
    else
    {
        int vflipped = 0;
        SetShaderValue(shader, s_State.SkyboxVflippedLoc, &vflipped, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_State.SkyboxDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_State.SkyboxFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_State.SkyboxExposureLoc, &skybox.Exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_State.SkyboxBrightnessLoc, &skybox.Brightness,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_State.SkyboxContrastLoc, &skybox.Contrast, SHADER_UNIFORM_FLOAT);
    }

    // Disable backface culling for inside cube rendering
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // The vertex shader handles removing translation
    ::DrawModel(s_State.SkyboxCube, {0, 0, 0}, 1.0f, WHITE);

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void Render::ApplyEnvironment(const EnvironmentSettings &settings)
{
    SetDirectionalLight(settings.LightDirection, settings.LightColor);
    SetAmbientLight(settings.AmbientIntensity);

    // If we have an active skybox, it will be drawn with these settings
    // during DrawSkybox calls.
}

void Render::DrawSkybox(const EnvironmentSettings &settings, const Camera3D &camera)
{
    SkyboxComponent sc;
    sc.TexturePath = settings.Skybox.TexturePath;
    sc.Exposure = settings.Skybox.Exposure;
    sc.Brightness = settings.Skybox.Brightness;
    sc.Contrast = settings.Skybox.Contrast;

    DrawSkybox(sc, camera);
}

void Render::BeginUI()
{
    // Currently BeginDrawing is handled by Application,
    // but we might want custom UI state here.
}

void Render::EndUI()
{
}

void Render::DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height,
                             float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    // Set desired texture to be enabled while drawing following vertex data
    rlSetTexture(texture.id);

    // Vertex data transformation can be defined with the commented lines,
    // but in this example we calculate the transformed vertex data directly when calling
    // rlVertex3f()
    // rlPushMatrix();
    // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
    // rlTranslatef(2.0f, 0.0f, 0.0f);
    // rlRotatef(45, 0, 1, 0);
    // rlScalef(2.0f, 2.0f, 2.0f);

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);
    // Front Face (Flipped Vertical UVs)
    rlNormal3f(0.0f, 0.0f, 1.0f);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

    // Back Face (Flipped Vertical UVs)
    rlNormal3f(0.0f, 0.0f, -1.0f);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);

    // Top Face (Flipped Vertical UVs)
    rlNormal3f(0.0f, 1.0f, 0.0f);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);

    // Bottom Face (Flipped Vertical UVs)
    rlNormal3f(0.0f, -1.0f, 0.0f);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);

    // Right face (Flipped Vertical UVs)
    rlNormal3f(1.0f, 0.0f, 0.0f);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);

    // Left Face (Flipped Vertical UVs)
    rlNormal3f(-1.0f, 0.0f, 0.0f);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlEnd();

    rlSetTexture(0);
}

} // namespace CHEngine
