#ifndef CH_GEOMETRY_GENERATOR_H
#define CH_GEOMETRY_GENERATOR_H

#include "engine/graphics/pipeline/renderer_types.h"

namespace CHEngine
{
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

         // Generate a cube mesh with given dimensions (for procedural cube generation)
        static Mesh GenerateCube(const glm::vec3& dimensions);
    };
}

#endif // CH_GEOMETRY_GENERATOR_H
