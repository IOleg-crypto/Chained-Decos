#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include "raylib.h"
#include <algorithm>
#include <string>
#include <vector>

namespace CHEngine
{
struct ShaderUniform
{
    std::string Name;
    int Type; // 0: Float, 1: Vec2, 2: Vec3, 3: Vec4, 4: Color
    float Value[4] = {0, 0, 0, 0};
};

struct ShaderComponent
{
    std::string ShaderPath;
    std::vector<ShaderUniform> Uniforms;
    bool Enabled = true;

    ShaderComponent() = default;
    ShaderComponent(const ShaderComponent&) = default;

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

    void SetVec3(const std::string& name, Vector3 value)
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
};
} // namespace CHEngine

#endif // CH_SHADER_COMPONENT_H
