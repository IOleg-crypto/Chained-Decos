#include "script_glue_entity.h"
#include "engine/audio/audio.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/components.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/entity.h"

namespace Chained
{
void Transform_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && outTranslation)
    {
        *outTranslation = entity.GetComponent<TransformComponent>().Translation;
    }
    else if (outTranslation)
    {
        *outTranslation = {};
    }
}
void Transform_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && inTranslation)
    {
        auto& tc = entity.GetComponent<TransformComponent>();
        tc.Translation = *inTranslation;
        tc.PrevTranslation = *inTranslation;
        tc.TransformChanged = true;

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (rb.Handle != kInvalidPhysicsBody)
            {
                auto* physics = ServiceLocator::TryGet<Physics>();
                if (physics && physics->GetWorld())
                {
                    physics->GetWorld()->SetTransform(rb.Handle, *inTranslation, tc.RotationQuat);
                }
            }
        }
    }
}
void Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<TransformComponent>() && outRotation)
    {
        *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }
    else if (outRotation)
    {
        *outRotation = {};
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
    else if (outScale)
    {
        *outScale = {};
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
static thread_local Coral::UCString s_ModelTagBuffer;

const Coral::UCChar* Model_GetModelPath(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    std::string path =
        entity && entity.HasComponent<ModelComponent>() ? entity.GetComponent<ModelComponent>().ModelPath : "";
    s_ModelTagBuffer = ToWide(path);
    return s_ModelTagBuffer.c_str();
}
void Model_SetModelPath(uint64_t entityID, const Coral::UCChar* inPath)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<ModelComponent>())
    {
        entity.GetComponent<ModelComponent>().ModelPath = ch_u16_to_string(inPath);
    }
}
void Entity_AddComponent(uint64_t entityID, const Coral::UCChar* componentName)
{
    Entity entity = GetEntity(entityID);
    if (!entity)
    {
        return;
    }
    std::string name = ch_u16_to_string(componentName);
    ResolveComponentName(name);
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
int Entity_FindAllWithComponent(const Coral::UCChar* componentName, uint64_t* outBuf, int bufSize)
{
    Scene* scene = GetActiveScene();
    if (!scene)
    {
        return 0;
    }
    std::string name = ch_u16_to_string(componentName);
    ResolveComponentName(name);
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
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        if (rb.Handle != kInvalidPhysicsBody)
        {
            auto* physics = ServiceLocator::TryGet<Physics>();
            if (physics && physics->GetWorld())
            {
                *outVelocity = physics->GetWorld()->GetVelocity(rb.Handle);
                return;
            }
        }
        *outVelocity = rb.Velocity;
    }
    else if (outVelocity)
    {
        *outVelocity = {};
    }
}
void RigidBody_SetVelocity(uint64_t entityID, glm::vec3* inVelocity)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<RigidBodyComponent>() && inVelocity)
    {
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        rb.Velocity = *inVelocity;
        if (rb.Handle != kInvalidPhysicsBody)
        {
            auto* physics = ServiceLocator::TryGet<Physics>();
            if (physics && physics->GetWorld())
            {
                glm::vec3 toSet = *inVelocity;
                if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
                {
                    constexpr float kJumpImpulseThreshold = 0.5f;
                    if (toSet.y <= kJumpImpulseThreshold)
                    {
                        toSet.y = physics->GetWorld()->GetVelocity(rb.Handle).y;
                    }
                }
                physics->GetWorld()->SetVelocity(rb.Handle, toSet);
            }
        }
    }
}
void RigidBody_ForceSetVelocity(uint64_t entityID, glm::vec3* inVelocity)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<RigidBodyComponent>() && inVelocity)
    {
        auto& rb = entity.GetComponent<RigidBodyComponent>();
        rb.Velocity = *inVelocity;
        rb.VelocityForced = true;
        if (rb.Handle != kInvalidPhysicsBody)
        {
            auto* physics = ServiceLocator::TryGet<Physics>();
            if (physics)
            {
                physics->ForceSetVelocity(rb.Handle, *inVelocity);
            }
        }
    }
}
bool RigidBody_IsGrounded(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (!entity || !entity.HasComponent<RigidBodyComponent>())
    {
        return false;
    }

    auto& rb = entity.GetComponent<RigidBodyComponent>();
    return rb.IsGrounded;
}
uint32_t RigidBody_IsKinematic(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return (entity && entity.HasComponent<RigidBodyComponent>() &&
            entity.GetComponent<RigidBodyComponent>().Type == RigidBodyComponent::BodyType::Kinematic)
               ? 1u
               : 0u;
}
void RigidBody_SetKinematic(uint64_t entityID, bool isKinematic)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<RigidBodyComponent>())
    {
        entity.GetComponent<RigidBodyComponent>().Type =
            isKinematic ? RigidBodyComponent::BodyType::Kinematic : RigidBodyComponent::BodyType::Dynamic;
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
            auto* audioService = ServiceLocator::TryGet<Audio>();
            if (!audioService)
            {
                return;
            }

            glm::vec3 worldPos = {0.0f, 0.0f, 0.0f};
            if (entity.HasComponent<TransformComponent>())
            {
                worldPos = glm::vec3(entity.GetComponent<TransformComponent>().WorldTransform[3]);
            }

            audioService->SetInstancePosition(audio.SoundHandle, worldPos);
            audioService->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
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
            auto* audioService = ServiceLocator::TryGet<Audio>();
            if (!audioService)
            {
                return;
            }
            audioService->Stop(audio.SoundHandle);
            audio.IsPlaying = false;
        }
    }
}
const Coral::UCChar* TagComponent_GetTag(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    std::string tag = entity && entity.HasComponent<TagComponent>() ? entity.GetComponent<TagComponent>().Tag : "";
    s_ModelTagBuffer = ToWide(tag);
    return s_ModelTagBuffer.c_str();
}
void Shader_SetFloat(uint64_t entityID, const Coral::UCChar* inName, float inValue)
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
void Shader_SetVec3(uint64_t entityID, const Coral::UCChar* inName, glm::vec3* inValue)
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
bool Entity_HasComponent(uint64_t entityID, const Coral::UCChar* componentName)
{
    Entity entity = GetEntity(entityID);
    if (!entity)
    {
        return false;
    }
    std::string name = ch_u16_to_string(componentName);
    ResolveComponentName(name);

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

// ── PlayerComponent ───────────────────────────────────────────────────
float PlayerComponent_GetMovementSpeed(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<Chained::PlayerComponent>()
               ? entity.GetComponent<Chained::PlayerComponent>().MovementSpeed
               : 0.0f;
}
void PlayerComponent_SetMovementSpeed(uint64_t entityID, float value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::PlayerComponent>())
    {
        entity.GetComponent<Chained::PlayerComponent>().MovementSpeed = value;
    }
}
float PlayerComponent_GetJumpForce(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<Chained::PlayerComponent>()
               ? entity.GetComponent<Chained::PlayerComponent>().JumpForce
               : 0.0f;
}
void PlayerComponent_SetJumpForce(uint64_t entityID, float value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::PlayerComponent>())
    {
        entity.GetComponent<Chained::PlayerComponent>().JumpForce = value;
    }
}
float PlayerComponent_GetLookSensitivity(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<Chained::PlayerComponent>()
               ? entity.GetComponent<Chained::PlayerComponent>().LookSensitivity
               : 0.0f;
}
void PlayerComponent_SetLookSensitivity(uint64_t entityID, float value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::PlayerComponent>())
    {
        entity.GetComponent<Chained::PlayerComponent>().LookSensitivity = value;
    }
}

