#ifndef CH_MATH_TYPES_H
#define CH_MATH_TYPES_H

#include <raylib.h>

namespace CHEngine
{
// Math types
using Vector2 = ::Vector2;
using Vector3 = ::Vector3;
using Vector4 = ::Vector4;
using Quaternion = ::Quaternion;
using Matrix = ::Matrix;

// Graphics types
using Color = ::Color;
using Texture2D = ::Texture2D;
using Image = ::Image;
using Mesh = ::Mesh;
using Model = ::Model;
using Material = ::Material;
using Shader = ::Shader;

// Geometric types
using BoundingBox = ::BoundingBox;
using Ray = ::Ray;
using RayCollision = ::RayCollision;

} // namespace CHEngine

#endif // CH_MATH_TYPES_H
