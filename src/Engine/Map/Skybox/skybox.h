#ifndef SKYBOX_H
#define SKYBOX_H

#include "raylib.h"
#include <string>

class Skybox
{
private:
    Mesh m_cube;                 // Cube mesh for skybox
    Model m_skyboxModel;         // Skybox model
    Texture2D m_skyboxTexture;   // Skybox texture
    bool m_initialized;          // Add initialization flag

public:
    Skybox();
    ~Skybox();

    void Init();                 // New initialization method
    void LoadMaterialTexture(const std::string &texturePath);
    void LoadMaterialShader(const std::string &vsPath, const std::string &fsPath);
    void LoadShadersAutomatically(); // Automatically find and load shaders from resources/shaders
    void DrawSkybox();
    void UnloadSkybox();
    bool IsInitialized() const { return m_initialized; }
    bool IsLoaded() const { return m_skyboxTexture.id != 0; }
};

#endif // SKYBOX_H