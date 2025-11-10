#include <filesystem>
#include <vector>
#include "rlImGui/rlImGui.h"
#include "rlgl.h"
#include "Skybox.h"

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif


Skybox::Skybox()
    : m_cube(), 
      m_skyboxModel(), 
      m_skyboxTexture(), 
      m_initialized(false)
{
    // Only initialize members, actual setup happens in Init()
}
Skybox::~Skybox()
{
    UnloadSkybox();
}

void Skybox::Init()
{
    if (m_initialized) return;
    
    m_cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_skyboxModel = LoadModelFromMesh(m_cube);
    m_skyboxTexture = { 0 };
    m_initialized = true;
}

void Skybox::LoadMaterialShader(const std::string &vsPath, const std::string &fsPath)
{
    if (!m_initialized)
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialShader() - Skybox not initialized");
        return;
    }
    TraceLog(LOG_INFO, "Skybox::LoadMaterialShader() - Loading shaders: VS=%s, FS=%s", vsPath.c_str(), fsPath.c_str());

    if (!std::filesystem::exists(vsPath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialShader() - Vertex shader not found: %s", vsPath.c_str());
        return;
    }

    if (!std::filesystem::exists(fsPath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialShader() - Fragment shader not found: %s", fsPath.c_str());
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
        int envMapValue[1] = { MATERIAL_MAP_CUBEMAP };
        SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
    }

    int doGammaLoc = GetShaderLocation(shader, "doGamma");
    if (doGammaLoc >= 0)
    {
        int doGammaValue[1] = { 0 }; // No gamma correction for regular textures
        SetShaderValue(shader, doGammaLoc, doGammaValue, SHADER_UNIFORM_INT);
    }

    int vflippedLoc = GetShaderLocation(shader, "vflipped");
    if (vflippedLoc >= 0)
    {
        int vflippedValue[1] = { 0 }; // Not flipped for regular cubemaps
        SetShaderValue(shader, vflippedLoc, vflippedValue, SHADER_UNIFORM_INT);
    }

    TraceLog(LOG_INFO, "Skybox::LoadMaterialShader() - Shaders loaded successfully");
}

void Skybox::LoadShadersAutomatically()
{
    if (!m_initialized)
    {
        TraceLog(LOG_WARNING, "Skybox::LoadShadersAutomatically() - Skybox not initialized");
        return;
    }

    // Try to find shaders in resources/shaders/
    std::string basePath = std::string(PROJECT_ROOT_DIR) + "resources/shaders/";
    std::string vsPath = basePath + "skybox.vs";
    std::string fsPath = basePath + "skybox.fs";

    // Check if shaders exist
    if (std::filesystem::exists(vsPath) && std::filesystem::exists(fsPath))
    {
        TraceLog(LOG_INFO, "Skybox::LoadShadersAutomatically() - Found shaders in resources/shaders/");
        LoadMaterialShader(vsPath, fsPath);
        return;
    }
    TraceLog(LOG_WARNING, "Skybox::LoadShadersAutomatically() - Could not find skybox shaders in resources/shaders/");
}

void Skybox::LoadMaterialTexture(const std::string &texturePath)
{
    if (!std::filesystem::exists(texturePath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialTexture() - File not found: %s", texturePath.c_str());
        return;
    }

    // Load image and convert to cubemap
    Image image = LoadImage(texturePath.c_str());
    if (image.data == nullptr)
    {
        TraceLog(LOG_WARNING, "Skybox::LoadMaterialTexture() - Failed to load image: %s", texturePath.c_str());
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

    // Disable backface culling for inside cube rendering
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    
    // Render skybox at large scale to surround the entire scene
    // Scale should be large enough to be outside the far plane but not too large to cause precision issues
    const float skyboxScale = 1000.0f;
    DrawModel(m_skyboxModel, Vector3{0, 0, 0}, skyboxScale, WHITE);
    
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}