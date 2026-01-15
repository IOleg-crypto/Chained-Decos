#ifndef CH_MATERIAL_H
#define CH_MATERIAL_H

#include "engine/core/math_types.h"
#include <string>

namespace CHEngine
{
struct MaterialInstance
{
    Color AlbedoColor = WHITE;
    std::string AlbedoPath = "";

    // Potential future fields
    // float Metalness = 0.0f;
    // float Roughness = 0.5f;
    // std::string NormalMapPath = "";

    MaterialInstance() = default;
};
} // namespace CHEngine

#endif // CH_MATERIAL_H
