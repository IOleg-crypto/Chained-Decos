#ifndef CH_SCRIPT_GLUE_INTERNAL_H
#define CH_SCRIPT_GLUE_INTERNAL_H

#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include "scriptengine.h"
#include "scriptengine_services.h"
#include <Coral/Array.hpp>
#include <Coral/String.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

// Macro to mark functions for InternalCall (formerly P/Invoke)
#define CH_SCRIPT_FUNC static

namespace Chained
{

// Forward declarations for explicit glue registration (prevents linker stripping)
void RegisterGlueSystem();
void RegisterGlueInput();
void RegisterGlueNetwork();
void RegisterGlueScene();
void RegisterGlueEntity();
void RegisterGlueCamera();
void RegisterGlueUI();
void RegisterGlueAudio();

// Internal helper to get active scene safely
inline Scene* GetActiveScene()
{
    return GetContextScene();
}

// Internal helper to get entity safely
inline Entity GetEntity(uint64_t entityID)
{
    Scene* scene = GetActiveScene();
    if (!scene)
    {
        return {};
    }
    return Entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
}

} // namespace Chained

#endif // CH_SCRIPT_GLUE_INTERNAL_H
