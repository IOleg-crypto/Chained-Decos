#ifndef CH_TRANSFORM_COMPONENT_H
#define CH_TRANSFORM_COMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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
};

} // namespace CHEngine

#endif // CH_TRANSFORM_COMPONENT_H
