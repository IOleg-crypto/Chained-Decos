#include "Skybox.h"
#include "core/config/ConfigManager.h"
#include "rlImGui/rlImGui.h"
#include "rlgl.h"
#include <filesystem>
#include <vector>

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif

Skybox::Skybox()
    : m_cube(), m_skyboxModel(), m_skyboxTexture(), m_initialized(false), m_gammaEnabled(false),
      m_gammaValue(2.2f), m_doGammaLoc(-1), m_fragGammaLoc(-1)
{
    // Only initialize members, actual setup happens in Init()
}
Skybox::~Skybox()
{
    UnloadSkybox();
}

void Skybox::Init()
{
    if (m_initialized)
        return;

    m_cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_skyboxModel = LoadModelFromMesh(m_cube);
    m_skyboxTexture = {0};
    m_initialized = true;
    // Load default shaders with proper path resolution
    std::string vsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/skybox.vs";
    std::string fsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/skybox.fs";

    TraceLog(LOG_INFO, "Skybox::Init() - Loading shaders from: %s, %s", vsPath.c_str(),
             fsPath.c_str());
    LoadMaterialShader(vsPath, fsPath);
}

void Skybox::LoadMaterialShader(const std::string &vsPath, const std::string &fsPath)
{
    if (!m_initialized)
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialShader() - Skybox not initialized");
        return;
    }
    TraceLog(LOG_INFO, "Skybox::LoadMaterialShader() - Loading shaders: VS=%s, FS=%s",
             vsPath.c_str(), fsPath.c_str());

    if (!std::filesystem::exists(vsPath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialShader() - Vertex shader not found: %s",
                 vsPath.c_str());
        return;
    }

    if (!std::filesystem::exists(fsPath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialShader() - Fragment shader not found: %s",
                 fsPath.c_str());
        return;
    }

    // Load shader with error checking
    Shader shader = LoadShader(vsPath.c_str(), fsPath.c_str());
    if (shader.id == 0)
    {
        TraceLog(LOG_ERROR, "Skybox::LoadMaterialShader() - Failed to load shaders");
        return;
    }

    // Set shader for skybox material
    m_skyboxModel.materials[0].shader = shader;

    // Get and set MVP location
    int mvpLoc = GetShaderLocation(shader, "mvp");
    m_skyboxModel.materials[0].shader.locs[SHADER_LOC_MATRIX_MVP] = mvpLoc;

    // Set shader uniforms for skybox cubemap
    int environmentMapLoc = GetShaderLocation(shader, "environmentMap");
    if (environmentMapLoc >= 0)
    {
        int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
        SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
    }

    int doGammaLoc = GetShaderLocation(shader, "doGamma");
    if (doGammaLoc >= 0)
    {
        m_doGammaLoc = doGammaLoc;
        int doGammaValue[1] = {m_gammaEnabled ? 1 : 0};
        SetShaderValue(shader, doGammaLoc, doGammaValue, SHADER_UNIFORM_INT);
    }

    int fragGammaLoc = GetShaderLocation(shader, "fragGamma");
    if (fragGammaLoc >= 0)
    {
        m_fragGammaLoc = fragGammaLoc;
        float fragGammaValue[1] = {m_gammaValue};
        SetShaderValue(shader, fragGammaLoc, fragGammaValue, SHADER_UNIFORM_FLOAT);
    }

    int vflippedLoc = GetShaderLocation(shader, "vflipped");
    if (vflippedLoc >= 0)
    {
        int vflippedValue[1] = {0}; // Not flipped for regular cubemaps
        SetShaderValue(shader, vflippedLoc, vflippedValue, SHADER_UNIFORM_INT);
    }

    TraceLog(LOG_INFO, "Skybox::LoadMaterialShader() - Shaders loaded successfully");
}

void Skybox::LoadMaterialTexture(const std::string &texturePath)
{
    if (!std::filesystem::exists(texturePath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialTexture() - File not found: %s",
                 texturePath.c_str());
        return;
    }

    // Load image and convert to cubemap
    Image image = LoadImage(texturePath.c_str());
    if (image.data == nullptr)
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialTexture() - Failed to load image: %s",
                 texturePath.c_str());
        return;
    }

    m_skyboxTexture = ::LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);
    UnloadImage(image);

    if (m_skyboxTexture.id == 0)
    {
        TraceLog(LOG_ERROR, "Skybox::LoadMaterialTexture() - Failed to create cubemap");
        return;
    }

    // Set material cubemap texture
    SetMaterialTexture(&m_skyboxModel.materials[0], MATERIAL_MAP_CUBEMAP, m_skyboxTexture);
}

void Skybox::UnloadSkybox()
{
    if (m_skyboxTexture.id != 0)
    {
        UnloadTexture(m_skyboxTexture);
        m_skyboxTexture = {0};
    }

    UnloadModel(m_skyboxModel);
    m_skyboxModel = {0};
}

void Skybox::DrawSkybox()
{
    if (!m_initialized)
    {
        TraceLog(LOG_WARNING, "Skybox::DrawSkybox() - Skybox not initialized");
        return;
    }

    if (!IsLoaded())
    {
        TraceLog(LOG_WARNING, "Skybox::DrawSkybox() - Skybox texture not loaded");
        return;
    }

    // Update gamma settings before rendering
    if (m_doGammaLoc >= 0)
    {
        int doGammaValue[1] = {m_gammaEnabled ? 1 : 0};
        SetShaderValue(m_skyboxModel.materials[0].shader, m_doGammaLoc, doGammaValue,
                       SHADER_UNIFORM_INT);
    }
    if (m_fragGammaLoc >= 0)
    {
        float fragGammaValue[1] = {m_gammaValue};
        SetShaderValue(m_skyboxModel.materials[0].shader, m_fragGammaLoc, fragGammaValue,
                       SHADER_UNIFORM_FLOAT);
    }

    // Disable backface culling for inside cube rendering
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // Render skybox at large scale to surround the entire scene
    // Scale should be large enough to be outside the far plane but not too large to cause precision
    // issues
    const float skyboxScale = 1000.0f;
    DrawModel(m_skyboxModel, Vector3{0, 0, 0}, skyboxScale, WHITE);

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void Skybox::SetGammaEnabled(bool enabled)
{
    m_gammaEnabled = enabled;
    // Update shader if already loaded
    if (m_initialized && m_doGammaLoc >= 0)
    {
        int doGammaValue[1] = {m_gammaEnabled ? 1 : 0};
        SetShaderValue(m_skyboxModel.materials[0].shader, m_doGammaLoc, doGammaValue,
                       SHADER_UNIFORM_INT);
    }
}

void Skybox::SetGammaValue(float gamma)
{
    // Clamp gamma value to reasonable range (0.5 to 3.0)
    m_gammaValue = std::max(0.5f, std::min(3.0f, gamma));
    // Update shader if already loaded
    if (m_initialized && m_fragGammaLoc >= 0)
    {
        float fragGammaValue[1] = {m_gammaValue};
        SetShaderValue(m_skyboxModel.materials[0].shader, m_fragGammaLoc, fragGammaValue,
                       SHADER_UNIFORM_FLOAT);
    }
}

void Skybox::UpdateGammaFromConfig()
{
    if (!m_initialized)
    {
        return;
    }

    // Update gamma settings from config
    // SetGammaEnabled(IsSkyboxGammaEnabled());
    // SetGammaValue(GetSkyboxGammaValue());
}



