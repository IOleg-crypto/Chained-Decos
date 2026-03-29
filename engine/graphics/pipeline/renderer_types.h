#ifndef CH_RENDERER_TYPES_H
#define CH_RENDERER_TYPES_H

#include "engine/core/ch_math.h"
#include <string>
#include <vector>
#include <memory>
#include "engine/graphics/api/vertex_array.h"

namespace CHEngine
{
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

        uint32_t ShaderID = 0;

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
    };
}

#endif // CH_RENDERER_TYPES_H
