#ifndef CH_ENVIRONMENT_H
#define CH_ENVIRONMENT_H

#include "asset.h"
#include "engine/core/base.h"
#include <raylib.h>
#include <string>

namespace CHEngine
{

struct SkyboxSettings
{
    std::string TexturePath;
    int Mode = 0; // 0: Equirectangular, 1: Cross (Horizontal)
    float Exposure = 1.0f;
    float Brightness = 0.0f;
    float Contrast = 1.0f;
};

struct FogSettings
{
    bool Enabled = false;
    Color FogColor = GRAY;
    float Density = 0.01f;
    float Start = 10.0f;
    float End = 100.0f;
};

struct LightingSettings
{
    Vector3 Direction = {-1.0f, -1.0f, -1.0f};
    Color LightColor = WHITE;
    float Ambient = 0.3f;
};

struct EnvironmentSettings
{
    LightingSettings Lighting;
    SkyboxSettings Skybox;
    FogSettings Fog;
};

class EnvironmentAsset : public Asset
{
public:
    EnvironmentAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~EnvironmentAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Environment;
    }

    const EnvironmentSettings& GetSettings() const
    {
        return m_Settings;
    }
    EnvironmentSettings& GetSettings()
    {
        return m_Settings;
    }

private:
    EnvironmentSettings m_Settings;
};

} // namespace CHEngine

#endif // CH_ENVIRONMENT_H
