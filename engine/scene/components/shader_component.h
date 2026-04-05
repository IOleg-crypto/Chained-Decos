#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H


#include "engine/core/reflection.h"
#include <algorithm>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace CHEngine
{
struct ShaderUniform
{
    std::string Name;
    int Type; // 0: Float, 1: Vec2, 2: Vec3, 3: Vec4, 4: Color
    float Value[4] = {0, 0, 0, 0};

    CH_REFLECT_BEGIN(ShaderUniform)
        props.Property("Name", Name);
        static const char* types[] = { "Float", "Vec2", "Vec3", "Vec4", "Color" };
        props.Enum("Type", Type, types, 5);
        if (Type == 4) // Color
        {
            Color c = { (unsigned char)(Value[0]*255), (unsigned char)(Value[1]*255), (unsigned char)(Value[2]*255), (unsigned char)(Value[3]*255) };
            if (props.Property("Color", c))
            {
                Value[0] = c.r / 255.0f;
                Value[1] = c.g / 255.0f;
                Value[2] = c.b / 255.0f;
                Value[3] = c.a / 255.0f;
            }
        }
        else
        {
            // Simple float array for others for now
            props.Property("Value", Value); 
        }
    CH_REFLECT_END()
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

    CH_REFLECT_BEGIN(ShaderComponent)
        props.Header("Shader Asset");
        props.File("ShaderPath", ShaderPath, "glsl,shader");
        props.Property("Enabled", Enabled);
        
        props.Header("Uniforms");
        props.Sequence("Uniforms", Uniforms);
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_SHADER_COMPONENT_H
