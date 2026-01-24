#pragma once
#include "asset.h"
#include "engine/core/base.h"
#include "engine/core/math_types.h"
#include <string>

namespace CHEngine
{

struct SkyboxSettings
{
    std::string TexturePath;
    float Exposure = 1.0f;
    float Brightness = 0.0f;
    float Contrast = 1.0f;
};

struct EnvironmentSettings
{
    Vector3 LightDirection = {-1.0f, -1.0f, -1.0f};
    Color LightColor = WHITE;
    float AmbientIntensity = 0.3f;

    SkyboxSettings Skybox;

    // Future: Fog, PostProcessing, etc.
};

class EnvironmentAsset : public Asset
{
public:
    EnvironmentAsset() = default;
    virtual ~EnvironmentAsset() = default;

    static Ref<EnvironmentAsset> Load(const std::string &path);
    bool Save(const std::string &path);

    virtual AssetType GetType() const override
    {
        return AssetType::Environment;
    }

    const EnvironmentSettings &GetSettings() const
    {
        return m_Settings;
    }
    EnvironmentSettings &GetSettings()
    {
        return m_Settings;
    }

private:
    EnvironmentSettings m_Settings;
};

} // namespace CHEngine
