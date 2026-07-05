#include "script_glue_entity.h"
#include "engine/audio/audio.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/components.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/entity.h"
#include "script_internal_call_registry.h"
namespace Chained
{
void Transform_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && outTranslation)
    {
        *outTranslation = entity.GetComponent<TransformComponent>().Translation;
    }
}
void Transform_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && inTranslation)
    {
        ComponentUtils::SetTranslation(entity.GetComponent<TransformComponent>(), *inTranslation);
    }
}
void Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && outRotation)
    {
        *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }
}
void Transform_SetRotation(uint64_t entityID, glm::vec3* inRotation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && inRotation)
    {
        ComponentUtils::SetRotation(entity.GetComponent<TransformComponent>(), *inRotation);
    }
}
void Transform_GetScale(uint64_t entityID, glm::vec3* outScale)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && outScale)
    {
        *outScale = entity.GetComponent<TransformComponent>().Scale;
    }
}
void Transform_SetScale(uint64_t entityID, glm::vec3* inScale)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && inScale)
    {
        ComponentUtils::SetScale(entity.GetComponent<TransformComponent>(), *inScale);
    }
}
Coral::String Model_GetModelPath(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return Coral::String::New(
        entity && entity.HasComponent<ModelComponent>() ? entity.GetComponent<ModelComponent>().ModelPath : "");
}
void Model_SetModelPath(uint64_t entityID, const char16_t* inPath)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<ModelComponent>())
    {
        entity.GetComponent<ModelComponent>().ModelPath = ch_u16_to_string(inPath);
    }
}
void Entity_AddComponent(uint64_t entityID, const char16_t* componentName)
{
    Entity entity = GetEntity(entityID);
    if (!entity)
    {
        return;
    }
    std::string name = ch_u16_to_string(componentName);
    if (name == "MeshComponent")
    {
        name = "ModelComponent";
    }
    if (name == "PhysicsComponent")
    {
        name = "ColliderComponent";
    }
    for (const auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Name == name || metadata.SerializationKey == name)
        {
            if (metadata.Add)
            {
                metadata.Add(entity);
            }
            return;
        }
    }
}
int Entity_FindAllWithComponent(const char16_t* componentName, uint64_t* outBuf, int bufSize)
{
    Scene* scene = GetActiveScene();
    if (!scene)
    {
        return 0;
    }
    std::string name = ch_u16_to_string(componentName);
    if (name == "MeshComponent")
    {
        name = "ModelComponent";
    }
    if (name == "PhysicsComponent")
    {
        name = "ColliderComponent";
    }
    for (const auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Name == name || metadata.SerializationKey == name)
        {
            if (metadata.GetAll)
            {
                auto ids = metadata.GetAll(scene);
                int count = (int)std::min(ids.size(), (size_t)bufSize);
                if (outBuf)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        outBuf[i] = ids[i];
                    }
                }
                return count;
            }
            return 0;
        }
    }
    return 0;
}
void RigidBody_GetVelocity(uint64_t entityID, glm::vec3* outVelocity)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<RigidBodyComponent>() && outVelocity)
    {
        *outVelocity = entity.GetComponent<RigidBodyComponent>().Velocity;
    }
}
void RigidBody_SetVelocity(uint64_t entityID, glm::vec3* inVelocity)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<RigidBodyComponent>() && inVelocity)
    {
        entity.GetComponent<RigidBodyComponent>().Velocity = *inVelocity;
    }
}
bool RigidBody_IsGrounded(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<RigidBodyComponent>() ? entity.GetComponent<RigidBodyComponent>().IsGrounded
                                                               : false;
}
bool RigidBody_IsKinematic(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<RigidBodyComponent>() ? entity.GetComponent<RigidBodyComponent>().IsKinematic
                                                               : false;
}
void RigidBody_SetKinematic(uint64_t entityID, bool isKinematic)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<RigidBodyComponent>())
    {
        entity.GetComponent<RigidBodyComponent>().IsKinematic = isKinematic;
    }
}
void AudioComponent_Play(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AudioComponent>())
    {
        auto& audio = entity.GetComponent<AudioComponent>();
        if (audio.SoundHandle != 0)
        {
            auto& audioService = *ServiceLocator::Get<Audio>();

            glm::vec3 worldPos = {0.0f, 0.0f, 0.0f};
            if (entity.HasComponent<TransformComponent>())
            {
                worldPos = glm::vec3(entity.GetComponent<TransformComponent>().WorldTransform[3]);
            }

            audioService.SetInstancePosition(audio.SoundHandle, worldPos);
            audioService.Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
            audio.IsPlaying = true;
        }
    }
}
void AudioComponent_Stop(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AudioComponent>())
    {
        auto& audio = entity.GetComponent<AudioComponent>();
        if (audio.SoundHandle != 0 && audio.IsPlaying)
        {
            auto& audioService = *ServiceLocator::Get<Audio>();
            audioService.Stop(audio.SoundHandle);
            audio.IsPlaying = false;
        }
    }
}
Coral::String TagComponent_GetTag(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    std::string tag = entity && entity.HasComponent<TagComponent>() ? entity.GetComponent<TagComponent>().Tag : "";
    return Coral::String::New(tag);
}
void Shader_SetFloat(uint64_t entityID, const char16_t* inName, float inValue)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<ShaderComponent>())
    {
        auto& shader = entity.GetComponent<ShaderComponent>();
        std::string name = ch_u16_to_string(inName);

        auto it =
            std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(), [&](const auto& u) { return u.Name == name; });

        if (it != shader.Uniforms.end())
        {
            it->Value = inValue; // Automatically updates variant type index to float
        }
        else
        {
            ShaderUniform uniform;
            uniform.Name = name;
            uniform.Value = inValue;
            shader.Uniforms.push_back(uniform);
        }
    }
}
void Shader_SetVec3(uint64_t entityID, const char16_t* inName, glm::vec3* inValue)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<ShaderComponent>() && inValue)
    {
        auto& shader = entity.GetComponent<ShaderComponent>();
        std::string name = ch_u16_to_string(inName);

        auto it =
            std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(), [&](const auto& u) { return u.Name == name; });

        if (it != shader.Uniforms.end())
        {
            it->Value = *inValue; // Automatically updates variant type index to glm::vec3
        }
        else
        {
            ShaderUniform uniform;
            uniform.Name = name;
            uniform.Value = *inValue;
            shader.Uniforms.push_back(uniform);
        }
    }
}
bool Shader_GetEnabled(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<ShaderComponent>() ? entity.GetComponent<ShaderComponent>().Enabled : false;
}
void Shader_SetEnabled(uint64_t entityID, bool enabled)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<ShaderComponent>())
    {
        entity.GetComponent<ShaderComponent>().Enabled = enabled;
    }
}
bool Entity_HasComponent(uint64_t entityID, const char16_t* componentName)
{
    Entity entity = GetEntity(entityID);
    if (!entity)
    {
        return false;
    }
    std::string name = ch_u16_to_string(componentName);
    if (name == "MeshComponent")
    {
        name = "ModelComponent";
    }
    if (name == "PhysicsComponent")
    {
        name = "ColliderComponent";
    }
    if (name == "AnimationComponent")
    {
        return false;
    }

    if (name.find("Control") != std::string::npos || name.find("Group") != std::string::npos)
    {
        if (entity.HasComponent<UIControlComponent>())
        {
            auto& widget = entity.GetComponent<UIControlComponent>();
            if (name == "ButtonControl" && std::holds_alternative<ButtonData>(widget.Data))
            {
                return true;
            }
            if (name == "PanelControl" && std::holds_alternative<PanelData>(widget.Data))
            {
                return true;
            }
            if (name == "LabelControl" && std::holds_alternative<LabelData>(widget.Data))
            {
                return true;
            }
            if (name == "ImageControl" && std::holds_alternative<ImageData>(widget.Data))
            {
                return true;
            }
            if (name == "CheckboxControl" && std::holds_alternative<CheckboxData>(widget.Data))
            {
                return true;
            }
            if (name == "ComboBoxControl" && std::holds_alternative<ComboBoxData>(widget.Data))
            {
                return true;
            }
            if (name == "SliderControl" && std::holds_alternative<SliderData>(widget.Data))
            {
                return true;
            }
            if (name == "ProgressBarControl" && std::holds_alternative<ProgressBarData>(widget.Data))
            {
                return true;
            }
            if (name == "InputTextControl" && std::holds_alternative<InputTextData>(widget.Data))
            {
                return true;
            }
            if (name == "ImageButtonControl" && std::holds_alternative<ImageButtonData>(widget.Data))
            {
                return true;
            }
            if (name == "SeparatorControl" && std::holds_alternative<SeparatorData>(widget.Data))
            {
                return true;
            }
            if (name == "RadioButtonControl" && std::holds_alternative<RadioButtonData>(widget.Data))
            {
                return true;
            }
            if (name == "ColorPickerControl" && std::holds_alternative<ColorPickerData>(widget.Data))
            {
                return true;
            }
            if (name == "DragFloatControl" && std::holds_alternative<DragFloatData>(widget.Data))
            {
                return true;
            }
            if (name == "DragIntControl" && std::holds_alternative<DragIntData>(widget.Data))
            {
                return true;
            }
            if (name == "VerticalLayoutGroup" && std::holds_alternative<VerticalLayoutGroupData>(widget.Data))
            {
                return true;
            }
        }
        return false;
    }

    for (const auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Name == name || metadata.SerializationKey == name)
        {
            if (metadata.Has)
            {
                return metadata.Has(entity);
            }
        }
    }
    return false;
}

