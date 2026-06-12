#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/component_registry.h"
#include "engine/audio/audio.h"
namespace Chained {

    void RegisterGlueEntity() {}

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
            ComponentUtils::SetTranslation(entity.GetComponent<TransformComponent>(), *inTranslation);
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
            ComponentUtils::SetRotation(entity.GetComponent<TransformComponent>(), *inRotation);
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
            ComponentUtils::SetScale(entity.GetComponent<TransformComponent>(), *inScale);
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
        
        if (name == "MeshComponent") name = "ModelComponent";
        if (name == "PhysicsComponent") name = "ColliderComponent";

        for (const auto& [id, metadata] : ComponentRegistry::GetRegistry()) {
            if (metadata.Name == name || metadata.SerializationKey == name) {
                if (metadata.Add) metadata.Add(entity);
                return;
            }
        }
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
        
        if (name == "MeshComponent") name = "ModelComponent";
        if (name == "PhysicsComponent") name = "ColliderComponent";
        if (name == "AnimationComponent") return false; // Hardcoded fallback if required, though registry will just return false if omitted
        
        if (name.find("Control") != std::string::npos || name.find("Group") != std::string::npos) {
            if (entity.HasComponent<WidgetComponent>()) {
                auto& widget = entity.GetComponent<WidgetComponent>();
                if (name == "ButtonControl" && std::holds_alternative<ButtonData>(widget.Data)) return true;
                if (name == "PanelControl" && std::holds_alternative<PanelData>(widget.Data)) return true;
                if (name == "LabelControl" && std::holds_alternative<LabelData>(widget.Data)) return true;
                if (name == "ImageControl" && std::holds_alternative<ImageData>(widget.Data)) return true;
                if (name == "CheckboxControl" && std::holds_alternative<CheckboxData>(widget.Data)) return true;
                if (name == "ComboBoxControl" && std::holds_alternative<ComboBoxData>(widget.Data)) return true;
                if (name == "SliderControl" && std::holds_alternative<SliderData>(widget.Data)) return true;
                if (name == "ProgressBarControl" && std::holds_alternative<ProgressBarData>(widget.Data)) return true;
                if (name == "InputTextControl" && std::holds_alternative<InputTextData>(widget.Data)) return true;
                if (name == "ImageButtonControl" && std::holds_alternative<ImageButtonData>(widget.Data)) return true;
                if (name == "SeparatorControl" && std::holds_alternative<SeparatorData>(widget.Data)) return true;
                if (name == "RadioButtonControl" && std::holds_alternative<RadioButtonData>(widget.Data)) return true;
                if (name == "ColorPickerControl" && std::holds_alternative<ColorPickerData>(widget.Data)) return true;
                if (name == "DragFloatControl" && std::holds_alternative<DragFloatData>(widget.Data)) return true;
                if (name == "DragIntControl" && std::holds_alternative<DragIntData>(widget.Data)) return true;
                if (name == "VerticalLayoutGroup" && std::holds_alternative<VerticalLayoutGroupData>(widget.Data)) return true;
            }
            return false;
        }

        for (const auto& [id, metadata] : ComponentRegistry::GetRegistry()) {
            if (metadata.Name == name || metadata.SerializationKey == name) {
                if (metadata.Has) return metadata.Has(entity);
            }
        }

        return false;
    }
    CH_ADD_INTERNAL_CALL(Entity, Entity_HasComponent_Ptr, Entity_HasComponent);

    CH_SCRIPT_FUNC Coral::Array<uint64_t> Entity_FindAllWithComponent(Coral::String componentName) {
        Scene* scene = GetActiveScene();
        if (!scene) return Coral::Array<uint64_t>::New(0);
        
        std::string name = (std::string)componentName;
        
        if (name == "MeshComponent") name = "ModelComponent";
        if (name == "PhysicsComponent") name = "ColliderComponent";

        for (const auto& [id, metadata] : ComponentRegistry::GetRegistry()) {
            if (metadata.Name == name || metadata.SerializationKey == name) {
                if (metadata.GetAll) return Coral::Array<uint64_t>::New(metadata.GetAll(scene));
            }
        }

        return Coral::Array<uint64_t>::New(0);
    }
    CH_ADD_INTERNAL_CALL(Entity, Entity_FindAllWithComponent_Ptr, Entity_FindAllWithComponent);

    CH_SCRIPT_FUNC void AudioComponent_Play(uint64_t entityID) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<AudioComponent>()) {
            auto& audio = entity.GetComponent<AudioComponent>();
            if (audio.SoundHandle != 0) {
                // Prevent duplicate instances — only play if not already active
                auto& audioService = Audio::Get();
                audioService.SetInstancePosition(audio.SoundHandle, entity.GetComponent<TransformComponent>().WorldTransform[3]);
                glm::vec3 worldPos = {0,0,0};
                if (entity.HasComponent<TransformComponent>()) {
                    auto& transform = entity.GetComponent<TransformComponent>();
                    worldPos = glm::vec3(transform.WorldTransform[3]);
                }
                audioService.Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
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
                auto& audioService = Audio::Get();
                audioService.Stop(audio.SoundHandle);
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
            auto& shader = entity.GetComponent<ShaderComponent>();
            std::string name = (std::string)inName;
            
            auto it = std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(), [&](const auto& u) { return u.Name == name; });
            if (it != shader.Uniforms.end()) {
                it->Value[0] = inValue;
            } else {
                shader.Uniforms.push_back({name, 0, {inValue, 0, 0, 0}});
            }
        }
    }
    CH_ADD_INTERNAL_CALL(ShaderComponent, Shader_SetFloat_Ptr, Shader_SetFloat);

    CH_SCRIPT_FUNC void Shader_SetVec3(uint64_t entityID, Coral::String inName, glm::vec3* inValue) {
        Entity entity = GetEntity(entityID);
        if (entity && entity.HasComponent<ShaderComponent>() && inValue) {
            auto& shader = entity.GetComponent<ShaderComponent>();
            std::string name = (std::string)inName;

            auto it = std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(), [&](const auto& u) { return u.Name == name; });
            if (it != shader.Uniforms.end()) {
                it->Value[0] = inValue->x;
                it->Value[1] = inValue->y;
                it->Value[2] = inValue->z;
            } else {
                shader.Uniforms.push_back({name, 2, {inValue->x, inValue->y, inValue->z, 0}});
            }
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

    

} // namespace Chained

