#ifndef CH_RENDERER_TYPES_H
#define CH_RENDERER_TYPES_H

#include "engine/core/ch_math.h"
#include "engine/core/reflection.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include "engine/graphics/api/vertex_array.h"

namespace CHEngine
{
    struct ShaderUniform
    {
        std::string Name;
        int Type; // 0: Float, 1: Vec2, 2: Vec3, 3: Vec4, 4: Color
        float Value[4] = {0, 0, 0, 0};

        CH_REFLECT_BEGIN(ShaderUniform)
        props.Property("Name", Name);
        static const char* types[] = {"Float", "Vec2", "Vec3", "Vec4", "Color"};
        if (props.Enum("Type", Type, types, 5))
        {
            // Zero out values when type changes to avoid mess
            memset(Value, 0, sizeof(Value));
        }

        if (Type == 4) // Color
        {
            CHEngine::Color c = {(unsigned char)glm::clamp(Value[0] * 255.0f, 0.0f, 255.0f),
                                (unsigned char)glm::clamp(Value[1] * 255.0f, 0.0f, 255.0f),
                                (unsigned char)glm::clamp(Value[2] * 255.0f, 0.0f, 255.0f),
                                (unsigned char)glm::clamp(Value[3] * 255.0f, 0.0f, 255.0f)};
            if (props.Property("Value", c))
            {
                Value[0] = c.r / 255.0f;
                Value[1] = c.g / 255.0f;
                Value[2] = c.b / 255.0f;
                Value[3] = c.a / 255.0f;
            }
        }
        else if (Type == 1) // Vec2
        {
            props.Property("Value", *(glm::vec2*)Value);
        }
        else if (Type == 2) // Vec3
        {
            props.Property("Value", *(glm::vec3*)Value);
        }
        else if (Type == 3) // Vec4
        {
            props.Property("Value", *(glm::vec4*)Value);
        }
        else // Float
        {
            props.Property("Value", Value[0]);
        }
        CH_REFLECT_END()
    };
    struct Material
    {
        glm::vec4 AlbedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 EmissiveColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        float EmissiveIntensity = 0.0f;
        float Metalness = 0.0f;
        float Roughness = 0.5f;

        uint32_t AlbedoMap = 0;
        uint32_t NormalMap = 0;
        uint32_t MetallicRoughnessMap = 0;
        uint32_t EmissiveMap = 0;
        uint32_t OcclusionMap = 0;

        std::string AlbedoPath;
        std::string NormalPath;
        std::string MetallicRoughnessPath;
        std::string EmissivePath;
        std::string OcclusionPath;

        uint32_t ShaderID = 0;
        bool Transparent = false;
        float Alpha = 1.0f;

        std::string Name;
    };

    struct Mesh
    {
        std::shared_ptr<VertexArray> VAO;
        uint32_t VertexCount = 0;
        uint32_t TriangleCount = 0;
        int MaterialIndex = 0;

        glm::vec3 MinBounds = { 0.0f, 0.0f, 0.0f };
        glm::vec3 MaxBounds = { 0.0f, 0.0f, 0.0f };
    };

    struct Model
    {
        std::vector<Mesh> Meshes;
        std::vector<Material> Materials;
        glm::mat4 Transform = glm::mat4(1.0f);

        static Model CreateFromFile(const std::string& path);
    };
}

#endif // CH_RENDERER_TYPES_H
