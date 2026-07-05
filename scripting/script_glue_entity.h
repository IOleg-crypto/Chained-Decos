#ifndef SCRIPT_GLUE_ENTITY_H
#define SCRIPT_GLUE_ENTITY_H
#include "script_glue_internal.h"
#include <algorithm>
#include <string>

namespace Chained
{

void RegisterGlueEntity();

// Helper: convert UTF-16 C# string to std::string
static std::string ch_u16_to_string(const char16_t* ptr)
{
    if (!ptr)
    {
        return {};
    }
    std::u16string u16(ptr);
    std::string result;
    result.reserve(u16.size());
    for (char16_t c : u16)
    {
        result += (c < 0x80) ? (char)c : '?';
    }
    return result;
}

// Helper: copy std::string to UTF-16 buffer
static void ch_string_to_u16(const std::string& src, char16_t* buf, int bufSize)
{
    if (!buf || bufSize <= 0)
    {
        return;
    }
    int n = std::min((int)src.size(), bufSize - 1);
    for (int i = 0; i < n; ++i)
    {
        buf[i] = (char16_t)(unsigned char)src[i];
    }
    buf[n] = 0;
}

// ── Entity / Transform ────────────────────────────────────────────────
CH_SCRIPT_FUNC void Transform_GetTranslation(uint64_t entityID, glm::vec3* outTranslation);

CH_SCRIPT_FUNC void Transform_SetTranslation(uint64_t entityID, glm::vec3* inTranslation);

CH_SCRIPT_FUNC void Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation);

CH_SCRIPT_FUNC void Transform_SetRotation(uint64_t entityID, glm::vec3* inRotation);

CH_SCRIPT_FUNC void Transform_GetScale(uint64_t entityID, glm::vec3* outScale);

CH_SCRIPT_FUNC void Transform_SetScale(uint64_t entityID, glm::vec3* inScale);

// ── Model Component ───────────────────────────────────────────────────
CH_SCRIPT_FUNC Coral::String Model_GetModelPath(uint64_t entityID);

CH_SCRIPT_FUNC void Model_SetModelPath(uint64_t entityID, const char16_t* inPath);

// ── Registry Management ───────────────────────────────────────────────
CH_SCRIPT_FUNC void Entity_AddComponent(uint64_t entityID, const char16_t* componentName);

CH_SCRIPT_FUNC bool Entity_HasComponent(uint64_t entityID, const char16_t* componentName);

CH_SCRIPT_FUNC int Entity_FindAllWithComponent(const char16_t* componentName, uint64_t* outBuf, int bufSize);

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
CH_SCRIPT_FUNC Coral::String TagComponent_GetTag(uint64_t entityID);

// ── Shader Component ──────────────────────────────────────────────────
CH_SCRIPT_FUNC void Shader_SetFloat(uint64_t entityID, const char16_t* inName, float inValue);

CH_SCRIPT_FUNC void Shader_SetVec3(uint64_t entityID, const char16_t* inName, glm::vec3* inValue);

CH_SCRIPT_FUNC bool Shader_GetEnabled(uint64_t entityID);

CH_SCRIPT_FUNC void Shader_SetEnabled(uint64_t entityID, bool enabled);

} // namespace Chained
#endif