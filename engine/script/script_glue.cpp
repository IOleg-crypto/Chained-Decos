#line 1 "d:/gitnext/Chained Decos/engine/script/script_glue.cpp"
#include <Coral/String.hpp>
#include "engine/core/log.h"
#include "engine/core/input.h"
#include "engine/core/application.h"
#include "engine/scene/scene.h"
#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "engine/script/scriptengine.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/scene/project.h"
#include "raylib.h"
#include "raymath.h"
#include "engine/script/script_glue.h"

// Macro to mark functions for InternalCall (formerly P/Invoke)
#define CH_SCRIPT_FUNC static

namespace CHEngine {

    // ── Logging ──────────────────────────────────────────────────────────
    // ── Logging ──────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Log_Info(Coral::String message) { CH_CORE_INFO("[C#] {}", (std::string)message); }
    CH_SCRIPT_FUNC void Log_Warn(Coral::String message) { CH_CORE_WARN("[C#] {}", (std::string)message); }
    CH_SCRIPT_FUNC void Log_Error(Coral::String message) { CH_CORE_ERROR("[C#] {}", (std::string)message); }

    // ── Input ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool Input_IsKeyDown(int keyCode) { return Input::IsKeyDown(keyCode); }
    CH_SCRIPT_FUNC bool Input_IsKeyPressed(int keyCode) { return Input::IsKeyPressed(keyCode); }
    CH_SCRIPT_FUNC bool Input_IsKeyReleased(int keyCode) { return Input::IsKeyReleased(keyCode); }
    CH_SCRIPT_FUNC bool Input_IsMouseButtonDown(int button) { return Input::IsMouseButtonDown(button); }
    CH_SCRIPT_FUNC bool Input_IsMouseButtonPressed(int button) { return Input::IsMouseButtonPressed(button); }
    
    CH_SCRIPT_FUNC void Input_GetMouseDelta(Vector3* outDelta) { 
        Vector2 delta = Input::GetMouseDelta();
        *outDelta = {delta.x, delta.y, 0.0f};
    }
    CH_SCRIPT_FUNC float Input_GetMouseWheelMove() { return Input::GetMouseWheelMove(); }

    // ── Scene / Global ────────────────────────────────────────────────────
    CH_SCRIPT_FUNC uint64_t Scene_FindEntityByTag(Coral::String tag) {
        if (!ScriptEngine::GetActiveScene()) return 0;
        std::string tagStr = (std::string)tag;
        CH_CORE_INFO("C# searching for entity by tag: '{}'", tagStr);
        Entity entity = ScriptEngine::GetActiveScene()->FindEntityByTag(tagStr);
        return entity ? (uint64_t)(uint32_t)entity : 0;
    }

    CH_SCRIPT_FUNC void Scene_LoadScene(Coral::String path) {
        // This should probably be deferred or handled by a high-level SceneManager
        // For now, it's a stub as in the original code
    }

    CH_SCRIPT_FUNC uint64_t Scene_GetPrimaryCameraEntity() {
        if (!ScriptEngine::GetActiveScene()) return 0;
        Entity entity = ScriptEngine::GetActiveScene()->GetPrimaryCameraEntity();
        return entity ? (uint64_t)(uint32_t)entity : 0;
    }

