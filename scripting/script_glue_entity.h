#ifndef SCRIPT_GLUE_ENTITY_H
#define SCRIPT_GLUE_ENTITY_H
#include "script_glue_internal.h"
#include <algorithm>
#include <string>

namespace Chained
{

// ── Entity / Transform ────────────────────────────────────────────────
CH_SCRIPT_FUNC void Transform_GetTranslation(uint64_t entityID, glm::vec3* outTranslation);

CH_SCRIPT_FUNC void Transform_SetTranslation(uint64_t entityID, glm::vec3* inTranslation);

CH_SCRIPT_FUNC void Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation);

CH_SCRIPT_FUNC void Transform_SetRotation(uint64_t entityID, glm::vec3* inRotation);

CH_SCRIPT_FUNC void Transform_GetScale(uint64_t entityID, glm::vec3* outScale);

CH_SCRIPT_FUNC void Transform_SetScale(uint64_t entityID, glm::vec3* inScale);

// ── Model Component ───────────────────────────────────────────────────
CH_SCRIPT_FUNC const Coral::UCChar* Model_GetModelPath(uint64_t entityID);

CH_SCRIPT_FUNC void Model_SetModelPath(uint64_t entityID, const Coral::UCChar* inPath);

// ── Registry Management ───────────────────────────────────────────────
CH_SCRIPT_FUNC void Entity_AddComponent(uint64_t entityID, const Coral::UCChar* componentName);

CH_SCRIPT_FUNC bool Entity_HasComponent(uint64_t entityID, const Coral::UCChar* componentName);

CH_SCRIPT_FUNC int Entity_FindAllWithComponent(const Coral::UCChar* componentName, uint64_t* outBuf, int bufSize);

// ── Physics (RigidBody) ───────────────────────────────────────────────
CH_SCRIPT_FUNC void RigidBody_GetVelocity(uint64_t entityID, glm::vec3* outVelocity);

CH_SCRIPT_FUNC void RigidBody_SetVelocity(uint64_t entityID, glm::vec3* inVelocity);

CH_SCRIPT_FUNC bool RigidBody_IsGrounded(uint64_t entityID);

CH_SCRIPT_FUNC bool RigidBody_IsKinematic(uint64_t entityID);

CH_SCRIPT_FUNC void RigidBody_SetKinematic(uint64_t entityID, bool isKinematic);

// ── Audio Component ───────────────────────────────────────────────────
CH_SCRIPT_FUNC void AudioComponent_Play(uint64_t entityID);

CH_SCRIPT_FUNC void AudioComponent_Stop(uint64_t entityID);

// ── Tag Component ─────────────────────────────────────────────────────
CH_SCRIPT_FUNC const Coral::UCChar* TagComponent_GetTag(uint64_t entityID);

// ── Shader Component ──────────────────────────────────────────────────
CH_SCRIPT_FUNC void Shader_SetFloat(uint64_t entityID, const Coral::UCChar* inName, float inValue);

CH_SCRIPT_FUNC void Shader_SetVec3(uint64_t entityID, const Coral::UCChar* inName, glm::vec3* inValue);

CH_SCRIPT_FUNC bool Shader_GetEnabled(uint64_t entityID);

CH_SCRIPT_FUNC void Shader_SetEnabled(uint64_t entityID, bool enabled);

// ── PlayerComponent ───────────────────────────────────────────────────
CH_SCRIPT_FUNC float PlayerComponent_GetMovementSpeed(uint64_t entityID);

CH_SCRIPT_FUNC void PlayerComponent_SetMovementSpeed(uint64_t entityID, float value);

CH_SCRIPT_FUNC float PlayerComponent_GetJumpForce(uint64_t entityID);

CH_SCRIPT_FUNC void PlayerComponent_SetJumpForce(uint64_t entityID, float value);

CH_SCRIPT_FUNC float PlayerComponent_GetLookSensitivity(uint64_t entityID);

CH_SCRIPT_FUNC void PlayerComponent_SetLookSensitivity(uint64_t entityID, float value);

// ── SpawnComponent ────────────────────────────────────────────────────
CH_SCRIPT_FUNC bool SpawnComponent_IsActive(uint64_t entityID);

CH_SCRIPT_FUNC void SpawnComponent_GetSpawnPoint(uint64_t entityID, glm::vec3* outPoint);
CH_SCRIPT_FUNC bool SpawnComponent_GetRenderSpawnZoneInScene(uint64_t entityID);
CH_SCRIPT_FUNC void SpawnComponent_GetZoneSize(uint64_t entityID, glm::vec3* outSize);

// ── AnimationComponent ──────────────────────────────────────────────────
CH_SCRIPT_FUNC int AnimationComponent_GetCurrentAnimationIndex(uint64_t entityID);
CH_SCRIPT_FUNC void AnimationComponent_SetCurrentAnimationIndex(uint64_t entityID, int index);
CH_SCRIPT_FUNC uint32_t AnimationComponent_GetIsPlaying(uint64_t entityID);
CH_SCRIPT_FUNC void AnimationComponent_SetIsPlaying(uint64_t entityID, uint32_t isPlaying);

} // namespace Chained
#endif