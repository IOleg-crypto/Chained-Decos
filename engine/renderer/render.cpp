#include "render.h"
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
Render::ShaderState Render::s_Shaders;
Render::SceneData Render::s_Scene;

void Render::Init()
{
    s_Shaders.lightingShader =
        Assets::LoadShader("engine:shaders/lighting.vs", "engine:shaders/lighting.fs");

    if (s_Shaders.lightingShader)
    {
        auto &shader = s_Shaders.lightingShader->GetShader();
        s_Shaders.lightDirLoc = GetShaderLocation(shader, "lightDir");
        s_Shaders.lightColorLoc = GetShaderLocation(shader, "lightColor");
        s_Shaders.ambientLoc = GetShaderLocation(shader, "ambient");

        for (int i = 0; i < 8; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "].";
            s_Shaders.lightLocs[i].position =
                GetShaderLocation(shader, (base + "position").c_str());
            s_Shaders.lightLocs[i].color = GetShaderLocation(shader, (base + "color").c_str());
            s_Shaders.lightLocs[i].radius = GetShaderLocation(shader, (base + "radius").c_str());
            s_Shaders.lightLocs[i].radiance =
                GetShaderLocation(shader, (base + "radiance").c_str());
            s_Shaders.lightLocs[i].falloff = GetShaderLocation(shader, (base + "falloff").c_str());
            s_Shaders.lightLocs[i].enabled = GetShaderLocation(shader, (base + "enabled").c_str());
        }
    }

    float ambient = 0.3f;
    if (Project::GetActive())
        ambient = Project::GetActive()->GetConfig().Render.AmbientIntensity;

    SetDirectionalLight({-1.0f, -1.0f, -1.0f}, WHITE);
    SetAmbientLight(ambient);

    // Skybox initialization
    s_Shaders.skyboxShader =
        Assets::LoadShader("engine:shaders/skybox.vs", "engine:shaders/skybox.fs");
    s_Shaders.skyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    if (s_Shaders.skyboxShader)
    {
        auto &shader = s_Shaders.skyboxShader->GetShader();
        s_Shaders.skyboxCube.materials[0].shader = shader;
        s_Shaders.skyboxVflippedLoc = GetShaderLocation(shader, "vflipped");
        s_Shaders.skyboxDoGammaLoc = GetShaderLocation(shader, "doGamma");
        s_Shaders.skyboxFragGammaLoc = GetShaderLocation(shader, "fragGamma");
        s_Shaders.skyboxExposureLoc = GetShaderLocation(shader, "exposure");
        s_Shaders.skyboxBrightnessLoc = GetShaderLocation(shader, "brightness");
        s_Shaders.skyboxContrastLoc = GetShaderLocation(shader, "contrast");

        int environmentMapLoc = GetShaderLocation(shader, "environmentMap");
        if (environmentMapLoc >= 0)
        {
            shader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
            int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
            SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
        }
    }

    // Panorama initialization
    s_Shaders.panoramaShader =
        Assets::LoadShader("engine:shaders/skybox.vs", "engine:shaders/skybox_panorama.fs");
    if (s_Shaders.panoramaShader)
    {
        auto &shader = s_Shaders.panoramaShader->GetShader();
        s_Shaders.panoDoGammaLoc = GetShaderLocation(shader, "doGamma");
        s_Shaders.panoFragGammaLoc = GetShaderLocation(shader, "fragGamma");
        s_Shaders.panoExposureLoc = GetShaderLocation(shader, "exposure");
        s_Shaders.panoBrightnessLoc = GetShaderLocation(shader, "brightness");
        s_Shaders.panoContrastLoc = GetShaderLocation(shader, "contrast");
    }

    // Infinite Grid initialization
    s_Shaders.gridShader =
        Assets::LoadShader("engine:shaders/infinite_grid.vs", "engine:shaders/infinite_grid.fs");
    if (s_Shaders.gridShader)
    {
        auto &shader = s_Shaders.gridShader->GetShader();
        s_Shaders.gridNearLoc = GetShaderLocation(shader, "near");
        s_Shaders.gridFarLoc = GetShaderLocation(shader, "far");
        s_Shaders.gridViewLoc = GetShaderLocation(shader, "matView");
        s_Shaders.gridProjLoc = GetShaderLocation(shader, "matProjection");
    }

    // Create fullscreen quad for grid using rlgl
    float gridVertices[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
                            -1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f};

    s_Shaders.gridVAO = rlLoadVertexArray();
    rlEnableVertexArray(s_Shaders.gridVAO);

    s_Shaders.gridVBO = rlLoadVertexBuffer(gridVertices, sizeof(gridVertices), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
    rlEnableVertexAttribute(0);

    rlDisableVertexArray();
}

void Render::Shutdown()
{
    s_Shaders.lightingShader = nullptr;
    s_Shaders.skyboxShader = nullptr;
    s_Shaders.panoramaShader = nullptr;
    s_Shaders.gridShader = nullptr;
    UnloadModel(s_Shaders.skyboxCube);

    if (s_Shaders.gridVAO != 0)
    {
        rlUnloadVertexArray(s_Shaders.gridVAO);
        rlUnloadVertexBuffer(s_Shaders.gridVBO);
    }
}

void Render::BeginScene(const Camera3D &camera)
{
    s_Scene.currentCamera = camera;
    BeginMode3D(camera);
}

void Render::EndScene()
{
    EndMode3D();
}

void Render::DrawGrid(int slices, float spacing)
{
    // Simple line-based grid using rlgl for performance and control
    rlCheckRenderBatchLimit(slices * 4);

    rlBegin(RL_LINES);
    rlColor4ub(100, 100, 100, 255);

    float halfSize = (float)slices * spacing / 2.0f;

    for (int i = 0; i <= slices; i++)
    {
        float pos = -halfSize + (float)i * spacing;

        // X-axis lines
        rlVertex3f(-halfSize, 0.0f, pos);
        rlVertex3f(halfSize, 0.0f, pos);

        // Z-axis lines
        rlVertex3f(pos, 0.0f, -halfSize);
        rlVertex3f(pos, 0.0f, halfSize);
    }
    rlEnd();
}

void Render::DrawLine(Vector3 start, Vector3 end, Color color)
{
    ::DrawLine3D(start, end, color);
}

void Render::DrawModel(const std::string &path, const Matrix &transform, Color tint, Vector3 scale)
{
    MaterialInstance material;
    material.AlbedoColor = tint;
    DrawModel(path, transform, material, scale);
}

void Render::DrawModel(const std::string &path, const Matrix &transform,
                       const std::vector<MaterialSlot> &overrides, Vector3 scale)
{
    auto asset = Assets::LoadModel(path);
    if (asset)
    {
        Model &model = asset->GetModel();
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));

        for (int i = 0; i < model.meshCount; i++)
        {
            int matIndex = model.meshMaterial[i];
            // Use local copy of material to avoid polluting shared asset
            Material mat = model.materials[matIndex];

            if (s_Shaders.lightingShader)
                mat.shader = s_Shaders.lightingShader->GetShader();

            // Find override for this slot
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
                    // Only apply if override flag is set
                    if (material.OverrideAlbedo)
                    {
                        mat.maps[MATERIAL_MAP_ALBEDO].color = material.AlbedoColor;
                        if (!material.AlbedoPath.empty())
                        {
                            auto texAsset = Assets::LoadTexture(material.AlbedoPath);
                            if (texAsset)
                                mat.maps[MATERIAL_MAP_ALBEDO].texture = texAsset->GetTexture();
                        }
                    }

                    if (material.OverrideNormal && !material.NormalMapPath.empty())
                    {
                        auto texAsset = Assets::LoadTexture(material.NormalMapPath);
                        if (texAsset)
                            mat.maps[MATERIAL_MAP_NORMAL].texture = texAsset->GetTexture();
                    }

                    if (material.OverrideMetallicRoughness &&
                        !material.MetallicRoughnessPath.empty())
                    {
                        auto texAsset = Assets::LoadTexture(material.MetallicRoughnessPath);
                        if (texAsset)
                            mat.maps[MATERIAL_MAP_ROUGHNESS].texture = texAsset->GetTexture();
                    }

                    if (material.OverrideEmissive && !material.EmissivePath.empty())
                    {
                        auto texAsset = Assets::LoadTexture(material.EmissivePath);
                        if (texAsset)
                            mat.maps[MATERIAL_MAP_EMISSION].texture = texAsset->GetTexture();
                    }
                }
            }

            DrawMesh(model.meshes[i], mat, MatrixIdentity());
        }
        rlPopMatrix();
    }
}

