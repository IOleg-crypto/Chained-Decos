#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <raylib.h>
#include <raymath.h>

namespace CHEngine
{
struct TransformComponent
{
    Vector3 position = {0, 0, 0};
    Vector3 rotation = {0, 0, 0};
    Vector3 scale = {1, 1, 1};

    // Helper methods
    Matrix GetMatrix() const
    {
        Matrix matScale = MatrixScale(scale.x, scale.y, scale.z);
        Matrix matRotation = MatrixRotateXYZ(rotation);
        Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);
        return MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
    }
};
} // namespace CHEngine

#endif // TRANSFORM_COMPONENT_H