    // ── Entity / Transform ────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Entity_GetTranslation(uint64_t entityID, Vector3* outTranslation) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outTranslation = entity.GetComponent<TransformComponent>().Translation;
    }

    CH_SCRIPT_FUNC void Entity_SetTranslation(uint64_t entityID, Vector3* inTranslation) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().Translation = *inTranslation;
    }

    CH_SCRIPT_FUNC void Entity_GetRotation(uint64_t entityID, Vector3* outRotation) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }

    CH_SCRIPT_FUNC void Entity_SetRotation(uint64_t entityID, Vector3* inRotation) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().SetRotation(*inRotation);
    }

    CH_SCRIPT_FUNC void Entity_GetScale(uint64_t entityID, Vector3* outScale) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outScale = entity.GetComponent<TransformComponent>().Scale;
    }

    CH_SCRIPT_FUNC void Entity_SetScale(uint64_t entityID, Vector3* inScale) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().Scale = *inScale;
    }

    CH_SCRIPT_FUNC bool Entity_HasComponent(uint64_t entityID, Coral::String componentName) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (!entity) return false;
        
        std::string name = (std::string)componentName;
        CH_CORE_INFO("C# queried HasComponent: '{}'", name);
        if (name == "TransformComponent") return entity.HasComponent<TransformComponent>();
        if (name == "RigidBodyComponent") return entity.HasComponent<RigidBodyComponent>();
        if (name == "CameraComponent")    return entity.HasComponent<CameraComponent>();
        if (name == "PlayerComponent")    return entity.HasComponent<PlayerComponent>();
        if (name == "AudioComponent")     return entity.HasComponent<AudioComponent>();
        if (name == "TagComponent")       return entity.HasComponent<TagComponent>();
        return false;
    }

    CH_SCRIPT_FUNC void Entity_GetVelocity(uint64_t entityID, Vector3* outVelocity) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<RigidBodyComponent>()) 
            *outVelocity = entity.GetComponent<RigidBodyComponent>().Velocity;
    }

    CH_SCRIPT_FUNC void Entity_SetVelocity(uint64_t entityID, Vector3* inVelocity) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<RigidBodyComponent>()) 
            entity.GetComponent<RigidBodyComponent>().Velocity = *inVelocity;
    }

    CH_SCRIPT_FUNC bool Entity_IsGrounded(uint64_t entityID) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<RigidBodyComponent>() ? entity.GetComponent<RigidBodyComponent>().IsGrounded : false;
    }

    // ── Tag ───────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC Coral::String TagComponent_GetTag(uint64_t entityID) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return Coral::String::New("");
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TagComponent>()) 
            return Coral::String::New(entity.GetComponent<TagComponent>().Tag); 
        return Coral::String::New("");
    }

    // ── Camera ────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Camera_GetForward(uint64_t entityID, Vector3* outForward) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            *outForward = Vector3RotateByQuaternion({0.0f, 0.0f, -1.0f}, tc.RotationQuat);
        }
    }

    CH_SCRIPT_FUNC void Camera_GetRight(uint64_t entityID, Vector3* outRight) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            *outRight = Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, tc.RotationQuat);
        }
    }

    CH_SCRIPT_FUNC void Camera_GetOrbit(uint64_t entityID, float* yaw, float* pitch, float* distance) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<CameraComponent>()) {
            auto& camera = entity.GetComponent<CameraComponent>();
            *yaw = camera.OrbitYaw;
            *pitch = camera.OrbitPitch;
            *distance = camera.OrbitDistance;
        }
    }

    CH_SCRIPT_FUNC void Camera_SetOrbit(uint64_t entityID, float yaw, float pitch, float distance) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<CameraComponent>()) {
            auto& camera = entity.GetComponent<CameraComponent>();
            camera.OrbitYaw = yaw;
            camera.OrbitPitch = pitch;
            camera.OrbitDistance = distance;
        }
    }

    // ── UI Controls ───────────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool ButtonControl_IsPressed(uint64_t entityID) { return false; } 
    CH_SCRIPT_FUNC bool CheckboxControl_GetChecked(uint64_t entityID) { return false; }
    CH_SCRIPT_FUNC int ComboBoxControl_GetSelectedIndex(uint64_t entityID) { return 0; }
    CH_SCRIPT_FUNC Coral::String ComboBoxControl_GetItem(uint64_t entityID, int index) { return Coral::String::New(""); }

    // ── Spawn / Transition ────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool SpawnComponent_IsActive(uint64_t entityID) { return false; }
    CH_SCRIPT_FUNC void SpawnComponent_GetSpawnPoint(uint64_t entityID, Vector3* point) { *point = {0,0,0}; }
    CH_SCRIPT_FUNC Coral::String SceneTransitionComponent_GetTargetScene(uint64_t entityID) { return Coral::String::New(""); }

    // ── PlayerComponent ───────────────────────────────────────────────────
    CH_SCRIPT_FUNC float PlayerComponent_GetMovementSpeed(uint64_t entityID) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return 0.0f;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().MovementSpeed : 0.0f;
    }

    CH_SCRIPT_FUNC void PlayerComponent_SetMovementSpeed(uint64_t entityID, float speed) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<PlayerComponent>()) 
            entity.GetComponent<PlayerComponent>().MovementSpeed = speed;
    }

    // ── Audio ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Audio_Play(Coral::String path, float volume, float pitch, bool loop) {
        if (Project::GetActive() && Project::GetActive()->GetAssetManager()) {
            auto asset = Project::GetActive()->GetAssetManager()->Get<SoundAsset>((std::string)path);
            if (asset) Audio::Play(asset, volume, pitch, loop);
        }
    }

    CH_SCRIPT_FUNC void Audio_Stop(Coral::String path) {
        if (Project::GetActive() && Project::GetActive()->GetAssetManager()) {
            auto asset = Project::GetActive()->GetAssetManager()->Get<SoundAsset>((std::string)path);
            if (asset) Audio::Stop(asset);
        }
    }

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Volume = volume;
    }

    CH_SCRIPT_FUNC void AudioComponent_SetLoop(uint64_t entityID, bool loop) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Loop = loop;
    }

    CH_SCRIPT_FUNC bool AudioComponent_IsPlaying(uint64_t entityID) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<AudioComponent>() ? entity.GetComponent<AudioComponent>().IsPlaying : false;
    }

    CH_SCRIPT_FUNC Coral::String AudioComponent_GetSoundPath(uint64_t entityID) {
        Scene* scene = ScriptEngine::GetActiveScene();
        if (!scene) return Coral::String::New("");
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<AudioComponent>() ? Coral::String::New(entity.GetComponent<AudioComponent>().SoundPath) : Coral::String::New("");
    }

    // ── Application ───────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Application_Close() { Application::Get().Close(); }
    CH_SCRIPT_FUNC void Window_SetSize(int w, int h) { Application::Get().GetWindow().SetSize(w, h); }
    CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled) { Application::Get().GetWindow().SetFullscreen(enabled); }
    CH_SCRIPT_FUNC void Window_SetVSync(bool enabled) { Application::Get().GetWindow().SetVSync(enabled); }
    CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled) { Application::Get().GetWindow().SetAntialiasing(enabled); }

    ManagedScriptInstance* s_PendingScriptInstance = nullptr;

    void ScriptGlue::SetPendingScriptInstance(ManagedScriptInstance* instance) {
        s_PendingScriptInstance = instance;
    }

    CH_SCRIPT_FUNC void RegisterLifecyclePointers(uint64_t entityID, void* onCreate, void* onStart, void* onUpdate, void* onDestroy) {
        if (s_PendingScriptInstance) {
            s_PendingScriptInstance->OnCreate = (void(*)())onCreate;
            s_PendingScriptInstance->OnStart = (void(*)())onStart;
            s_PendingScriptInstance->OnUpdate = (void(*)(float))onUpdate;
            s_PendingScriptInstance->OnDestroy = (void(*)())onDestroy;
        }
    }

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        #define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr) \
            assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)

        // Script lifecycle
        CH_ADD_INTERNAL_CALL(Script, RegisterLifecyclePointers, RegisterLifecyclePointers);

        // Logging
        CH_ADD_INTERNAL_CALL(Log, Log_Info_Ptr, Log_Info);
        CH_ADD_INTERNAL_CALL(Log, Log_Warn_Ptr, Log_Warn);
        CH_ADD_INTERNAL_CALL(Log, Log_Error_Ptr, Log_Error);

        // Input
        CH_ADD_INTERNAL_CALL(Input, Input_IsKeyDown_Ptr, Input_IsKeyDown);
        CH_ADD_INTERNAL_CALL(Input, Input_IsKeyPressed_Ptr, Input_IsKeyPressed);
        CH_ADD_INTERNAL_CALL(Input, Input_IsKeyReleased_Ptr, Input_IsKeyReleased);
        CH_ADD_INTERNAL_CALL(Input, Input_IsMouseButtonDown_Ptr, Input_IsMouseButtonDown);
        CH_ADD_INTERNAL_CALL(Input, Input_IsMouseButtonPressed_Ptr, Input_IsMouseButtonPressed);
        CH_ADD_INTERNAL_CALL(Input, Input_GetMouseDelta_Ptr, Input_GetMouseDelta);
        CH_ADD_INTERNAL_CALL(Input, Input_GetMouseWheelMove_Ptr, Input_GetMouseWheelMove);

        // Scene
        CH_ADD_INTERNAL_CALL(Scene, Scene_FindEntityByTag_Ptr, Scene_FindEntityByTag);
        CH_ADD_INTERNAL_CALL(Scene, Scene_LoadScene_Ptr, Scene_LoadScene);
        CH_ADD_INTERNAL_CALL(Scene, Scene_GetPrimaryCameraEntity_Ptr, Scene_GetPrimaryCameraEntity);

        // Application / Window
        CH_ADD_INTERNAL_CALL(Application, Application_Close_Ptr, Application_Close);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetSize_Ptr, Window_SetSize);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetFullscreen_Ptr, Window_SetFullscreen);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetVSync_Ptr, Window_SetVSync);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetAntialiasing_Ptr, Window_SetAntialiasing);

        // Audio (Global)
        CH_ADD_INTERNAL_CALL(Audio, Audio_Play_Ptr, Audio_Play);
        CH_ADD_INTERNAL_CALL(Audio, Audio_Stop_Ptr, Audio_Stop);

        // Entity / Transform
        CH_ADD_INTERNAL_CALL(Entity, Entity_HasComponent_Ptr, Entity_HasComponent);
        CH_ADD_INTERNAL_CALL(Entity, Entity_GetTranslation_Ptr, Entity_GetTranslation);
        CH_ADD_INTERNAL_CALL(Entity, Entity_SetTranslation_Ptr, Entity_SetTranslation);
        CH_ADD_INTERNAL_CALL(Entity, Entity_GetRotation_Ptr, Entity_GetRotation);
        CH_ADD_INTERNAL_CALL(Entity, Entity_SetRotation_Ptr, Entity_SetRotation);
        CH_ADD_INTERNAL_CALL(Entity, Entity_GetScale_Ptr, Entity_GetScale);
        CH_ADD_INTERNAL_CALL(Entity, Entity_SetScale_Ptr, Entity_SetScale);
        CH_ADD_INTERNAL_CALL(Entity, Entity_GetVelocity_Ptr, Entity_GetVelocity);
        CH_ADD_INTERNAL_CALL(Entity, Entity_SetVelocity_Ptr, Entity_SetVelocity);
        CH_ADD_INTERNAL_CALL(Entity, Entity_IsGrounded_Ptr, Entity_IsGrounded);

        // TagComponent
        CH_ADD_INTERNAL_CALL(TagComponent, TagComponent_GetTag_Ptr, TagComponent_GetTag);

        // CameraComponent
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetForward_Ptr, Camera_GetForward);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetRight_Ptr, Camera_GetRight);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetOrbit_Ptr, Camera_GetOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetOrbit_Ptr, Camera_SetOrbit);

        // PlayerComponent
        CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_GetMovementSpeed_Ptr, PlayerComponent_GetMovementSpeed);
        CH_ADD_INTERNAL_CALL(PlayerComponent, PlayerComponent_SetMovementSpeed_Ptr, PlayerComponent_SetMovementSpeed);

        // AudioComponent
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_SetVolume_Ptr, AudioComponent_SetVolume);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_SetLoop_Ptr, AudioComponent_SetLoop);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_IsPlaying_Ptr, AudioComponent_IsPlaying);
        CH_ADD_INTERNAL_CALL(AudioComponent, AudioComponent_GetSoundPath_Ptr, AudioComponent_GetSoundPath);

        // UI Controls
        CH_ADD_INTERNAL_CALL(ButtonControl, ButtonControl_IsPressed_Ptr, ButtonControl_IsPressed);
        CH_ADD_INTERNAL_CALL(CheckboxControl, CheckboxControl_GetChecked_Ptr, CheckboxControl_GetChecked);
        CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetSelectedIndex_Ptr, ComboBoxControl_GetSelectedIndex);
        CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetItem_Ptr, ComboBoxControl_GetItem);

        // Gameplay
        CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_IsActive_Ptr, SpawnComponent_IsActive);
        CH_ADD_INTERNAL_CALL(SpawnComponent, SpawnComponent_GetSpawnPoint_Ptr, SpawnComponent_GetSpawnPoint);
        CH_ADD_INTERNAL_CALL(SceneTransitionComponent, SceneTransitionComponent_GetTargetScene_Ptr, SceneTransitionComponent_GetTargetScene);

        #undef CH_ADD_INTERNAL_CALL

        // Finalize registration
        assembly.UploadInternalCalls();
    }

} // namespace CHEngine
