#include "Skybox.h"
#include "core/Log.h"
#include "core/config/ConfigManager.h"
#include "rlImGui/rlImGui.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
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

// Helper to get a direction vector for a pixel in a cube face
static Vector3 GetDirectionForPixel(int face, float u, float v)
{
    // u, v are in [0, 1]
    float x = 2.0f * u - 1.0f;
    float y = 2.0f * v - 1.0f;

    switch (face)
    {
    case 0:
        return {1.0f, -y, -x}; // +X (Right)
    case 1:
        return {-1.0f, -y, x}; // -X (Left)
    case 2:
        return {x, 1.0f, y}; // +Y (Top)
    case 3:
        return {x, -1.0f, -y}; // -Y (Bottom)
    case 4:
        return {x, -y, 1.0f}; // +Z (Front)
    case 5:
        return {-x, -y, -1.0f}; // -Z (Back)
    default:
        return {0.0f, 0.0f, 0.0f};
    }
}

// Manual conversion from Panorama (equirectangular) to Cubemap (vertical strip)
static Image ConvertPanoramaToCubemap(Image panorama)
{
    int size = panorama.height / 2; // Common quality choice
    if (size == 0)
        size = 512;

    // Create image for 6 faces in a vertical column
    Image cubemap = GenImageColor(size, size * 6, BLACK);
    ImageFormat(&cubemap, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // Ensure panorama is in a format we can easily sample
    Image src = ImageCopy(panorama);
    ImageFormat(&src, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Color *srcPixels = (Color *)src.data;
    Color *dstPixels = (Color *)cubemap.data;

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                // Get direction for this pixel
                float x_norm = (float)x / (float)(size - 1);
                float y_norm = (float)y / (float)(size - 1);
                Vector3 dir = GetDirectionForPixel(face, x_norm, y_norm);

                // Map direction to equirectangular UV
                float theta = atan2(dir.z, dir.x);
                float radius = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                float phi = asin(dir.y / radius);

                float srcU = (theta + PI) / (2.0f * PI);
                float srcV = (phi + PI / 2.0f) / PI;

                // Bilinear sampling
                float fX = srcU * (src.width - 1);
                float fY = (1.0f - srcV) * (src.height - 1);

                int x0 = (int)floor(fX);
                int y0 = (int)floor(fY);
                int x1 = std::min(x0 + 1, src.width - 1);
                int y1 = std::min(y0 + 1, src.height - 1);

                float dx = fX - (float)x0;
                float dy = fY - (float)y0;

                Color c00 = srcPixels[y0 * src.width + x0];
                Color c10 = srcPixels[y0 * src.width + x1];
                Color c01 = srcPixels[y1 * src.width + x0];
                Color c11 = srcPixels[y1 * src.width + x1];

                int dstIdx = face * size * size + y * size + x;
                dstPixels[dstIdx].r =
                    (unsigned char)((c00.r * (1.0f - dx) + c10.r * dx) * (1.0f - dy) +
                                    (c01.r * (1.0f - dx) + c11.r * dx) * dy);
                dstPixels[dstIdx].g =
                    (unsigned char)((c00.g * (1.0f - dx) + c10.g * dx) * (1.0f - dy) +
                                    (c01.g * (1.0f - dx) + c11.g * dx) * dy);
                dstPixels[dstIdx].b =
                    (unsigned char)((c00.b * (1.0f - dx) + c10.b * dx) * (1.0f - dy) +
                                    (c01.b * (1.0f - dx) + c11.b * dx) * dy);
                dstPixels[dstIdx].a = 255;
            }
        }
    }

    UnloadImage(src);
    return cubemap;
}

void Skybox::LoadMaterialTexture(const std::string &texturePath)
{
    if (!std::filesystem::exists(texturePath))
    {
        CD_CORE_WARN("Skybox::LoadMaterialTexture() - File not found: %s", texturePath.c_str());
        return;
    }

    // Try to load as cubemap first with auto-detect
    CD_CORE_INFO("Skybox::LoadMaterialTexture() - Attempting to load cubemap: %s",
                 texturePath.c_str());
    Image image = LoadImage(texturePath.c_str());
    if (image.data == nullptr)
    {
        CD_CORE_WARN("Skybox::LoadMaterialTexture() - Failed to load image: %s",
                     texturePath.c_str());
        return;
    }

    // Unload existing texture if any
    if (m_skyboxTexture.id != 0)
    {
        UnloadTexture(m_skyboxTexture);
        m_skyboxTexture = {0};
    }

    // 1. Attempt standard cubemap detection (cross, line)
    // CUBEMAP_LAYOUT_AUTO_DETECT = 0
    m_skyboxTexture = ::LoadTextureCubemap(image, 0);

    // 2. If it fails and it's a wide image (like 16:9), it's likely a panorama.
    // Manually convert it to a cubemap vertical strip and load.
    if (m_skyboxTexture.id == 0 && image.width > image.height)
    {
        CD_CORE_INFO("Skybox::LoadMaterialTexture() - Auto-detect failed, performing manual "
                     "Panorama conversion...");
        Image cubemapImg = ConvertPanoramaToCubemap(image);

        // Load the vertical strip (layout 1 is LINE_VERTICAL)
        m_skyboxTexture = ::LoadTextureCubemap(cubemapImg, 1);
        UnloadImage(cubemapImg);
    }

    UnloadImage(image);

    if (m_skyboxTexture.id == 0)
    {
        CD_CORE_ERROR("Skybox::LoadMaterialTexture() - Failed to create cubemap. Image might not "
                      "match any standard layout.");
        return;
    }

    CD_CORE_INFO("Skybox::LoadMaterialTexture() - Successfully created Cubemap");

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
bool Skybox::IsLoaded() const
{
    return m_skyboxTexture.id != 0;
}
bool Skybox::IsInitialized() const
{
    return m_initialized;
}
float Skybox::GetGammaValue() const
{
    return m_gammaValue;
}
bool Skybox::IsGammaEnabled() const
{
    return m_gammaEnabled;
}
float Skybox::GetExposure() const
{
    return m_exposure;
}
