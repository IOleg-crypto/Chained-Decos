#include "engine/graphics/draw_command.h"
#include "engine/core/profiler.h"
#include "engine/graphics/api_context.h"
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

RendererState &DrawCommand::GetState()
{
    return APIContext::GetState();
}

void DrawCommand::Init()
{
    // High-level init now handled by Visuals/APIContext
}

void DrawCommand::Shutdown()
{
    // High-level shutdown now handled by Visuals/APIContext
}

void DrawCommand::SetDirectionalLight(Vector3 direction, Color color)
{
    APIContext::SetDirectionalLight(direction, color);
}

void DrawCommand::SetAmbientLight(float intensity)
{
    APIContext::SetAmbientLight(intensity);
}

void DrawCommand::ApplyEnvironment(const EnvironmentSettings &settings)
{
    APIContext::ApplyEnvironment(settings);
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
        auto &state = APIContext::GetState();
        if (state.LightingShader)
            mat.shader = state.LightingShader->GetShader();

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

    auto &state = APIContext::GetState();
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
