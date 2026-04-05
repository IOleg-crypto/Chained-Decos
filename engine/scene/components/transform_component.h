#ifndef CH_TRANSFORM_COMPONENT_H
#define CH_TRANSFORM_COMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "engine/core/reflection.h"

namespace CHEngine
{
struct TransformComponent
{
    glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation = {0.0f, 0.0f, 0.0f}; // Euler angles (radians)
    glm::quat RotationQuat = {1.0f, 0.0f, 0.0f, 0.0f}; // GLM quat is w, x, y, z constructor? No, glm::quat(w, x, y, z)
    glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& translation)
        : Translation(translation), IsDirty(true)
    {
    }

    void SetTranslation(const glm::vec3& translation)
    {
        Translation = translation;
        IsDirty = true;
    }

    void SetScale(const glm::vec3& scale)
    {
        Scale = scale;
        IsDirty = true;
    }

    void SetRotation(const glm::vec3& euler)
    {
        Rotation = euler;
        RotationQuat = glm::quat(euler);
        IsDirty = true;
    }

    void SetRotationQuat(const glm::quat& quat)
    {
        RotationQuat = quat;
        Rotation = glm::eulerAngles(quat);
        IsDirty = true;
    }

    glm::mat4 GetTransform() const
    {
        return GetTransform(Translation, RotationQuat, Scale);
    }

    static glm::mat4 GetTransform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
    {
        return glm::translate(glm::mat4(1.0f), translation) * 
               glm::toMat4(rotation) * 
               glm::scale(glm::mat4(1.0f), scale);
    }

    glm::mat4 GetInterpolatedTransform(float alpha) const
    {
        glm::vec3 t = glm::mix(PrevTranslation, Translation, alpha);
        glm::quat q = glm::slerp(PrevRotationQuat, RotationQuat, alpha);
        glm::vec3 s = glm::mix(PrevScale, Scale, alpha);

        return GetTransform(t, q, s);
    }

    // World transform cache
    glm::mat4 WorldTransform = glm::mat4(1.0f);
    bool IsDirty = true;

    // Previous state for interpolation
    glm::vec3 PrevTranslation = {0, 0, 0};
    glm::quat PrevRotationQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 PrevScale = {1, 1, 1};

    
    CH_REFLECT_BEGIN(TransformComponent)
        // Position with reasonable range
        props.Property("Translation", Translation);
        
        // Rotation with slider hint (in radians, -π to π range)
        props.Property("Rotation", Rotation, PropertyMeta(-3.14159f, 3.14159f, 0.01f));
        
        // Scale with slider hint (0.1 to 10.0 range)
        props.Property("Scale", Scale, PropertyMeta(0.1f, 10.0f, 0.1f));

        // Sync rotation quat after change
        if (props.HasChanged() || props.GetMode() == ReflectionMode::Deserialize)
        {
             SetRotation(Rotation);
        }
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_TRANSFORM_COMPONENT_H
