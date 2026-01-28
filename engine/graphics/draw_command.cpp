#include "engine/graphics/draw_command.h"
#include "engine/core/profiler.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/texture_asset.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "raymath.h"
#include "rlgl.h"
#include <filesystem>

namespace CHEngine
{
RendererState DrawCommand::s_State;

static void InitLights(RendererState &state)
{
    // state.LightingShader = AssetManager::Get<ShaderAsset>("engine:shaders/lighting.chshader");
    if (false) // state.LightingShader
    {
        state.LightDirLoc = state.LightingShader->GetLocation("lightDir");
        state.LightColorLoc = state.LightingShader->GetLocation("lightColor");
        state.AmbientLoc = state.LightingShader->GetLocation("ambient");

        for (int i = 0; i < 8; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "].";
            state.LightLocs[i].position = state.LightingShader->GetLocation(base + "position");
            state.LightLocs[i].color = state.LightingShader->GetLocation(base + "color");
            state.LightLocs[i].radius = state.LightingShader->GetLocation(base + "radius");
            state.LightLocs[i].radiance = state.LightingShader->GetLocation(base + "radiance");
            state.LightLocs[i].falloff = state.LightingShader->GetLocation(base + "falloff");
            state.LightLocs[i].enabled = state.LightingShader->GetLocation(base + "enabled");
        }
    }
}

static void InitSkybox(RendererState &state)
{
    // state.SkyboxShader = AssetManager::Get<ShaderAsset>("engine:shaders/skybox.chshader");
    state.SkyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    if (false) // state.SkyboxShader
    {
        auto &shader = state.SkyboxShader->GetShader();
        state.SkyboxCube.materials[0].shader = shader;
        state.SkyboxVflippedLoc = state.SkyboxShader->GetLocation("vflipped");
        state.SkyboxDoGammaLoc = state.SkyboxShader->GetLocation("doGamma");
        state.SkyboxFragGammaLoc = state.SkyboxShader->GetLocation("fragGamma");
        state.SkyboxExposureLoc = state.SkyboxShader->GetLocation("exposure");
        state.SkyboxBrightnessLoc = state.SkyboxShader->GetLocation("brightness");
        state.SkyboxContrastLoc = state.SkyboxShader->GetLocation("contrast");

        int environmentMapLoc = state.SkyboxShader->GetLocation("environmentMap");
        if (environmentMapLoc >= 0)
        {
            shader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
            int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
            SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
        }
    }
}

static void InitPanorama(RendererState &state)
{
    // state.PanoramaShader = AssetManager::Get<ShaderAsset>("engine:shaders/panorama.chshader");
    if (false) // state.PanoramaShader
    {
        state.PanoDoGammaLoc = state.PanoramaShader->GetLocation("doGamma");
        state.PanoFragGammaLoc = state.PanoramaShader->GetLocation("fragGamma");
        state.PanoExposureLoc = state.PanoramaShader->GetLocation("exposure");
        state.PanoBrightnessLoc = state.PanoramaShader->GetLocation("brightness");
        state.PanoContrastLoc = state.PanoramaShader->GetLocation("contrast");
    }
}

void DrawCommand::Init()
{
    InitLights(s_State);
    InitSkybox(s_State);
    InitPanorama(s_State);

    float ambient = 0.3f;
    if (Project::GetActive())
        ambient = Project::GetActive()->GetConfig().Render.AmbientIntensity;

    SetDirectionalLight({-1.0f, -1.0f, -1.0f}, WHITE);
    SetAmbientLight(ambient);
}

void DrawCommand::Shutdown()
{
    s_State.LightingShader = nullptr;
    s_State.SkyboxShader = nullptr;
    s_State.PanoramaShader = nullptr;
    UnloadModel(s_State.SkyboxCube);
}

void DrawCommand::SetDirectionalLight(Vector3 direction, Color color)
{
    s_State.CurrentLightDir = direction;
    s_State.CurrentLightColor = color;

    if (s_State.LightingShader)
    {
        float dir[3] = {direction.x, direction.y, direction.z};
        float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.LightDirLoc, dir,
                       SHADER_UNIFORM_VEC3);
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.LightColorLoc, col,
                       SHADER_UNIFORM_VEC4);
    }
}

void DrawCommand::SetAmbientLight(float intensity)
{
    s_State.CurrentAmbientIntensity = intensity;
    if (s_State.LightingShader)
    {
        SetShaderValue(s_State.LightingShader->GetShader(), s_State.AmbientLoc, &intensity,
                       SHADER_UNIFORM_FLOAT);
    }
}

void DrawCommand::ApplyEnvironment(const EnvironmentSettings &settings)
{
    SetDirectionalLight(settings.LightDirection, settings.LightColor);
    SetAmbientLight(settings.AmbientIntensity);
}
void DrawCommand::Clear(Color color)
{
    ::ClearBackground(color);
}

void DrawCommand::SetViewport(int x, int y, int width, int height)
{
    rlViewport(x, y, width, height);
}

