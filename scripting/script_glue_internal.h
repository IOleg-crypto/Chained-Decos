#ifndef CH_SCRIPT_GLUE_INTERNAL_H
#define CH_SCRIPT_GLUE_INTERNAL_H

#include "engine/core/assets/asset_manager.h"
#include "engine/audio/audio.h"
#include "engine/audio/audio.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "script_glue.h"
#include "scriptengine.h"
#include <Coral/Array.hpp>
#include <Coral/String.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

// Macro to mark functions for InternalCall (formerly P/Invoke)
#define CH_SCRIPT_FUNC static

namespace CHEngine
{

// Internal helper to get active scene safely
inline Scene* GetActiveScene()
{
    return ScriptEngine::Get().GetActiveScene();
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

// Module registration functions (to be called by ScriptGlue::RegisterInternalCalls)
void RegisterSystemGlue(Coral::ManagedAssembly& assembly);
void RegisterInputGlue(Coral::ManagedAssembly& assembly);
void RegisterSceneGlue(Coral::ManagedAssembly& assembly);
void RegisterEntityGlue(Coral::ManagedAssembly& assembly);
void RegisterCameraGlue(Coral::ManagedAssembly& assembly);
void RegisterUIGlue(Coral::ManagedAssembly& assembly);
void RegisterGameplayGlue(Coral::ManagedAssembly& assembly);

} // namespace CHEngine

#endif // CH_SCRIPT_GLUE_INTERNAL_H
