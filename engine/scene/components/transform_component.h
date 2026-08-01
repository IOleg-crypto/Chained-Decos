#ifndef CH_TRANSFORM_COMPONENT_H
#define CH_TRANSFORM_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Chained
{
// Local transform state and cached world matrix.
struct TransformComponent
{
    glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};           // Euler angles in radians.
    glm::quat RotationQuat = {1.0f, 0.0f, 0.0f, 0.0f}; // GLM uses w, x, y, z.
    glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

    // Cached world transform.
    glm::mat4 WorldTransform = glm::mat4(1.0f);
    glm::mat4 InverseWorldTransform = glm::mat4(1.0f);
    bool TransformChanged = true;

    // Previous state for interpolation.
    glm::vec3 PrevTranslation = {0, 0, 0};
    glm::quat PrevRotationQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 PrevScale = {1, 1, 1};

    static const char* GetStaticName()
    {
        return "TransformComponent";
    }

    // Declarative UI layout layout metadata for compile-time reflection
    struct UI
    {
        UIMeta Translation = {.Tooltip = "Local position of the entity in world or parent space"};
        UIMeta Rotation = {
            .Min = -3.14159f, .Max = 3.14159f, .Speed = 0.01f, .Tooltip = "Local rotation angles specified in radians"};
        UIMeta Scale = {
            .Min = 0.1f, .Max = 10.0f, .Speed = 0.1f, .Tooltip = "Local scale multipliers along X, Y, and Z axes"};

        UIMeta TransformChanged = {.ReadOnly = true, .Transient = true};
        UIMeta PrevTranslation = {.ReadOnly = true, .Transient = true};
        UIMeta PrevRotationQuat = {.ReadOnly = true, .Transient = true};
        UIMeta PrevScale = {.ReadOnly = true, .Transient = true};
    };
};

CH_MARK_RFL(TransformComponent);

} // namespace Chained

#endif // CH_TRANSFORM_COMPONENT_H