static void ApplyMaterialOverrides(Material &mat, int meshIndex, int matIndex,
                                   const std::vector<MaterialSlot> &overrides)
{
    for (const auto &slot : overrides)
    {
        bool matches = (slot.Index == -1);
        if (slot.Target == MaterialSlotTarget::MaterialIndex)
            matches |= (slot.Index == matIndex);
        else if (slot.Target == MaterialSlotTarget::MeshIndex)
            matches |= (slot.Index == meshIndex);

        if (matches)
        {
            const auto &material = slot.Material;

            // Apply custom shader if provided
            if (material.OverrideShader && !material.ShaderPath.empty())
            {
                auto shaderAsset = ShaderAsset::Load(material.ShaderPath);
                if (shaderAsset)
                    mat.shader = shaderAsset->GetShader();
            }

            if (material.OverrideAlbedo)
                mat.maps[MATERIAL_MAP_ALBEDO].color = material.AlbedoColor;

            if (!material.AlbedoPath.empty())
            {
                // auto texAsset = AssetManager::Get<TextureAsset>(material.AlbedoPath);
                // if (texAsset)
                //     mat.maps[MATERIAL_MAP_ALBEDO].texture = texAsset->GetTexture();
            }

            if (material.OverrideNormal && !material.NormalMapPath.empty())
            {
                // auto texAsset = AssetManager::Get<TextureAsset>(material.NormalMapPath);
                // if (texAsset)
                //     mat.maps[MATERIAL_MAP_NORMAL].texture = texAsset->GetTexture();
            }
        }
    }
}

void DrawCommand::DrawModel(const std::string &path, const Matrix &transform,
                            const std::vector<MaterialSlot> &overrides)
{
    // auto asset = AssetManager::Get<ModelAsset>(path);
    std::shared_ptr<ModelAsset> asset = nullptr;
    if (!asset)
        return;

    Model &model = asset->GetModel();

    rlPushMatrix();
    Matrix finalTransform = MatrixMultiply(model.transform, transform);
    rlMultMatrixf(MatrixToFloat(finalTransform));

    // Stats
    ProfilerStats stats;
    stats.DrawCalls++;
    stats.MeshCount += model.meshCount;
    for (int i = 0; i < model.meshCount; i++)
        stats.PolyCount += model.meshes[i].triangleCount;
    Profiler::UpdateStats(stats);

    for (int i = 0; i < model.meshCount; i++)
    {
        int matIndex = model.meshMaterial[i];
        Material mat = model.materials[matIndex]; // Copy

        // Default global lighting shader if none specified in materials
        if (s_State.LightingShader)
            mat.shader = s_State.LightingShader->GetShader();

        ApplyMaterialOverrides(mat, i, matIndex, overrides);
        ::DrawMesh(model.meshes[i], mat, MatrixIdentity());
    }

    rlPopMatrix();
}

void DrawCommand::DrawLine(Vector3 start, Vector3 end, Color color)
{
    ::DrawLine3D(start, end, color);
}

void DrawCommand::DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera)
{
    if (skybox.TexturePath.empty())
        return;

    // auto texAsset = AssetManager::Get<TextureAsset>(skybox.TexturePath);
    std::shared_ptr<TextureAsset> texAsset = nullptr;
    if (!texAsset)
        return;

    auto &state = s_State;
    bool usePanorama = std::filesystem::path(skybox.TexturePath).extension() != ".hdr";

    std::shared_ptr<ShaderAsset> shaderAsset =
        usePanorama ? state.PanoramaShader : state.SkyboxShader;
    if (!shaderAsset)
        return;

    auto &shader = shaderAsset->GetShader();
    state.SkyboxCube.materials[0].shader = shader;
    SetMaterialTexture(&state.SkyboxCube.materials[0],
                       usePanorama ? MATERIAL_MAP_ALBEDO : MATERIAL_MAP_CUBEMAP,
                       texAsset->GetTexture());

    // Set Uniforms
    int doGamma = 1;
    float fragGamma = 2.2f;
    if (usePanorama)
    {
        SetShaderValue(shader, state.PanoDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, state.PanoFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, state.PanoExposureLoc, &skybox.Exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, state.PanoBrightnessLoc, &skybox.Brightness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, state.PanoContrastLoc, &skybox.Contrast, SHADER_UNIFORM_FLOAT);
    }
    else
    {
        int vflipped = 0;
        SetShaderValue(shader, state.SkyboxVflippedLoc, &vflipped, SHADER_UNIFORM_INT);
        SetShaderValue(shader, state.SkyboxDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, state.SkyboxFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, state.SkyboxExposureLoc, &skybox.Exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, state.SkyboxBrightnessLoc, &skybox.Brightness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, state.SkyboxContrastLoc, &skybox.Contrast, SHADER_UNIFORM_FLOAT);
    }

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    ::DrawModel(state.SkyboxCube, {0, 0, 0}, 1.0f, WHITE);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void DrawCommand::DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height,
                                  float length, Color color)
{
    float x = position.x, y = position.y, z = position.z;
    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    // Front Face
    rlNormal3f(0.0f, 0.0f, 1.0f);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

    // Back Face
    rlNormal3f(0.0f, 0.0f, -1.0f);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);

    // Top Face
    rlNormal3f(0.0f, 1.0f, 0.0f);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);

    // Bottom Face
    rlNormal3f(0.0f, -1.0f, 0.0f);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);

    // Right Face
    rlNormal3f(1.0f, 0.0f, 0.0f);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);

    // Left Face
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