bool SpawnComponent_GetIsActive(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<Chained::SpawnComponent>()
               ? entity.GetComponent<Chained::SpawnComponent>().IsActive
               : false;
}
void SpawnComponent_SetIsActive(uint64_t entityID, bool value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::SpawnComponent>())
    {
        entity.GetComponent<Chained::SpawnComponent>().IsActive = value;
    }
}
bool SpawnComponent_IsCheckpoint(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<Chained::SpawnComponent>()
               ? entity.GetComponent<Chained::SpawnComponent>().IsCheckpoint
               : false;
}
void SpawnComponent_SetIsCheckpoint(uint64_t entityID, bool value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::SpawnComponent>())
    {
        entity.GetComponent<Chained::SpawnComponent>().IsCheckpoint = value;
    }
}
void SpawnComponent_GetSpawnPoint(uint64_t entityID, glm::vec3* outPoint)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::SpawnComponent>() && outPoint)
    {
        *outPoint = entity.GetComponent<Chained::SpawnComponent>().SpawnPoint;
    }
    else if (outPoint)
    {
        *outPoint = {};
    }
}
void SpawnComponent_SetSpawnPoint(uint64_t entityID, glm::vec3* inPoint)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::SpawnComponent>() && inPoint)
    {
        entity.GetComponent<Chained::SpawnComponent>().SpawnPoint = *inPoint;
    }
}
bool SpawnComponent_GetRenderSpawnZoneInScene(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<Chained::SpawnComponent>()
               ? entity.GetComponent<Chained::SpawnComponent>().RenderSpawnZoneInScene
               : false;
}
void SpawnComponent_GetZoneSize(uint64_t entityID, glm::vec3* outSize)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<Chained::SpawnComponent>() && outSize)
    {
        *outSize = entity.GetComponent<Chained::SpawnComponent>().ZoneSize;
    }
    else if (outSize)
    {
        *outSize = {};
    }
}

