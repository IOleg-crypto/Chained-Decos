#ifndef LIGHTINGSYSTEM_H
#define LIGHTINGSYSTEM_H

#include <raylib.h>
#include <vector>
#include <string>
#include <memory>

#include "MapObject.h"

// Light types
enum class LightType
{
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2,
    AREA = 3
};

// Light properties
struct LightProperties
{
    LightType type;
    Vector3 position;
    Vector3 target;        // For directional and spot lights
    Color color;
    float intensity;
    float range;           // For point and spot lights
    float angle;           // For spot lights (cone angle)
    float attenuation;     // Light falloff
    bool enabled;
    bool castShadows;
    std::string name;

    LightProperties() :
        type(LightType::POINT),
        position({0.0f, 0.0f, 0.0f}),
        target({0.0f, 0.0f, 1.0f}),
        color(WHITE),
        intensity(1.0f),
        range(10.0f),
        angle(45.0f),
        attenuation(1.0f),
        enabled(true),
        castShadows(false)
    {}
};

// Lighting system for managing scene lighting
class LightingSystem
{
private:
    std::vector<LightProperties> m_lights;
    int m_selectedLight;
    bool m_showLightGizmos;
    bool m_lightingEnabled;

    // Ambient lighting
    Color m_ambientColor;
    float m_ambientIntensity;

    // Environment lighting
    bool m_environmentLighting;
    std::string m_environmentMap;
    Texture2D m_envTexture;
    bool m_envTextureLoaded;

    // Shadow settings
    bool m_shadowsEnabled;
    int m_shadowMapResolution;
    float m_shadowBias;
    float m_shadowDarkness;

public:
    LightingSystem();
    ~LightingSystem();

    // Initialization
    bool Initialize();
    void Cleanup();

    // Light management
    int AddLight(const LightProperties& light);
    bool RemoveLight(int index);
    bool UpdateLight(int index, const LightProperties& light);
    void ClearLights();

    // Light selection
    void SelectLight(int index);
    int GetSelectedLight() const { return m_selectedLight; }
    LightProperties* GetSelectedLightProperties();
    const LightProperties* GetSelectedLightProperties() const;

    // Light properties
    LightProperties* GetLight(int index);
    const LightProperties* GetLight(int index) const;
    size_t GetLightCount() const { return m_lights.size(); }

    // Lighting control
    void EnableLighting(bool enable);
    void EnableLight(int index, bool enable);
    void EnableShadows(bool enable);
    void EnableEnvironmentLighting(bool enable);

    // Ambient lighting
    void SetAmbientColor(const Color& color);
    void SetAmbientIntensity(float intensity);
    Color GetAmbientColor() const { return m_ambientColor; }
    float GetAmbientIntensity() const { return m_ambientIntensity; }

    // Environment lighting
    bool LoadEnvironmentMap(const std::string& texturePath);
    void UnloadEnvironmentMap();

    // Shadow settings
    void SetShadowMapResolution(int resolution);
    void SetShadowBias(float bias);
    void SetShadowDarkness(float darkness);
    int GetShadowMapResolution() const { return m_shadowMapResolution; }
    float GetShadowBias() const { return m_shadowBias; }
    float GetShadowDarkness() const { return m_shadowDarkness; }

    // Gizmo visibility
    void ShowLightGizmos(bool show);
    bool AreLightGizmosVisible() const { return m_showLightGizmos; }

    // Utility functions
    void UpdateLightGizmos();
    Vector3 GetLightDirection(int index) const;
    float GetLightDistance(const Vector3& point, int lightIndex) const;
    bool IsLightVisible(int index) const;

    // Rendering
    void Render();
    void RenderLightGizmos();
    void RenderShadowMaps();

    // Serialization
    std::string SerializeToJson() const;
    bool DeserializeFromJson(const std::string& json);

private:
    // Helper functions
    void RenderDirectionalLightGizmo(int index);
    void RenderPointLightGizmo(int index);
    void RenderSpotLightGizmo(int index);
    void RenderAreaLightGizmo(int index);

    void UpdateLightMatrix(int index);
    Matrix GetLightViewMatrix(int index) const;
    Matrix GetLightProjectionMatrix(int index) const;

    Color GetLightColor(int index) const;
    float GetLightRadius(int index) const;
};

#endif // LIGHTINGSYSTEM_H