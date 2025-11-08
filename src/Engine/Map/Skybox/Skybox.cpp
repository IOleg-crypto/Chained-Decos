#include <filesystem>
#include "rlImGui/rlImGui.h"
#include "rlgl.h"
#include "Skybox.h"


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

    TraceLog(LOG_INFO, "Skybox::LoadMaterialShader() - Shaders loaded successfully");
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

void Skybox::LoadTextureCubemap(const std::string &texturePath)
{
    if (!std::filesystem::exists(texturePath))
    {
        TraceLog(LOG_WARNING, "Skybox::LoadTextureCubemap() - Texture file not found: %s", texturePath.c_str());
        return;
    }

    m_skyboxTexture = LoadTexture(texturePath.c_str());
    if (m_skyboxTexture.id == 0)
    {
        TraceLog(LOG_ERROR, "Skybox::LoadTextureCubemap() - Failed to load cubemap texture");
        return;
    }

    m_skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = m_skyboxTexture;
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
    
    DrawModel(m_skyboxModel, Vector3{0, 0, 0}, 1.0f, WHITE);
    
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}