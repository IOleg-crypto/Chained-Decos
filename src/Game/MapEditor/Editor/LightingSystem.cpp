#include "LightingSystem.h"
#include <raylib.h>
#include <cmath>
#include <algorithm>
#include <iostream>

LightingSystem::LightingSystem()
    : m_selectedLight(-1)
    , m_showLightGizmos(true)
    , m_lightingEnabled(true)
    , m_ambientColor({26, 26, 26, 255}) // Dark gray
    , m_ambientIntensity(0.3f)
    , m_environmentLighting(false)
    , m_envTextureLoaded(false)
    , m_shadowsEnabled(false)
    , m_shadowMapResolution(1024)
    , m_shadowBias(0.005f)
    , m_shadowDarkness(0.8f)
{
}

LightingSystem::~LightingSystem()
{
    Cleanup();
}

bool LightingSystem::Initialize()
{
    // Set up default ambient lighting
    m_ambientColor = {26, 26, 26, 255};

    // Add a default directional light (sun)
    LightProperties defaultLight;
    defaultLight.type = LightType::DIRECTIONAL;
    defaultLight.name = "Sun";
    defaultLight.color = {245, 245, 200, 255}; // Warm sunlight color
    defaultLight.intensity = 1.0f;
    defaultLight.target = {0.0f, -1.0f, 0.0f}; // Pointing down
    defaultLight.enabled = true;
    defaultLight.castShadows = true;

    m_lights.push_back(defaultLight);
    m_selectedLight = 0;

    return true;
}

void LightingSystem::Cleanup()
{
    if (m_envTextureLoaded)
    {
        UnloadTexture(m_envTexture);
        m_envTextureLoaded = false;
    }
    m_lights.clear();
    m_selectedLight = -1;
}

int LightingSystem::AddLight(const LightProperties& light)
{
    m_lights.push_back(light);
    return static_cast<int>(m_lights.size()) - 1;
}

bool LightingSystem::RemoveLight(int index)
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return false;

    m_lights.erase(m_lights.begin() + index);

    if (m_selectedLight >= static_cast<int>(m_lights.size()))
    {
        m_selectedLight = static_cast<int>(m_lights.size()) - 1;
    }

    return true;
}

bool LightingSystem::UpdateLight(int index, const LightProperties& light)
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return false;

    m_lights[index] = light;
    return true;
}

void LightingSystem::ClearLights()
{
    m_lights.clear();
    m_selectedLight = -1;
}

void LightingSystem::SelectLight(int index)
{
    if (index >= -1 && index < static_cast<int>(m_lights.size()))
    {
        m_selectedLight = index;
    }
}

LightProperties* LightingSystem::GetSelectedLightProperties()
{
    if (m_selectedLight >= 0 && m_selectedLight < static_cast<int>(m_lights.size()))
    {
        return &m_lights[m_selectedLight];
    }
    return nullptr;
}

const LightProperties* LightingSystem::GetSelectedLightProperties() const
{
    if (m_selectedLight >= 0 && m_selectedLight < static_cast<int>(m_lights.size()))
    {
        return &m_lights[m_selectedLight];
    }
    return nullptr;
}

LightProperties* LightingSystem::GetLight(int index)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size()))
    {
        return &m_lights[index];
    }
    return nullptr;
}

const LightProperties* LightingSystem::GetLight(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_lights.size()))
    {
        return &m_lights[index];
    }
    return nullptr;
}

void LightingSystem::EnableLighting(bool enable)
{
    m_lightingEnabled = enable;
}

void LightingSystem::EnableLight(int index, bool enable)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size()))
    {
        m_lights[index].enabled = enable;
    }
}

void LightingSystem::EnableShadows(bool enable)
{
    m_shadowsEnabled = enable;
}

void LightingSystem::EnableEnvironmentLighting(bool enable)
{
    m_environmentLighting = enable;
}

void LightingSystem::SetAmbientColor(const Color& color)
{
    m_ambientColor = color;
}

void LightingSystem::SetAmbientIntensity(float intensity)
{
    m_ambientIntensity = std::max(0.0f, std::min(1.0f, intensity));
}

bool LightingSystem::LoadEnvironmentMap(const std::string& texturePath)
{
    if (m_envTextureLoaded)
    {
        UnloadTexture(m_envTexture);
    }

    m_envTexture = LoadTexture(texturePath.c_str());
    m_envTextureLoaded = m_envTexture.id != 0;
    m_environmentMap = texturePath;

    if (m_envTextureLoaded)
    {
        std::cout << "Loaded environment map: " << texturePath << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to load environment map: " << texturePath << std::endl;
        return false;
    }
}

