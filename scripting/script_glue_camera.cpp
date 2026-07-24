#include "script_glue_camera.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"


namespace Chained
{
void Camera_GetForward(uint64_t entityID, glm::vec3* outForward)
{
    if (!outForward)
        return;
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>())
    {
        auto& tc = entity.GetComponent<TransformComponent>();
        glm::quat rotation = tc.RotationQuat;
        *outForward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }
    else
    {
        CH_CORE_WARN("[Diag Camera_GetForward] entityID={} NOT FOUND or no TransformComponent!", entityID);
        *outForward = {};
    }
}
void Camera_GetRight(uint64_t entityID, glm::vec3* outRight)
{
    if (!outRight)
        return;
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>())
    {
        auto& tc = entity.GetComponent<TransformComponent>();
        glm::quat rotation = tc.RotationQuat;
        *outRight = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        *outRight = {};
    }
}
void Camera_SetOrbit(uint64_t entityID, float yaw, float pitch, float distance)
{
    //CH_CORE_INFO("[Diag Camera] SetOrbit entity={} yaw={} pitch={} dist={}", entityID, yaw, pitch, distance);
    Entity entity = GetEntity(entityID);
    Scene* scene = GetActiveScene();
    if (entity && entity.HasComponent<CameraComponent>() && entity.HasComponent<TransformComponent>() && scene)
    {
        auto& camera = entity.GetComponent<CameraComponent>();
        camera.OrbitYaw = yaw;
        camera.OrbitPitch = pitch;
        camera.OrbitDistance = distance;

        // Sync with TransformComponent
        auto& tc = entity.GetComponent<TransformComponent>();
        Entity target = scene->FindEntityByTag(camera.TargetEntityTag);

        glm::vec3 targetPos = glm::vec3(0.0f);
        if (target && target.HasComponent<TransformComponent>())
        {
            // Fix: Use WorldTransform instead of local translation to support parented players
            const auto& targetTC = target.GetComponent<TransformComponent>();
            targetPos = glm::vec3(targetTC.WorldTransform[3]);
        }

        // Calculate rotation from orbit angles
        // Convention: Yaw rotates around Y, Pitch rotates around X
        float yawRad = glm::radians(yaw);
        float pitchRad = glm::radians(pitch);

        // Standard orbital rotation: Pitch then Yaw
        glm::quat rotation = glm::quat(glm::vec3(pitchRad, yawRad, 0.0f));
        glm::vec3 offset = rotation * glm::vec3(0.0f, 0.0f, distance);

        glm::vec3 newPos = targetPos + offset;
        ComponentUtils::SetTranslation(tc, newPos);
        ComponentUtils::SetRotationQuat(tc, rotation);
    }
}
void Camera_GetOrbit(uint64_t entityID, float* yaw, float* pitch, float* distance)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<CameraComponent>())
    {
        auto& camera = entity.GetComponent<CameraComponent>();
        if (yaw) *yaw = camera.OrbitYaw;
        if (pitch) *pitch = camera.OrbitPitch;
        if (distance) *distance = camera.OrbitDistance;
    }
}
bool Camera_GetPrimary(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().Primary : false;
}
void Camera_SetPrimary(uint64_t entityID, bool primary)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<CameraComponent>())
    {
        CH_CORE_TRACE("[ScriptGlue] Camera_SetPrimary: Entity={}, Primary={}", (uint32_t)entityID, primary);
        entity.GetComponent<CameraComponent>().Primary = primary;
    }
}
void Camera_SetIsOrbit(uint64_t entityID, bool isOrbit)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<CameraComponent>())
    {
        entity.GetComponent<CameraComponent>().IsOrbitCamera = isOrbit;
    }
}
static thread_local Coral::UCString s_CameraTagBuffer;

const Coral::UCChar* Camera_GetTargetTag(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    std::string tag =
        entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().TargetEntityTag : "";
    s_CameraTagBuffer = ToWide(tag);
    return s_CameraTagBuffer.c_str();
}
void Camera_SetTargetTag(uint64_t entityID, const Coral::UCChar* tag)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<CameraComponent>() && tag)
    {
        entity.GetComponent<CameraComponent>().TargetEntityTag = ch_u16_to_string(tag);
    }
}

bool Camera_GetIsOrbit(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().IsOrbitCamera
                                                            : false;
}
} // namespace Chained
