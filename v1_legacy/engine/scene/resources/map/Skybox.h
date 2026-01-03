#ifndef CD_SCENE_RESOURCES_MAP_SKYBOX_H
#define CD_SCENE_RESOURCES_MAP_SKYBOX_H

#include "raylib.h"
#include <string>

// Forward declaration
class ConfigManager;

class Skybox
{
private:
    Mesh m_cube;               // Cube mesh for skybox
    Model m_skyboxModel;       // Skybox model
    Texture2D m_skyboxTexture; // Skybox texture
    bool m_initialized;        // Add initialization flag
    bool m_gammaEnabled;       // Gamma correction enabled
    float m_gammaValue;        // Gamma value (typically 2.2)
    float m_exposure;          // Exposure value
    float m_brightness;        // Brightness value
    float m_contrast;          // Contrast value
    int m_doGammaLoc;          // Shader location for doGamma uniform
    int m_fragGammaLoc;        // Shader location for fragGamma uniform
    int m_exposureLoc;         // Shader location for exposure uniform
    int m_brightnessLoc;       // Shader location for brightness uniform
    int m_contrastLoc;         // Shader location for contrast uniform

public:
    Skybox();
    ~Skybox();

    void Init(); // New initialization method
    void LoadMaterialTexture(const std::string &texturePath);
    void LoadMaterialShader(const std::string &vsPath, const std::string &fsPath);
    void DrawSkybox(Vector3 position);
    void UnloadSkybox();
    bool IsInitialized() const;
    bool IsLoaded() const;

    // Gamma correction settings
    void SetGammaEnabled(bool enabled);
    void SetGammaValue(float gamma);
    float GetGammaValue() const;
    bool IsGammaEnabled() const;

    // Exposure settings
    void SetExposure(float exposure);
    float GetExposure() const;

    // Brightness and Contrast settings
    void SetBrightness(float brightness);
    float GetBrightness() const;
    void SetContrast(float contrast);
    float GetContrast() const;

    // Update gamma from config (for settings integration)
    void UpdateGammaFromConfig();
};

#endif // CD_SCENE_RESOURCES_MAP_SKYBOX_H