// ── AnimationComponent ──────────────────────────────────────────────────
int AnimationComponent_GetCurrentAnimationIndex(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>()
               ? entity.GetComponent<AnimationComponent>().CurrentAnimationIndex
               : -1;
}
void AnimationComponent_SetCurrentAnimationIndex(uint64_t entityID, int index)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        auto& anim = entity.GetComponent<AnimationComponent>();
        if (anim.CurrentAnimationIndex != index)
        {
            anim.CurrentAnimationIndex = index;
            anim.CurrentFrame = 0;
            anim.FrameTimeCounter = 0.0f;
            anim.IsFinished = false;
        }
    }
}
uint32_t AnimationComponent_GetIsPlaying(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>()
               ? (entity.GetComponent<AnimationComponent>().IsPlaying ? 1 : 0)
               : 0;
}
void AnimationComponent_SetIsPlaying(uint64_t entityID, uint32_t isPlaying)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        entity.GetComponent<AnimationComponent>().IsPlaying = isPlaying != 0;
    }
}
bool AnimationComponent_GetIsLooping(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>() ? entity.GetComponent<AnimationComponent>().IsLooping
                                                               : false;
}
void AnimationComponent_SetIsLooping(uint64_t entityID, bool isLooping)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        entity.GetComponent<AnimationComponent>().IsLooping = isLooping;
    }
}
bool AnimationComponent_GetIsFinished(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>() ? entity.GetComponent<AnimationComponent>().IsFinished
                                                               : false;
}
float AnimationComponent_GetDuration(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>() ? entity.GetComponent<AnimationComponent>().Duration
                                                               : 0.0f;
}
float AnimationComponent_GetNormalizedTime(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>()
               ? entity.GetComponent<AnimationComponent>().NormalizedTime
               : 0.0f;
}
float AnimationComponent_GetBlendDuration(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<AnimationComponent>() ? entity.GetComponent<AnimationComponent>().BlendDuration
                                                               : 0.0f;
}
void AnimationComponent_SetBlendDuration(uint64_t entityID, float blendDuration)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        entity.GetComponent<AnimationComponent>().BlendDuration = blendDuration;
    }
}
void AnimationComponent_CrossFade(uint64_t entityID, int targetIndex, float blendDuration)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        auto& anim = entity.GetComponent<AnimationComponent>();
        if (anim.CurrentAnimationIndex == targetIndex && !anim.Blending)
        {
            return;
        }
        anim.TargetAnimationIndex = targetIndex;
        anim.TargetFrame = 0;
        anim.BlendDuration = blendDuration;
        anim.BlendTimer = 0.0f;
        anim.Blending = true;
        anim.IsPlaying = true;
        anim.IsFinished = false;
    }
}
// ── AnimationComponent graph variables ──────────────────────────────
void AnimationComponent_SetFloat(uint64_t entityID, const Coral::UCChar* name, float value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        std::string strName = ch_u16_to_string(name);
        entity.GetComponent<AnimationComponent>().SetFloat(strName, value);
    }
}

void AnimationComponent_SetBool(uint64_t entityID, const Coral::UCChar* name, bool value)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        std::string strName = ch_u16_to_string(name);
        entity.GetComponent<AnimationComponent>().SetBool(strName, value);
    }
}

float AnimationComponent_GetFloat(uint64_t entityID, const Coral::UCChar* name)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<AnimationComponent>())
    {
        std::string strName = ch_u16_to_string(name);
        return entity.GetComponent<AnimationComponent>().GetFloat(strName);
    }
    return 0.0f;
}

} // namespace Chained