void LightingSystem::UnloadEnvironmentMap()
{
    if (m_envTextureLoaded)
    {
        UnloadTexture(m_envTexture);
        m_envTextureLoaded = false;
    }
    m_environmentMap.clear();
}

void LightingSystem::SetShadowMapResolution(int resolution)
{
    m_shadowMapResolution = std::max(256, std::min(4096, resolution));
}

void LightingSystem::SetShadowBias(float bias)
{
    m_shadowBias = std::max(0.0f, std::min(0.1f, bias));
}

void LightingSystem::SetShadowDarkness(float darkness)
{
    m_shadowDarkness = std::max(0.0f, std::min(1.0f, darkness));
}

void LightingSystem::ShowLightGizmos(bool show)
{
    m_showLightGizmos = show;
}

void LightingSystem::UpdateLightGizmos()
{
    // Update light gizmo positions and properties
    // This would be called each frame to update light gizmos
}

Vector3 LightingSystem::GetLightDirection(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return {0.0f, 0.0f, 0.0f};

    const LightProperties& light = m_lights[index];

    if (light.type == LightType::DIRECTIONAL)
    {
        Vector3 direction = Vector3Subtract(light.target, light.position);
        return Vector3Normalize(direction);
    }

    return {0.0f, -1.0f, 0.0f}; // Default downward direction
}

float LightingSystem::GetLightDistance(const Vector3& point, int lightIndex) const
{
    if (lightIndex < 0 || lightIndex >= static_cast<int>(m_lights.size()))
        return 0.0f;

    const LightProperties& light = m_lights[lightIndex];
    return Vector3Distance(point, light.position);
}

bool LightingSystem::IsLightVisible(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return false;

    return m_lights[index].enabled;
}

void LightingSystem::Render()
{
    if (!m_lightingEnabled) return;

    // Apply ambient lighting
    Color ambient = {
        static_cast<unsigned char>(m_ambientColor.r * m_ambientIntensity),
        static_cast<unsigned char>(m_ambientColor.g * m_ambientIntensity),
        static_cast<unsigned char>(m_ambientColor.b * m_ambientIntensity),
        m_ambientColor.a
    };

    // In a real implementation, this would set up lighting uniforms for shaders
    // For now, we'll just render light gizmos if enabled
    if (m_showLightGizmos)
    {
        RenderLightGizmos();
    }
}

void LightingSystem::RenderLightGizmos()
{
    for (size_t i = 0; i < m_lights.size(); ++i)
    {
        if (!m_lights[i].enabled) continue;

        switch (m_lights[i].type)
        {
            case LightType::DIRECTIONAL:
                RenderDirectionalLightGizmo(static_cast<int>(i));
                break;
            case LightType::POINT:
                RenderPointLightGizmo(static_cast<int>(i));
                break;
            case LightType::SPOT:
                RenderSpotLightGizmo(static_cast<int>(i));
                break;
            case LightType::AREA:
                RenderAreaLightGizmo(static_cast<int>(i));
                break;
        }
    }
}

void LightingSystem::RenderDirectionalLightGizmo(int index)
{
    const LightProperties& light = m_lights[index];

    // Draw light direction as a line
    Vector3 endPoint = Vector3Add(light.position, Vector3Scale(light.target, 5.0f));
    DrawLine3D(light.position, endPoint, light.color);

    // Draw light position as a small sphere
    DrawSphere(light.position, 0.1f, light.color);

    // Draw target direction indicator
    DrawSphere(endPoint, 0.05f, YELLOW);
}

void LightingSystem::RenderPointLightGizmo(int index)
{
    const LightProperties& light = m_lights[index];

    // Draw light as a sphere with radius based on range
    DrawSphere(light.position, 0.1f, light.color);

    // Draw light range as a wireframe sphere
    if (light.range > 0)
    {
        DrawSphereWires(light.position, light.range, 16, 16, Fade(light.color, 0.3f));
    }
}

