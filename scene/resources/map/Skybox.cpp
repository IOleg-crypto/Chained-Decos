#include "Skybox.h"
#include "core/Log.h"
#include "core/config/ConfigManager.h"
#include "rlImGui/rlImGui.h"
#include "rlgl.h"
#include <filesystem>
#include <vector>

Skybox::Skybox()
    : m_cube(), m_skyboxModel(), m_skyboxTexture(), m_initialized(false), m_gammaEnabled(false),
      m_gammaValue(2.2f), m_exposure(1.0f), m_doGammaLoc(-1), m_fragGammaLoc(-1), m_exposureLoc(-1)
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

    // Robust shader path resolution
    std::string vsPath, fsPath;
    std::vector<std::string> searchPaths = {std::string(PROJECT_ROOT_DIR) + "/resources/shaders/",
                                            "./resources/shaders/", "../resources/shaders/",
                                            "resources/shaders/"};

    for (const auto &path : searchPaths)
    {
        if (std::filesystem::exists(path + "skybox.vs") &&
            std::filesystem::exists(path + "skybox.fs"))
        {
            vsPath = path + "skybox.vs";
            fsPath = path + "skybox.fs";
            break;
        }
    }

    if (vsPath.empty())
    {
        CD_CORE_ERROR("Skybox::Init() - Shaders skybox.vs/fs not found in any standard location!");
        return;
    }

    CD_CORE_INFO("Skybox::Init() - Loading shaders from: %s, %s", vsPath.c_str(), fsPath.c_str());
    LoadMaterialShader(vsPath, fsPath);
}

void Skybox::LoadMaterialShader(const std::string &vsPath, const std::string &fsPath)
{
    if (!m_initialized)
    {
        CD_CORE_WARN("Skybox::LoadMaterialShader() - Skybox not initialized");
        return;
    }
    CD_CORE_INFO("Skybox::LoadMaterialShader() - Loading shaders: VS=%s, FS=%s", vsPath.c_str(),
                 fsPath.c_str());

    if (!std::filesystem::exists(vsPath))
    {
        CD_CORE_WARN("Skybox::LoadMaterialShader() - Vertex shader not found: %s", vsPath.c_str());
        return;
    }

    if (!std::filesystem::exists(fsPath))
    {
        CD_CORE_WARN("Skybox::LoadMaterialShader() - Fragment shader not found: %s",
                     fsPath.c_str());
        return;
    }

    // Load shader with error checking
    Shader shader = LoadShader(vsPath.c_str(), fsPath.c_str());
    if (shader.id == 0)
    {
        CD_CORE_ERROR("Skybox::LoadMaterialShader() - Failed to load shaders");
        return;
    }

    // Set shader for skybox material
    m_skyboxModel.materials[0].shader = shader;

    // Get and set MVP location
    int mvpLoc = GetShaderLocation(shader, "mvp");
    m_skyboxModel.materials[0].shader.locs[SHADER_LOC_MATRIX_MVP] = mvpLoc;

    int environmentMapLoc = GetShaderLocation(shader, "environmentMap");
    if (environmentMapLoc >= 0)
    {
        m_skyboxModel.materials[0].shader.locs[SHADER_LOC_MAP_CUBEMAP] = environmentMapLoc;
        int envMapValue[1] = {MATERIAL_MAP_CUBEMAP};
        SetShaderValue(shader, environmentMapLoc, envMapValue, SHADER_UNIFORM_INT);
    }

    // Map Projection and View matrices for the skybox shader
    int viewLoc = GetShaderLocation(shader, "matView");
    if (viewLoc >= 0)
        m_skyboxModel.materials[0].shader.locs[SHADER_LOC_MATRIX_VIEW] = viewLoc;

    int projLoc = GetShaderLocation(shader, "matProjection");
    if (projLoc >= 0)
        m_skyboxModel.materials[0].shader.locs[SHADER_LOC_MATRIX_PROJECTION] = projLoc;

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

    int exposureLoc = GetShaderLocation(shader, "exposure");
    if (exposureLoc >= 0)
    {
        m_exposureLoc = exposureLoc;
        float exposureValue[1] = {m_exposure};
        SetShaderValue(shader, exposureLoc, exposureValue, SHADER_UNIFORM_FLOAT);
    }

    CD_CORE_INFO("Skybox::LoadMaterialShader() - Shaders loaded successfully");
}

void Skybox::LoadMaterialTexture(const std::string &texturePath)
{
    if (!std::filesystem::exists(texturePath))
    {
        CD_CORE_WARN("Skybox::LoadMaterialTexture() - File not found: %s", texturePath.c_str());
        return;
    }

    // Load image and convert to cubemap
    Image image = LoadImage(texturePath.c_str());
    if (image.data == nullptr)
    {
        CD_CORE_WARN("Skybox::LoadMaterialTexture() - Failed to load image: %s",
                     texturePath.c_str());
        return;
    }

    m_skyboxTexture = ::LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);
    UnloadImage(image);

    if (m_skyboxTexture.id == 0)
    {
        CD_CORE_ERROR("Skybox::LoadMaterialTexture() - Failed to create cubemap");
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

void Skybox::DrawSkybox(Vector3 position)
{
    if (!m_initialized)
    {
        CD_CORE_WARN("Skybox::DrawSkybox() - Skybox not initialized");
        return;
    }

    if (!IsLoaded())
    {
        CD_CORE_WARN("Skybox::DrawSkybox() - Skybox texture not loaded");
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
    if (m_exposureLoc >= 0)
    {
        float exposureValue[1] = {m_exposure};
        SetShaderValue(m_skyboxModel.materials[0].shader, m_exposureLoc, exposureValue,
                       SHADER_UNIFORM_FLOAT);
    }

    // Disable backface culling for inside cube rendering
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // Render skybox at large scale to surround the entire scene
    // Scale should be large enough to be outside the far plane but not too large to cause precision
    // issues
    const float skyboxScale = 1000.0f;
    DrawModel(m_skyboxModel, position, skyboxScale, WHITE);

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

void Skybox::SetExposure(float exposure)
{
    m_exposure = std::max(0.0f, exposure);
    if (m_initialized && m_exposureLoc >= 0)
    {
        float exposureValue[1] = {m_exposure};
        SetShaderValue(m_skyboxModel.materials[0].shader, m_exposureLoc, exposureValue,
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
