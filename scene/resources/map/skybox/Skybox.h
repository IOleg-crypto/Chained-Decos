#ifndef SKYBOX_H
#define SKYBOX_H

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
    int m_doGammaLoc;          // Shader location for doGamma uniform
    int m_fragGammaLoc;        // Shader location for fragGamma uniform

public:
    Skybox();
    ~Skybox();

    void Init(); // New initialization method
    void LoadMaterialTexture(const std::string &texturePath);
    void LoadMaterialShader(const std::string &vsPath, const std::string &fsPath);
    void DrawSkybox(Vector3 position);
    void UnloadSkybox();
    bool IsInitialized() const
    {
        return m_initialized;
    }
    bool IsLoaded() const
    {
        return m_skyboxTexture.id != 0;
    }

    // Gamma correction settings
    void SetGammaEnabled(bool enabled);
    void SetGammaValue(float gamma);
    bool IsGammaEnabled() const
    {
        return m_gammaEnabled;
    }
    float GetGammaValue() const
    {
        return m_gammaValue;
    }

    // Update gamma from config (for settings integration)
    void UpdateGammaFromConfig();
};

#endif // SKYBOX_H