void Render::DrawModel(const std::string &path, const Matrix &transform,
                       const MaterialInstance &material, Vector3 scale)
{
    std::vector<MaterialSlot> overrides;
    MaterialSlot slot;
    slot.Index = -1; // Override all slots
    slot.Material = material;
    overrides.push_back(slot);
    DrawModel(path, transform, overrides, scale);
}

void Render::SetDirectionalLight(Vector3 direction, Color color)
{
    s_Scene.lightDir = direction;
    s_Scene.lightColor = color;

    float dir[3] = {direction.x, direction.y, direction.z};
    float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    if (s_Shaders.lightingShader)
    {
        SetShaderValue(s_Shaders.lightingShader->GetShader(), s_Shaders.lightDirLoc, dir,
                       SHADER_UNIFORM_VEC3);
        SetShaderValue(s_Shaders.lightingShader->GetShader(), s_Shaders.lightColorLoc, col,
                       SHADER_UNIFORM_VEC4);
    }
}

void Render::SetAmbientLight(float intensity)
{
    s_Scene.ambientIntensity = intensity;
    if (s_Shaders.lightingShader)
    {
        SetShaderValue(s_Shaders.lightingShader->GetShader(), s_Shaders.ambientLoc, &intensity,
                       SHADER_UNIFORM_FLOAT);
    }
}

