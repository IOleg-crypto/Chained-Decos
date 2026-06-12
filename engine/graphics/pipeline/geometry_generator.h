#ifndef CH_GEOMETRY_GENERATOR_H
#define CH_GEOMETRY_GENERATOR_H

#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/api/model_data.h"
#include <string>

namespace Chained
{
    struct ProceduralParameters
    {
        float Radius = 0.5f;
        float InnerRadius = 0.2f;
        float Height = 1.0f;
        int Slices = 16;
        int Stacks = 16;
        glm::vec3 Dimensions = {1.0f, 1.0f, 1.0f};
    };

    class GeometryGenerator
    {
    public:
        // Generates a sphere mesh
        static Mesh GenerateSphere(float radius, int slices, int stacks);

        // Generates a unit cube mesh for skyboxes/cubemaps (triangles)
        static Mesh GenerateUnitCube();

        // Generates a wireframe cube with 12 edges for debug drawing (GL_LINES)
        static Mesh GenerateWireCube();

        // Generates a grid mesh
        static Mesh GenerateGrid(int slices, float spacing);

        // Generates a fullscreen quad or a simple plane quad
        static Mesh GenerateQuad(float size);

        // Generates a capsule mesh (cylinder + two hemispheres)
        static Mesh GenerateCapsule(float radius, float height, int slices, int stacks);

         // Generate a cube mesh with given dimensions (for procedural cube generation)
        static Mesh GenerateCube(const glm::vec3& dimensions);

        // Generate procedural model (composed of one or more meshes) using generic type strings
        static Model GenerateProceduralModel(const std::string& type, const ProceduralParameters& params);
    };
}

#endif // CH_GEOMETRY_GENERATOR_H
