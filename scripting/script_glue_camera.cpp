#include "script_glue_internal.h"

namespace CHEngine {

    // ── Camera ────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Camera_GetForward(uint64_t entityID, Vector3* outForward) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            *outForward = Vector3RotateByQuaternion({0.0f, 0.0f, -1.0f}, tc.RotationQuat);
        }
    }

    CH_SCRIPT_FUNC void Camera_GetRight(uint64_t entityID, Vector3* outRight) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            *outRight = Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, tc.RotationQuat);
        }
    }

    CH_SCRIPT_FUNC void Camera_GetOrbit(uint64_t entityID, float* yaw, float* pitch, float* distance) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) {
            auto& camera = entity.GetComponent<CameraComponent>();
            *yaw = camera.OrbitYaw;
            *pitch = camera.OrbitPitch;
            *distance = camera.OrbitDistance;
        }
    }

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
            
            Vector3 targetPos = {0, 0, 0};
            if (target && target.HasComponent<TransformComponent>()) {
                targetPos = target.GetComponent<TransformComponent>().Translation;
            }

            // Calculate rotation from orbit angles
            float yawRad = yaw * DEG2RAD;
            float pitchRad = pitch * DEG2RAD;
            Quaternion rotation = QuaternionFromEuler(pitchRad, yawRad, 0);
            Vector3 offset = Vector3RotateByQuaternion({0.0f, 0.0f, distance}, rotation);
            
            tc.SetTranslation(Vector3Add(targetPos, offset));
            tc.SetRotationQuat(rotation);
        }
    }

    CH_SCRIPT_FUNC bool Camera_GetPrimary(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().Primary : false;
    }

    CH_SCRIPT_FUNC void Camera_SetPrimary(uint64_t entityID, bool primary) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().Primary = primary;
    }

    CH_SCRIPT_FUNC bool Camera_GetIsOrbit(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().IsOrbitCamera : false;
    }

    CH_SCRIPT_FUNC void Camera_SetIsOrbit(uint64_t entityID, bool isOrbit) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().IsOrbitCamera = isOrbit;
    }

    CH_SCRIPT_FUNC Coral::String Camera_GetTargetTag(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<CameraComponent>() ? Coral::String::New(entity.GetComponent<CameraComponent>().TargetEntityTag) : Coral::String::New("");
    }

    CH_SCRIPT_FUNC void Camera_SetTargetTag(uint64_t entityID, Coral::String tag) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().TargetEntityTag = (std::string)tag;
    }

    void RegisterCameraInternalCalls(Coral::ManagedAssembly& assembly) {
        #define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr) assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)
        
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetForward_Ptr, Camera_GetForward);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetRight_Ptr, Camera_GetRight);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetOrbit_Ptr, Camera_GetOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetOrbit_Ptr, Camera_SetOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetPrimary_Ptr, Camera_GetPrimary);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetPrimary_Ptr, Camera_SetPrimary);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetIsOrbit_Ptr, Camera_GetIsOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetIsOrbit_Ptr, Camera_SetIsOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetTargetTag_Ptr, Camera_GetTargetTag);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetTargetTag_Ptr, Camera_SetTargetTag);

        #undef CH_ADD_INTERNAL_CALL
    }

} // namespace CHEngine
