#include "script_glue_internal.h"
#include "script_internal_call_registry.h"

namespace CHEngine {

    // ── Entity / Transform ────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Entity_GetTranslation(uint64_t entityID, glm::vec3* outTranslation) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outTranslation = entity.GetComponent<TransformComponent>().Translation;
    }
    CH_ADD_INTERNAL_CALL(TransformComponent, Transform_GetTranslation_Ptr, Entity_GetTranslation);

    CH_SCRIPT_FUNC void Entity_SetTranslation(uint64_t entityID, glm::vec3* inTranslation) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().SetTranslation(*inTranslation);
    }
    CH_ADD_INTERNAL_CALL(TransformComponent, Transform_SetTranslation_Ptr, Entity_SetTranslation);

    CH_SCRIPT_FUNC void Entity_GetRotation(uint64_t entityID, glm::vec3* outRotation) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }
    CH_ADD_INTERNAL_CALL(TransformComponent, Transform_GetRotation_Ptr, Entity_GetRotation);

    CH_SCRIPT_FUNC void Entity_SetRotation(uint64_t entityID, glm::vec3* inRotation) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().SetRotation(*inRotation);
    }
    CH_ADD_INTERNAL_CALL(TransformComponent, Transform_SetRotation_Ptr, Entity_SetRotation);

    CH_SCRIPT_FUNC void Entity_GetScale(uint64_t entityID, glm::vec3* outScale) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outScale = entity.GetComponent<TransformComponent>().Scale;
    }
    CH_ADD_INTERNAL_CALL(TransformComponent, Transform_GetScale_Ptr, Entity_GetScale);

    CH_SCRIPT_FUNC void Entity_SetScale(uint64_t entityID, glm::vec3* inScale) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().SetScale(*inScale);
    }
    CH_ADD_INTERNAL_CALL(TransformComponent, Transform_SetScale_Ptr, Entity_SetScale);

    CH_SCRIPT_FUNC Coral::String Entity_GetModelPath(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<ModelComponent>() ? Coral::String::New(entity.GetComponent<ModelComponent>().ModelPath) : Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(ModelComponent, Model_GetModelPath_Ptr, Entity_GetModelPath);

    CH_SCRIPT_FUNC void Entity_SetModelPath(uint64_t entityID, Coral::String inPath) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<ModelComponent>())
            entity.GetComponent<ModelComponent>().ModelPath = (std::string)inPath;
    }
    CH_ADD_INTERNAL_CALL(ModelComponent, Model_SetModelPath_Ptr, Entity_SetModelPath);

    CH_SCRIPT_FUNC void Entity_AddComponent(uint64_t entityID, Coral::String componentName) {
        Entity entity = GetEntity(entityID);
        if (!entity) return;
        
        std::string name = (std::string)componentName;
        if (name == "TransformComponent") entity.AddComponent<TransformComponent>();
        else if (name == "RigidBodyComponent") entity.AddComponent<RigidBodyComponent>();
        else if (name == "ModelComponent") entity.AddComponent<ModelComponent>();
        else if (name == "TagComponent") entity.AddComponent<TagComponent>();
        else if (name == "AudioComponent") entity.AddComponent<AudioComponent>();
        else if (name == "CameraComponent") entity.AddComponent<CameraComponent>();
        else if (name == "ShaderComponent") entity.AddComponent<ShaderComponent>();
        else if (name == "ManagedScriptComponent") entity.AddComponent<ManagedScriptComponent>();
        else if (name == "SpriteComponent") entity.AddComponent<SpriteComponent>();
        else if (name == "PlayerComponent") entity.AddComponent<PlayerComponent>();
    }
    CH_ADD_INTERNAL_CALL(Entity, Entity_AddComponent_Ptr, Entity_AddComponent);

    CH_SCRIPT_FUNC void Entity_GetVelocity(uint64_t entityID, glm::vec3* outVelocity) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<RigidBodyComponent>()) 
            *outVelocity = entity.GetComponent<RigidBodyComponent>().Velocity;
    }
    CH_ADD_INTERNAL_CALL(RigidBodyComponent, RigidBody_GetVelocity_Ptr, Entity_GetVelocity);

    CH_SCRIPT_FUNC void Entity_SetVelocity(uint64_t entityID, glm::vec3* inVelocity) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<RigidBodyComponent>()) 
            entity.GetComponent<RigidBodyComponent>().Velocity = *inVelocity;
    }
    CH_ADD_INTERNAL_CALL(RigidBodyComponent, RigidBody_SetVelocity_Ptr, Entity_SetVelocity);

    CH_SCRIPT_FUNC bool Entity_IsGrounded(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<RigidBodyComponent>() ? entity.GetComponent<RigidBodyComponent>().IsGrounded : false;
    }
    CH_ADD_INTERNAL_CALL(RigidBodyComponent, RigidBody_IsGrounded_Ptr, Entity_IsGrounded);

    CH_SCRIPT_FUNC bool Entity_IsKinematic(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<RigidBodyComponent>() ? entity.GetComponent<RigidBodyComponent>().IsKinematic : false;
    }
    CH_ADD_INTERNAL_CALL(RigidBodyComponent, RigidBody_IsKinematic_Ptr, Entity_IsKinematic);

    CH_SCRIPT_FUNC void Entity_SetKinematic(uint64_t entityID, bool isKinematic) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<RigidBodyComponent>())
            entity.GetComponent<RigidBodyComponent>().IsKinematic = isKinematic;
    }
    CH_ADD_INTERNAL_CALL(RigidBodyComponent, RigidBody_SetKinematic_Ptr, Entity_SetKinematic);

    CH_SCRIPT_FUNC bool Entity_HasComponent(uint64_t entityID, Coral::String componentName) {
        Entity entity = GetEntity(entityID);
        if (!entity) return false;
        
        std::string name = (std::string)componentName;
        if (name == "TransformComponent") return entity.HasComponent<TransformComponent>();
        if (name == "TagComponent")       return entity.HasComponent<TagComponent>();
        if (name == "MeshComponent" || name == "ModelComponent") return entity.HasComponent<ModelComponent>();
        if (name == "MaterialComponent")  return entity.HasComponent<MaterialComponent>();
        if (name == "SpriteComponent")    return entity.HasComponent<SpriteComponent>();
        if (name == "LightComponent")     return entity.HasComponent<LightComponent>();
        if (name == "CameraComponent")    return entity.HasComponent<CameraComponent>();
        if (name == "AudioComponent")     return entity.HasComponent<AudioComponent>();
        if (name == "RigidBodyComponent") return entity.HasComponent<RigidBodyComponent>();
        if (name == "PhysicsComponent" || name == "ColliderComponent") return entity.HasComponent<ColliderComponent>();
        if (name == "AnimationComponent") return entity.HasComponent<AnimationComponent>();
        if (name == "HierarchyComponent") return entity.HasComponent<HierarchyComponent>();
        if (name == "IDComponent")        return entity.HasComponent<IDComponent>();
        if (name == "ManagedScriptComponent") return entity.HasComponent<ManagedScriptComponent>();
        if (name == "PlayerComponent")    return entity.HasComponent<PlayerComponent>();
        if (name == "SpawnComponent")     return entity.HasComponent<SpawnComponent>();
        if (name == "SceneTransitionComponent") return entity.HasComponent<SceneTransitionComponent>();
        if (name == "ShaderComponent")   return entity.HasComponent<ShaderComponent>();
        if (name == "ControlComponent")   return entity.HasComponent<ControlComponent>();
        if (name.find("Control") != std::string::npos || name.find("Group") != std::string::npos) {
             if (name == "ButtonControl")      return entity.HasComponent<ButtonControl>();
             if (name == "PanelControl")       return entity.HasComponent<PanelControl>();
             if (name == "LabelControl")       return entity.HasComponent<LabelControl>();
             if (name == "ImageControl")       return entity.HasComponent<ImageControl>();
             if (name == "CheckboxControl")    return entity.HasComponent<CheckboxControl>();
             if (name == "ComboBoxControl")    return entity.HasComponent<ComboBoxControl>();
             if (name == "SliderControl")      return entity.HasComponent<SliderControl>();
             if (name == "ProgressBarControl") return entity.HasComponent<ProgressBarControl>();
             if (name == "InputTextControl")   return entity.HasComponent<InputTextControl>();
             if (name == "ImageButtonControl") return entity.HasComponent<ImageButtonControl>();
             if (name == "SeparatorControl")   return entity.HasComponent<SeparatorControl>();
             if (name == "RadioButtonControl") return entity.HasComponent<RadioButtonControl>();
             if (name == "ColorPickerControl") return entity.HasComponent<ColorPickerControl>();
             if (name == "DragFloatControl")   return entity.HasComponent<DragFloatControl>();
             if (name == "DragIntControl")     return entity.HasComponent<DragIntControl>();
             if (name == "VerticalLayoutGroup") return entity.HasComponent<VerticalLayoutGroup>();
        }

        return false;
    }
    CH_ADD_INTERNAL_CALL(Entity, Entity_HasComponent_Ptr, Entity_HasComponent);

    CH_SCRIPT_FUNC Coral::Array<uint64_t> Entity_FindAllWithComponent(Coral::String componentName) {
        Scene* scene = GetActiveScene();
        if (!scene) return Coral::Array<uint64_t>::New(0);
        
        std::string name = (std::string)componentName;
        std::vector<uint64_t> ids;

        auto addToVec = [&](auto view) {
            for (auto entity : view) ids.push_back((uint64_t)(uint32_t)entity);
        };

        if (name == "TransformComponent") addToVec(scene->GetRegistry().view<TransformComponent>());
        else if (name == "RigidBodyComponent") addToVec(scene->GetRegistry().view<RigidBodyComponent>());
        else if (name == "CameraComponent")    addToVec(scene->GetRegistry().view<CameraComponent>());
        else if (name == "PlayerComponent")    addToVec(scene->GetRegistry().view<PlayerComponent>());
        else if (name == "AudioComponent")     addToVec(scene->GetRegistry().view<AudioComponent>());
        else if (name == "TagComponent")       addToVec(scene->GetRegistry().view<TagComponent>());
        else if (name == "ShaderComponent")    addToVec(scene->GetRegistry().view<ShaderComponent>());

        return Coral::Array<uint64_t>::New(ids);
    }
    CH_ADD_INTERNAL_CALL(Entity, Entity_FindAllWithComponent_Ptr, Entity_FindAllWithComponent);

    CH_SCRIPT_FUNC void AudioComponent_Play(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) {
            auto& audio = entity.GetComponent<AudioComponent>();
            if (audio.SoundHandle != 0) {
                glm::vec3 worldPos = {0,0,0};
                if (entity.HasComponent<TransformComponent>()) {
                    auto& transform = entity.GetComponent<TransformComponent>();
                    worldPos = glm::vec3(transform.WorldTransform[3]);
                }
                Audio::Get().Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
                audio.IsPlaying = true;
            }
        }
    }
    CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_Play_Ptr, AudioComponent_Play);

    CH_SCRIPT_FUNC void AudioComponent_Stop(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) {
            auto& audio = entity.GetComponent<AudioComponent>();
            if (audio.SoundHandle != 0 && audio.IsPlaying) {
                Audio::Get().Stop(audio.SoundHandle);
                audio.IsPlaying = false;
            }
        }
    }
    CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_Stop_Ptr, AudioComponent_Stop);

    CH_SCRIPT_FUNC Coral::String TagComponent_GetTag(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<TagComponent>()) 
            return Coral::String::New(entity.GetComponent<TagComponent>().Tag); 
        return Coral::String::New("");
    }
    CH_ADD_INTERNAL_CALL(TagComponent, TagComponent_GetTag_Ptr, TagComponent_GetTag);

    CH_SCRIPT_FUNC void Shader_SetFloat(uint64_t entityID, Coral::String inName, float inValue) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<ShaderComponent>()) {
            entity.GetComponent<ShaderComponent>().SetFloat((std::string)inName, inValue);
            // CH_CORE_INFO("Shader_SetFloat: entity={}, name={}, value={}", entityID, (std::string)inName, inValue);
        }
    }
    CH_ADD_INTERNAL_CALL(ShaderComponent, Shader_SetFloat_Ptr, Shader_SetFloat);

    CH_SCRIPT_FUNC void Shader_SetVec3(uint64_t entityID, Coral::String inName, glm::vec3* inValue) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<ShaderComponent>()) {
            entity.GetComponent<ShaderComponent>().SetVec3((std::string)inName, *inValue);
        }
    }
    CH_ADD_INTERNAL_CALL(ShaderComponent, Shader_SetVec3_Ptr, Shader_SetVec3);

    CH_SCRIPT_FUNC bool Shader_GetEnabled(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        return entity && entity.HasComponent<ShaderComponent>() ? entity.GetComponent<ShaderComponent>().Enabled : false;
    }
    CH_ADD_INTERNAL_CALL(ShaderComponent, Shader_GetEnabled_Ptr, Shader_GetEnabled);

    CH_SCRIPT_FUNC void Shader_SetEnabled(uint64_t entityID, bool enabled) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<ShaderComponent>()) {
            entity.GetComponent<ShaderComponent>().Enabled = enabled;
        }
    }
    CH_ADD_INTERNAL_CALL(ShaderComponent, Shader_SetEnabled_Ptr, Shader_SetEnabled);

} // namespace CHEngine