void LightingSystem::RenderSpotLightGizmo(int index)
{
    const LightProperties& light = m_lights[index];

    // Draw light position
    DrawSphere(light.position, 0.1f, light.color);

    // Draw light cone
    Vector3 forward = Vector3Normalize(Vector3Subtract(light.target, light.position));
    Vector3 up = {0.0f, 1.0f, 0.0f};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));

    float angleRad = light.angle * DEG2RAD;
    float coneLength = light.range;

    // Draw cone lines
    for (int i = 0; i < 8; ++i)
    {
        float angle = (i / 8.0f) * 2.0f * PI;
        Vector3 coneDir = Vector3Add(
            Vector3Scale(forward, cosf(angleRad)),
            Vector3Scale(Vector3Normalize(Vector3Add(
                Vector3Scale(right, cosf(angle)),
                Vector3Scale(up, sinf(angle))
            )), sinf(angleRad))
        );

        Vector3 endPoint = Vector3Add(light.position, Vector3Scale(coneDir, coneLength));
        DrawLine3D(light.position, endPoint, Fade(light.color, 0.5f));
    }

    // Draw target point
    Vector3 targetPoint = Vector3Add(light.position, Vector3Scale(forward, coneLength));
    DrawSphere(targetPoint, 0.05f, YELLOW);
}

void LightingSystem::RenderAreaLightGizmo(int index)
{
    const LightProperties& light = m_lights[index];

    // Draw area light as a rectangle
    Vector3 forward = Vector3Normalize(Vector3Subtract(light.target, light.position));
    Vector3 up = {0.0f, 1.0f, 0.0f};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));

    Vector3 corners[4];
    float width = light.range;
    float height = width * 0.5f; // Assume square for now

    corners[0] = Vector3Subtract(Vector3Subtract(light.position, Vector3Scale(right, width * 0.5f)), Vector3Scale(up, height * 0.5f));
    corners[1] = Vector3Add(Vector3Subtract(light.position, Vector3Scale(right, width * 0.5f)), Vector3Scale(up, height * 0.5f));
    corners[2] = Vector3Add(Vector3Add(light.position, Vector3Scale(right, width * 0.5f)), Vector3Scale(up, height * 0.5f));
    corners[3] = Vector3Subtract(Vector3Add(light.position, Vector3Scale(right, width * 0.5f)), Vector3Scale(up, height * 0.5f));

    // Draw rectangle
    DrawLine3D(corners[0], corners[1], light.color);
    DrawLine3D(corners[1], corners[2], light.color);
    DrawLine3D(corners[2], corners[3], light.color);
    DrawLine3D(corners[3], corners[0], light.color);

    // Draw diagonals
    DrawLine3D(corners[0], corners[2], Fade(light.color, 0.5f));
    DrawLine3D(corners[1], corners[3], Fade(light.color, 0.5f));
}

void LightingSystem::RenderShadowMaps()
{
    if (!m_shadowsEnabled) return;

    // Shadow map rendering would be implemented here
    // This is a placeholder for shadow mapping functionality
}

std::string LightingSystem::SerializeToJson() const
{
    // JSON serialization would be implemented here
    return "{}";
}

bool LightingSystem::DeserializeFromJson(const std::string& json)
{
    // JSON deserialization would be implemented here
    return true;
}

void LightingSystem::UpdateLightMatrix(int index)
{
    // Update light transformation matrices for shadow mapping
    // This would be called for shadow map generation
}

Matrix LightingSystem::GetLightViewMatrix(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return MatrixIdentity();

    const LightProperties& light = m_lights[index];
    Vector3 direction = GetLightDirection(index);
    Vector3 position = light.position;

    // For directional lights, position is used as the view position
    // For point/spot lights, position is the light position
    return MatrixLookAt(position, Vector3Add(position, direction), {0.0f, 1.0f, 0.0f});
}

Matrix LightingSystem::GetLightProjectionMatrix(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return MatrixIdentity();

    const LightProperties& light = m_lights[index];

    switch (light.type)
    {
        case LightType::DIRECTIONAL:
            return MatrixOrtho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
        case LightType::POINT:
        case LightType::SPOT:
            return MatrixPerspective(light.angle * 2.0f, 1.0f, 0.1f, light.range);
        case LightType::AREA:
            return MatrixOrtho(-light.range, light.range, -light.range, light.range, 0.1f, 50.0f);
        default:
            return MatrixIdentity();
    }
}

Color LightingSystem::GetLightColor(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return WHITE;

    return m_lights[index].color;
}

float LightingSystem::GetLightRadius(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lights.size()))
        return 0.0f;

    return m_lights[index].range;
}