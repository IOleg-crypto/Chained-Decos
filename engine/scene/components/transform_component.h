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

    // Cached world transform.
    glm::mat4 WorldTransform = glm::mat4(1.0f);
    glm::mat4 InverseWorldTransform = glm::mat4(1.0f);
    bool IsDirty = true;

    // Previous state for interpolation.
    glm::vec3 PrevTranslation = {0, 0, 0};
    glm::quat PrevRotationQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 PrevScale = {1, 1, 1};

    CH_REFLECT_BEGIN(TransformComponent)
        // Position.
        CH_PROP(props, Translation);
        
        // Rotation in radians.
        CH_PROP_META(props, Rotation, PropertyMeta(-3.14159f, 3.14159f, 0.01f));
        
        // Scale.
        CH_PROP_META(props, Scale, PropertyMeta(0.1f, 10.0f, 0.1f));

        // Keep the quaternion in sync using an inline lambda or similar if we want to avoid member functions
        if (props.HasChanged() || props.GetMode() == ReflectionMode::Deserialize)
        {
             RotationQuat = glm::quat(Rotation);
             IsDirty = true;
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_TRANSFORM_COMPONENT_H
