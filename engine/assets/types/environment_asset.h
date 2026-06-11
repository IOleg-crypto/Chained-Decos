#ifndef CH_ENVIRONMENT_H
#define CH_ENVIRONMENT_H

#include "engine/assets/asset.h"
#include <glm/glm.hpp>
#include <string>

#include "color.h"

namespace CHEngine
{

struct Skybox
{
    std::string TexturePath;
    int Mode = 0; // 0: Equirectangular, 1: Cross (Horizontal), 2: Cubemap (GPU)
    float Exposure = 1.0f;
    float Brightness = 0.0f;
    float Contrast = 1.0f;
    bool VFlipped = false;
};

struct Fog
{
    bool Enabled = false;
    int Mode = 0; // 0: Linear, 1: Exp, 2: Exp2
    Color FogColor = {158, 148, 148, 255};
    float Density = 0.001f;
    float Start = 10.0f;
    float End = 5000.0f;
};

struct Lighting
{
    glm::vec3 Direction = {-1.0f, -1.0f, -1.0f};
    Color LightColor = {255, 255, 255, 255}; // WHITE
    float Ambient = 0.3f;
    float Exposure = 1.0f;
    float Gamma = 2.2f;
};

struct EnvironmentParameters
{
    Lighting Lighting;
    Skybox Skybox;
    Fog Fog;
};

class EnvironmentAsset : public Asset
{
public:
    EnvironmentAsset()
        : Asset(GetStaticType())
    {
    }
    EnvironmentAsset(AssetHandle handle)
        : Asset(GetStaticType(), handle)
    {
    }
    virtual ~EnvironmentAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Environment;
    }

    void OnLoaded() override
    {
        CH_CORE_INFO("Initialized environment asset.");
    }

    const EnvironmentParameters& GetSettings() const
    {
        return m_Settings;
    }
    EnvironmentParameters& GetSettings()
    {
        return m_Settings;
    }
    void SetSettings(const EnvironmentParameters& settings)
    {
        m_Settings = settings;
    }

private:
    EnvironmentParameters m_Settings;
};

} // namespace CHEngine

#endif // CH_ENVIRONMENT_H