void RegisterGlueEntity()
{
    // === Entity Core ===
    CH_ADD_INTERNAL_CALL("Entity", Entity_AddComponent, Entity_AddComponent);
    CH_ADD_INTERNAL_CALL("Entity", Entity_HasComponent, Entity_HasComponent);
    CH_ADD_INTERNAL_CALL("Entity", Entity_FindAllWithComponent, Entity_FindAllWithComponent);
    CH_ADD_INTERNAL_CALL("Entity", TagComponent_GetTag, TagComponent_GetTag);

    // === Transform Component ===
    CH_ADD_INTERNAL_CALL("Transform", Transform_GetTranslation, Transform_GetTranslation);
    CH_ADD_INTERNAL_CALL("Transform", Transform_SetTranslation, Transform_SetTranslation);
    CH_ADD_INTERNAL_CALL("Transform", Transform_GetRotation, Transform_GetRotation);
    CH_ADD_INTERNAL_CALL("Transform", Transform_SetRotation, Transform_SetRotation);
    CH_ADD_INTERNAL_CALL("Transform", Transform_GetScale, Transform_GetScale);
    CH_ADD_INTERNAL_CALL("Transform", Transform_SetScale, Transform_SetScale);

    // === Model Component ===
    CH_ADD_INTERNAL_CALL("Model", Model_GetModelPath, Model_GetModelPath);
    CH_ADD_INTERNAL_CALL("Model", Model_SetModelPath, Model_SetModelPath);

    // === RigidBody Component ===
    CH_ADD_INTERNAL_CALL("RigidBody", RigidBody_GetVelocity, RigidBody_GetVelocity);
    CH_ADD_INTERNAL_CALL("RigidBody", RigidBody_SetVelocity, RigidBody_SetVelocity);
    CH_ADD_INTERNAL_CALL("RigidBody", RigidBody_IsGrounded, RigidBody_IsGrounded);
    CH_ADD_INTERNAL_CALL("RigidBody", RigidBody_IsKinematic, RigidBody_IsKinematic);
    CH_ADD_INTERNAL_CALL("RigidBody", RigidBody_SetKinematic, RigidBody_SetKinematic);

    // === Audio Component ===
    CH_ADD_INTERNAL_CALL("AudioComponent", AudioComponent_Play, AudioComponent_Play);
    CH_ADD_INTERNAL_CALL("AudioComponent", AudioComponent_Stop, AudioComponent_Stop);

    // === Shader Component ===
    CH_ADD_INTERNAL_CALL("Shader", Shader_SetFloat, Shader_SetFloat);
    CH_ADD_INTERNAL_CALL("Shader", Shader_SetVec3, Shader_SetVec3);
    CH_ADD_INTERNAL_CALL("Shader", Shader_GetEnabled, Shader_GetEnabled);
    CH_ADD_INTERNAL_CALL("Shader", Shader_SetEnabled, Shader_SetEnabled);
}
}
// namespace Chained