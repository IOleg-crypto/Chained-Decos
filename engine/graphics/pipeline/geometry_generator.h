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

        // Generates a unit cube mesh for skyboxes/cubemaps
        static Mesh GenerateUnitCube();

        // Generates a grid mesh
        static Mesh GenerateGrid(int slices, float spacing);

        // Generates a fullscreen quad or a simple plane quad
        static Mesh GenerateQuad(float size);
    };
}

#endif // CH_GEOMETRY_GENERATOR_H
