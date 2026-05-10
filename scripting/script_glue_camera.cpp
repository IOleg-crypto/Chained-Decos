#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/scene/components/component_utils.h"

namespace CHEngine {

    void RegisterGlueCamera() {}

    // ── Camera ────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Camera_GetForward(uint64_t entityID, glm::vec3* outForward) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            glm::quat rotation = tc.RotationQuat;
            *outForward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        }
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetForward_Ptr, Camera_GetForward);

    CH_SCRIPT_FUNC void Camera_GetRight(uint64_t entityID, glm::vec3* outRight) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            glm::quat rotation = tc.RotationQuat;
            *outRight = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetRight_Ptr, Camera_GetRight);

    CH_SCRIPT_FUNC void Camera_GetOrbit(uint64_t entityID, float* yaw, float* pitch, float* distance) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) {
            auto& camera = entity.GetComponent<CameraComponent>();
            *yaw = camera.OrbitYaw;
            *pitch = camera.OrbitPitch;
            *distance = camera.OrbitDistance;
        }
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetOrbit_Ptr, Camera_GetOrbit);

    CH_SCRIPT_FUNC void Camera_SetOrbit(uint64_t entityID, float yaw, float pitch, float distance) {
        Entity entity = GetEntity(entityID);
        Scene* scene = GetActiveScene();
        if (entity && entity.HasComponent<CameraComponent>() && entity.HasComponent<TransformComponent>() && scene) {
            auto& camera = entity.GetComponent<CameraComponent>();
            camera.OrbitYaw = yaw;
            camera.OrbitPitch = pitch;
            camera.OrbitDistance = distance;

            // Sync with TransformComponent
            auto& tc = entity.GetComponent<TransformComponent>();
            Entity target = scene->FindEntityByTag(camera.TargetEntityTag);
            
            glm::vec3 targetPos = glm::vec3(0.0f);
            if (target && target.HasComponent<TransformComponent>()) {
                // Fix: Use WorldTransform instead of local translation to support parented players
                const auto& targetTC = target.GetComponent<TransformComponent>();
                targetPos = glm::vec3(targetTC.WorldTransform[3]);
            }

            CH_CORE_TRACE("[ScriptGlue] Camera_SetOrbit: Entity={}, Yaw={}, Pitch={}, Distance={}", (uint32_t)entityID, yaw, pitch, distance);

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
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetOrbit_Ptr, Camera_SetOrbit);

    CH_SCRIPT_FUNC bool Camera_GetPrimary(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().Primary : false;
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetPrimary_Ptr, Camera_GetPrimary);

    CH_SCRIPT_FUNC void Camera_SetPrimary(uint64_t entityID, bool primary) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) {
            CH_CORE_TRACE("[ScriptGlue] Camera_SetPrimary: Entity={}, Primary={}", (uint32_t)entityID, primary);
            entity.GetComponent<CameraComponent>().Primary = primary;
        }
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetPrimary_Ptr, Camera_SetPrimary);

    CH_SCRIPT_FUNC bool Camera_GetIsOrbit(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().IsOrbitCamera : false;
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetIsOrbit_Ptr, Camera_GetIsOrbit);

    CH_SCRIPT_FUNC void Camera_SetIsOrbit(uint64_t entityID, bool isOrbit) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().IsOrbitCamera = isOrbit;
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetIsOrbit_Ptr, Camera_SetIsOrbit);

    CH_SCRIPT_FUNC Coral::String Camera_GetTargetTag(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<CameraComponent>() ? Coral::String::New(entity.GetComponent<CameraComponent>().TargetEntityTag) : Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetTargetTag_Ptr, Camera_GetTargetTag);

    CH_SCRIPT_FUNC void Camera_SetTargetTag(uint64_t entityID, Coral::String tag) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().TargetEntityTag = (std::string)tag;
    }
    CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetTargetTag_Ptr, Camera_SetTargetTag);

} // namespace CHEngine