void Render::DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera)
{
    if (skybox.TexturePath.empty())
        return;

    auto texAsset = Assets::LoadTexture(skybox.TexturePath);
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

    Ref<ShaderAsset> shaderAsset = usePanorama ? s_Shaders.panoramaShader : s_Shaders.skyboxShader;
    if (!shaderAsset)
        return;

    auto &shader = shaderAsset->GetShader();
    s_Shaders.skyboxCube.materials[0].shader = shader;

    // Set texture to material
    SetMaterialTexture(&s_Shaders.skyboxCube.materials[0],
                       usePanorama ? MATERIAL_MAP_ALBEDO : MATERIAL_MAP_CUBEMAP, tex);

    // Update shader uniforms
    int doGamma = 1;
    float fragGamma = 2.2f;

    if (usePanorama)
    {
        SetShaderValue(shader, s_Shaders.panoDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_Shaders.panoFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_Shaders.panoExposureLoc, &skybox.Exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_Shaders.panoBrightnessLoc, &skybox.Brightness,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_Shaders.panoContrastLoc, &skybox.Contrast, SHADER_UNIFORM_FLOAT);
    }
    else
    {
        int vflipped = 0;
        SetShaderValue(shader, s_Shaders.skyboxVflippedLoc, &vflipped, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_Shaders.skyboxDoGammaLoc, &doGamma, SHADER_UNIFORM_INT);
        SetShaderValue(shader, s_Shaders.skyboxFragGammaLoc, &fragGamma, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_Shaders.skyboxExposureLoc, &skybox.Exposure, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_Shaders.skyboxBrightnessLoc, &skybox.Brightness,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, s_Shaders.skyboxContrastLoc, &skybox.Contrast, SHADER_UNIFORM_FLOAT);
    }

    // Disable backface culling for inside cube rendering
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // The vertex shader handles removing translation
    ::DrawModel(s_Shaders.skyboxCube, {0, 0, 0}, 1.0f, WHITE);

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void Render::BeginUI()
{
    // Currently BeginDrawing is handled by Application,
    // but we might want custom UI state here.
}

void Render::EndUI()
{
}

} // namespace CHEngine
