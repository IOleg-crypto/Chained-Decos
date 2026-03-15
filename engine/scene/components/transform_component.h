#ifndef CH_TRANSFORM_COMPONENT_H
#define CH_TRANSFORM_COMPONENT_H

#include "raylib.h"
#include "raymath.h"

namespace CHEngine
{
struct TransformComponent
{
    Vector3 Translation = {0.0f, 0.0f, 0.0f};
    Vector3 Rotation = {0.0f, 0.0f, 0.0f}; // Euler angles (radians) for UI
    Quaternion RotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 Scale = {1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const Vector3& translation)
        : Translation(translation), IsDirty(true)
    {
    }

    void SetTranslation(const Vector3& translation)
    {
        Translation = translation;
        IsDirty = true;
    }

    void SetScale(const Vector3& scale)
    {
        Scale = scale;
        IsDirty = true;
    }

    void SetRotation(const Vector3& euler)
    {
        Rotation = euler;
        RotationQuat = QuaternionFromEuler(euler.x, euler.y, euler.z);
        IsDirty = true;
    }

    void SetRotationQuat(const Quaternion& quat)
    {
        RotationQuat = quat;
        Rotation = QuaternionToEuler(quat);
        IsDirty = true;
    }

    Matrix GetTransform() const
    {
        Matrix rot = QuaternionToMatrix(RotationQuat);
        return MatrixMultiply(MatrixMultiply(MatrixScale(Scale.x, Scale.y, Scale.z), rot),
                              MatrixTranslate(Translation.x, Translation.y, Translation.z));
    }

    static Matrix GetTransform(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
    {
        Matrix rot = QuaternionToMatrix(rotation);
        return MatrixMultiply(MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), rot),
                              MatrixTranslate(translation.x, translation.y, translation.z));
    }

    Matrix GetInterpolatedTransform(float alpha) const
    {
        Vector3 t = Vector3Lerp(PrevTranslation, Translation, alpha);
        Quaternion q = QuaternionSlerp(PrevRotationQuat, RotationQuat, alpha);
        Vector3 s = Vector3Lerp(PrevScale, Scale, alpha);

        return GetTransform(t, q, s);
    }

    // World transform cache
    Matrix WorldTransform = MatrixIdentity();
    bool IsDirty = true;

    // Previous state for interpolation
    Vector3 PrevTranslation = {0, 0, 0};
    Quaternion PrevRotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 PrevScale = {1, 1, 1};
};

} // namespace CHEngine

#endif // CH_TRANSFORM_COMPONENT_H
