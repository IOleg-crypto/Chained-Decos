#ifndef CH_TRANSFORM_COMPONENT_H
#define CH_TRANSFORM_COMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "engine/core/reflection.h"

namespace CHEngine
{
// Local transform state and cached world matrix.
struct TransformComponent
{
    glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation = {0.0f, 0.0f, 0.0f}; // Euler angles in radians.
    glm::quat RotationQuat = {1.0f, 0.0f, 0.0f, 0.0f}; // GLM uses w, x, y, z.
    glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& translation)
        : Translation(translation), IsDirty(true)
    {
    }

    // Sets translation and marks the transform dirty.
    void SetTranslation(const glm::vec3& translation)
    {
        Translation = translation;
        IsDirty = true;
    }

    // Sets scale and marks the transform dirty.
    void SetScale(const glm::vec3& scale)
    {
        Scale = scale;
        IsDirty = true;
    }

    // Sets Euler rotation and refreshes the cached quaternion.
    void SetRotation(const glm::vec3& eulerAngles)
    {
        Rotation = eulerAngles;
        RotationQuat = glm::quat(eulerAngles);
        IsDirty = true;
    }

    // Sets the quaternion and refreshes Euler angles.
    void SetRotationQuat(const glm::quat& rotationQuat)
    {
        RotationQuat = rotationQuat;
        Rotation = glm::eulerAngles(rotationQuat);
        IsDirty = true;
    }

    // Returns the current local transform matrix.
    glm::mat4 GetTransform() const
    {
        return GetTransform(Translation, RotationQuat, Scale);
    }

    // Returns a transform matrix from explicit translation, rotation, and scale.
    static glm::mat4 GetTransform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
    {
        return glm::translate(glm::mat4(1.0f), translation) * 
               glm::toMat4(rotation) * 
               glm::scale(glm::mat4(1.0f), scale);
    }

    // Returns an interpolated transform.
    glm::mat4 GetInterpolatedTransform(float alpha) const
    {
        glm::vec3 interpolatedTranslation = glm::mix(PrevTranslation, Translation, alpha);
        glm::quat interpolatedRotation = glm::slerp(PrevRotationQuat, RotationQuat, alpha);
        glm::vec3 interpolatedScale = glm::mix(PrevScale, Scale, alpha);

        return GetTransform(interpolatedTranslation, interpolatedRotation, interpolatedScale);
    }

    // Cached world transform.
    glm::mat4 WorldTransform = glm::mat4(1.0f);
    bool IsDirty = true;

    // Previous state for interpolation.
    glm::vec3 PrevTranslation = {0, 0, 0};
    glm::quat PrevRotationQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 PrevScale = {1, 1, 1};

    
    CH_REFLECT_BEGIN(TransformComponent)
        // Position.
        props.Property("Translation", Translation);
        
        // Rotation in radians.
        props.Property("Rotation", Rotation, PropertyMeta(-3.14159f, 3.14159f, 0.01f));
        
        // Scale.
        props.Property("Scale", Scale, PropertyMeta(0.1f, 10.0f, 0.1f));

        // Keep the quaternion in sync.
        if (props.HasChanged() || props.GetMode() == ReflectionMode::Deserialize)
        {
             SetRotation(Rotation);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_TRANSFORM_COMPONENT_H
