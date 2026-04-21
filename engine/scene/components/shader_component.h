#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include "engine/core/assets/asset_manager.h"
#include "engine/core/reflection.h"
#include "engine/graphics/assets/shader_asset.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
#include "engine/graphics/pipeline/renderer_types.h"

namespace CHEngine
{

struct ShaderComponent
{
    std::string ShaderPath;
    std::vector<ShaderUniform> Uniforms;
    bool Enabled = true;

    ShaderComponent() = default;
    ShaderComponent(const ShaderComponent&) = default;

    static bool IsSystemUniform(const std::string& name)
    {
        static const std::vector<std::string> reserved = {"mvp",
                                                          "matModel",
                                                          "matView",
                                                          "matProjection",
                                                          "matNormal",
                                                          "boneMatrices",
                                                          "useSkinning",
                                                          "uTime",
                                                          "viewPos",
                                                          "lightDir",
                                                          "lightColor",
                                                          "ambient",
                                                          "skyAmbientColor",
                                                          "uLightCount",
                                                          "uExposure",
                                                          "uGamma",
                                                          "texture0",
                                                          "texture1",
                                                          "texture2",
                                                          "texture3",
                                                          "texture4",
                                                          "texture5",
                                                          "colDiffuse",
                                                          "useTexture",
                                                          "colEmissive",
                                                          "emissiveIntensity",
                                                          "metalness",
                                                          "roughness",
                                                          "useNormalMap",
                                                          "useMetallicMap",
                                                          "useRoughnessMap",
                                                          "useOcclusionMap",
                                                          "useEmissiveTexture"};
        return std::find(reserved.begin(), reserved.end(), name) != reserved.end();
    }

    void SetFloat(const std::string& name, float value)
    {
        auto it = std::find_if(Uniforms.begin(), Uniforms.end(), [&](const auto& u) { return u.Name == name; });
        if (it != Uniforms.end())
        {
            it->Value[0] = value;
        }
        else
        {
            Uniforms.push_back({name, 0, {value, 0, 0, 0}});
        }
    }

    void SetVec3(const std::string& name, const glm::vec3& value)
    {
        auto it = std::find_if(Uniforms.begin(), Uniforms.end(), [&](const auto& u) { return u.Name == name; });
        if (it != Uniforms.end())
        {
            it->Value[0] = value.x;
            it->Value[1] = value.y;
            it->Value[2] = value.z;
        }
        else
        {
            Uniforms.push_back({name, 2, {value.x, value.y, value.z, 0}});
        }
    }

    void SyncWithShader()
    {
        auto asset = AssetManager::Get().Get<ShaderAsset>(ShaderPath);
        if (!asset)
        {
            return;
        }

        const auto& names = asset->GetUniformNames();
        for (const auto& name : names)
        {
            if (IsSystemUniform(name))
            {
                continue;
            }

            auto it = std::find_if(Uniforms.begin(), Uniforms.end(), [&](const auto& u) { return u.Name == name; });
            if (it == Uniforms.end())
            {
                Uniforms.push_back({name, 0, {0, 0, 0, 0}});
            }
        }
    }

    CH_REFLECT_BEGIN(ShaderComponent)
    props.Header("Shader Asset");
    if (props.File("ShaderPath", ShaderPath, "glsl,shader,chshader"))
    {
        SyncWithShader();
    }
    props.Property("Enabled", Enabled);

    props.Action("Refresh Uniforms", [&]() { SyncWithShader(); });

    // Filter out system uniforms if they somehow got in
    Uniforms.erase(std::remove_if(Uniforms.begin(), Uniforms.end(),
                                  [](const auto& u) { return IsSystemUniform(u.Name); }),
                   Uniforms.end());

    props.Sequence("Uniforms", Uniforms, false);
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_SHADER_COMPONENT_H
