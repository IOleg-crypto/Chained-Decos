#ifndef CH_SCRIPT_GLUE_INTERNAL_H
#define CH_SCRIPT_GLUE_INTERNAL_H

#include <Coral/String.hpp>
#include <Coral/Array.hpp>
#include <imgui.h>
#include "engine/core/log.h"
#include "engine/core/input.h"
#include "engine/core/application.h"
#include "engine/scene/scene.h"
#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "scriptengine.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/scene/project.h"
#include "engine/core/assets/asset_manager.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "script_glue.h"

// Macro to mark functions for InternalCall (formerly P/Invoke)
#define CH_SCRIPT_FUNC static

namespace CHEngine {
    
    // Internal helper to get active scene safely
    inline Scene* GetActiveScene() { return ScriptEngine::Get().GetActiveScene(); }

    // Internal helper to get entity safely
    inline Entity GetEntity(uint64_t entityID) {
        Scene* scene = GetActiveScene();
        if (!scene) return {};
        return Entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
    }

    // Module registration functions (to be called by ScriptGlue::RegisterInternalCalls)
    void RegisterSystemInternalCalls(Coral::ManagedAssembly& assembly);
    void RegisterInputInternalCalls(Coral::ManagedAssembly& assembly);
    void RegisterSceneInternalCalls(Coral::ManagedAssembly& assembly);
    void RegisterEntityInternalCalls(Coral::ManagedAssembly& assembly);
    void RegisterCameraInternalCalls(Coral::ManagedAssembly& assembly);
    void RegisterUIInternalCalls(Coral::ManagedAssembly& assembly);
    void RegisterGameplayInternalCalls(Coral::ManagedAssembly& assembly);

} // namespace CHEngine

#endif // CH_SCRIPT_GLUE_INTERNAL_H
