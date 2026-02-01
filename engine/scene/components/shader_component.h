#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include <string>
#include <vector>
#include "raylib.h"

namespace CHEngine
{
    struct ShaderUniform
    {
        std::string Name;
        int Type; // 0: Float, 1: Vec2, 2: Vec3, 3: Vec4, 4: Color
        float Value[4] = {0,0,0,0};
    };

    struct ShaderComponent
    {
        std::string ShaderPath;
        std::vector<ShaderUniform> Uniforms;
        bool Enabled = true;

        ShaderComponent() = default;
        ShaderComponent(const ShaderComponent&) = default;
    };
}

#endif // CH_SHADER_COMPONENT_H
