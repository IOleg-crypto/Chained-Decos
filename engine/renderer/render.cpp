#include "render.h"
#include "engine/renderer/asset_manager.h"
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
    s_Shaders.lightingShader = LoadShader(PROJECT_ROOT_DIR "/assets/shaders/lighting.vs",
                                          PROJECT_ROOT_DIR "/assets/shaders/lighting.fs");
    s_Shaders.lightDirLoc = GetShaderLocation(s_Shaders.lightingShader, "lightDir");
    s_Shaders.lightColorLoc = GetShaderLocation(s_Shaders.lightingShader, "lightColor");
    s_Shaders.ambientLoc = GetShaderLocation(s_Shaders.lightingShader, "ambient");

    for (int i = 0; i < 8; i++)
    {
        std::string base = "lights[" + std::to_string(i) + "].";
        s_Shaders.lightLocs[i].position =
            GetShaderLocation(s_Shaders.lightingShader, (base + "position").c_str());
        s_Shaders.lightLocs[i].color =
            GetShaderLocation(s_Shaders.lightingShader, (base + "color").c_str());
        s_Shaders.lightLocs[i].radius =
            GetShaderLocation(s_Shaders.lightingShader, (base + "radius").c_str());
        s_Shaders.lightLocs[i].radiance =
            GetShaderLocation(s_Shaders.lightingShader, (base + "radiance").c_str());
        s_Shaders.lightLocs[i].falloff =
            GetShaderLocation(s_Shaders.lightingShader, (base + "falloff").c_str());
        s_Shaders.lightLocs[i].enabled =
            GetShaderLocation(s_Shaders.lightingShader, (base + "enabled").c_str());
    }

    SetDirectionalLight({-1.0f, -1.0f, -1.0f}, WHITE);
    SetAmbientLight(0.3f);

    // Skybox initialization
    s_Shaders.skyboxShader = LoadShader(PROJECT_ROOT_DIR "/assets/shaders/skybox.vs",
                                        PROJECT_ROOT_DIR "/assets/shaders/skybox.fs");
    s_Shaders.skyboxCube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    s_Shaders.skyboxCube.materials[0].shader = s_Shaders.skyboxShader;

    s_Shaders.skyboxVflippedLoc = GetShaderLocation(s_Shaders.skyboxShader, "vflipped");
    s_Shaders.skyboxDoGammaLoc = GetShaderLocation(s_Shaders.skyboxShader, "doGamma");
    s_Shaders.skyboxFragGammaLoc = GetShaderLocation(s_Shaders.skyboxShader, "fragGamma");
    s_Shaders.skyboxExposureLoc = GetShaderLocation(s_Shaders.skyboxShader, "exposure");
    s_Shaders.skyboxBrightnessLoc = GetShaderLocation(s_Shaders.skyboxShader, "brightness");
    s_Shaders.skyboxContrastLoc = GetShaderLocation(s_Shaders.skyboxShader, "contrast");

    int environmentMapLoc = GetShaderLocation(s_Shaders.skyboxShader, "environmentMap");
    if (environmentMapLoc >= 0)
    {
        s_Shaders.skyboxShader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
        int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
        SetShaderValue(s_Shaders.skyboxShader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
    }

    // Panorama initialization
    s_Shaders.panoramaShader = LoadShader(PROJECT_ROOT_DIR "/assets/shaders/skybox.vs",
                                          PROJECT_ROOT_DIR "/assets/shaders/skybox_panorama.fs");
    s_Shaders.panoDoGammaLoc = GetShaderLocation(s_Shaders.panoramaShader, "doGamma");
    s_Shaders.panoFragGammaLoc = GetShaderLocation(s_Shaders.panoramaShader, "fragGamma");
    s_Shaders.panoExposureLoc = GetShaderLocation(s_Shaders.panoramaShader, "exposure");
    s_Shaders.panoBrightnessLoc = GetShaderLocation(s_Shaders.panoramaShader, "brightness");
    s_Shaders.panoContrastLoc = GetShaderLocation(s_Shaders.panoramaShader, "contrast");

    // Infinite Grid initialization
    s_Shaders.gridShader = LoadShader(PROJECT_ROOT_DIR "/resources/shaders/infinite_grid.vs",
                                      PROJECT_ROOT_DIR "/resources/shaders/infinite_grid.fs");
    s_Shaders.gridNearLoc = GetShaderLocation(s_Shaders.gridShader, "near");
    s_Shaders.gridFarLoc = GetShaderLocation(s_Shaders.gridShader, "far");
    s_Shaders.gridViewLoc = GetShaderLocation(s_Shaders.gridShader, "matView");
    s_Shaders.gridProjLoc = GetShaderLocation(s_Shaders.gridShader, "matProjection");

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
    UnloadShader(s_Shaders.lightingShader);
    UnloadShader(s_Shaders.skyboxShader);
    UnloadShader(s_Shaders.gridShader);
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
    Model model = AssetManager::LoadModel(path);
    if (model.meshCount > 0)
    {
        // Apply our lighting shader to all materials in the model
        for (int i = 0; i < model.materialCount; i++)
        {
            model.materials[i].shader = s_Shaders.lightingShader;
        }

        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        ::DrawModelEx(model, {0, 0, 0}, {1, 0, 0}, 0.0f, scale, tint);
        rlPopMatrix();
    }
}

void Render::DrawModel(const std::string &path, const Matrix &transform,
                       const MaterialComponent &material, Vector3 scale)
{
    Model model = AssetManager::LoadModel(path);
    if (model.meshCount > 0)
    {
        // Apply our lighting shader to all materials in the model
        for (int i = 0; i < model.materialCount; i++)
        {
            model.materials[i].shader = s_Shaders.lightingShader;

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

        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(transform));
        ::DrawModelEx(model, {0, 0, 0}, {1, 0, 0}, 0.0f, scale, WHITE);
        rlPopMatrix();
    }
}

void Render::SetDirectionalLight(Vector3 direction, Color color)
{
    s_Scene.lightDir = direction;
    s_Scene.lightColor = color;

    float dir[3] = {direction.x, direction.y, direction.z};
    float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    SetShaderValue(s_Shaders.lightingShader, s_Shaders.lightDirLoc, dir, SHADER_UNIFORM_VEC3);
    SetShaderValue(s_Shaders.lightingShader, s_Shaders.lightColorLoc, col, SHADER_UNIFORM_VEC4);
}

void Render::SetAmbientLight(float intensity)
{
    s_Scene.ambientIntensity = intensity;
    SetShaderValue(s_Shaders.lightingShader, s_Shaders.ambientLoc, &intensity,
                   SHADER_UNIFORM_FLOAT);
}

void Render::DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera)
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

    Shader shader = usePanorama ? s_Shaders.panoramaShader : s_Shaders.skyboxShader;
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
