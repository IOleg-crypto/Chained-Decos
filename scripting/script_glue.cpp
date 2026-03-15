#include <Coral/String.hpp>
#include <Coral/Array.hpp>
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
#include "raylib.h"
#include "raymath.h"
#include "script_glue.h"

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
        auto* scene = ScriptEngine::Get().GetActiveScene();
        if (scene) {
            auto entity = scene->FindEntityByTag((std::string)tag);
            return entity ? (uint64_t)(uint32_t)entity : 0;
        }
        return 0;
    }

    CH_SCRIPT_FUNC void Scene_LoadScene(Coral::String path) {
        ScriptEngine::Get().RequestLoadScene((std::string)path);
    }

    CH_SCRIPT_FUNC uint64_t Scene_GetPrimaryCameraEntity() {
        if (!ScriptEngine::Get().GetActiveScene()) return 0;
        Entity entity = ScriptEngine::Get().GetActiveScene()->GetPrimaryCameraEntity();
        return entity ? (uint64_t)(uint32_t)entity : 0;
    }

    // ── Entity / Transform ────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Entity_GetTranslation(uint64_t entityID, Vector3* outTranslation) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outTranslation = entity.GetComponent<TransformComponent>().Translation;
    }

    CH_SCRIPT_FUNC void Entity_SetTranslation(uint64_t entityID, Vector3* inTranslation) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().Translation = *inTranslation;
    }

    CH_SCRIPT_FUNC void Entity_GetRotation(uint64_t entityID, Vector3* outRotation) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }

    CH_SCRIPT_FUNC void Entity_SetRotation(uint64_t entityID, Vector3* inRotation) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().SetRotation(*inRotation);
    }

    CH_SCRIPT_FUNC void Entity_GetScale(uint64_t entityID, Vector3* outScale) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            *outScale = entity.GetComponent<TransformComponent>().Scale;
    }

    CH_SCRIPT_FUNC void Entity_SetScale(uint64_t entityID, Vector3* inScale) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) 
            entity.GetComponent<TransformComponent>().Scale = *inScale;
    }

    CH_SCRIPT_FUNC bool Entity_HasComponent(uint64_t entityID, Coral::String componentName) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
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
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<RigidBodyComponent>()) 
            *outVelocity = entity.GetComponent<RigidBodyComponent>().Velocity;
    }

    CH_SCRIPT_FUNC void Entity_SetVelocity(uint64_t entityID, Vector3* inVelocity) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<RigidBodyComponent>()) 
            entity.GetComponent<RigidBodyComponent>().Velocity = *inVelocity;
    }

    CH_SCRIPT_FUNC bool Entity_IsGrounded(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<RigidBodyComponent>() ? entity.GetComponent<RigidBodyComponent>().IsGrounded : false;
    }

    CH_SCRIPT_FUNC Coral::Array<uint64_t> Entity_FindAllWithComponent(Coral::String componentName) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
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

        return Coral::Array<uint64_t>::New(ids);
    }

    // ── Tag ───────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC Coral::String TagComponent_GetTag(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return Coral::String::New("");
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TagComponent>()) 
            return Coral::String::New(entity.GetComponent<TagComponent>().Tag); 
        return Coral::String::New("");
    }

    // ── Camera ────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Camera_GetForward(uint64_t entityID, Vector3* outForward) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            *outForward = Vector3RotateByQuaternion({0.0f, 0.0f, -1.0f}, tc.RotationQuat);
        }
    }

    CH_SCRIPT_FUNC void Camera_GetRight(uint64_t entityID, Vector3* outRight) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<TransformComponent>()) {
            auto& tc = entity.GetComponent<TransformComponent>();
            *outRight = Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, tc.RotationQuat);
        }
    }

    CH_SCRIPT_FUNC void Camera_GetOrbit(uint64_t entityID, float* yaw, float* pitch, float* distance) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
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
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<CameraComponent>()) {
            auto& camera = entity.GetComponent<CameraComponent>();
            camera.OrbitYaw = yaw;
            camera.OrbitPitch = pitch;
            camera.OrbitDistance = distance;
        }
    }

    CH_SCRIPT_FUNC bool Camera_GetPrimary(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().Primary : false;
    }

    CH_SCRIPT_FUNC void Camera_SetPrimary(uint64_t entityID, bool primary) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().Primary = primary;
    }

    CH_SCRIPT_FUNC bool Camera_GetIsOrbit(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().IsOrbitCamera : false;
    }

    CH_SCRIPT_FUNC void Camera_SetIsOrbit(uint64_t entityID, bool isOrbit) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().IsOrbitCamera = isOrbit;
    }

    CH_SCRIPT_FUNC Coral::String Camera_GetTargetTag(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return Coral::String::New("");
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<CameraComponent>() ? Coral::String::New(entity.GetComponent<CameraComponent>().TargetEntityTag) : Coral::String::New("");
    }

    CH_SCRIPT_FUNC void Camera_SetTargetTag(uint64_t entityID, Coral::String tag) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<CameraComponent>()) 
            entity.GetComponent<CameraComponent>().TargetEntityTag = (std::string)tag;
    }

    // ── UI Controls ───────────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool ButtonControl_IsPressed(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<ButtonControl>() ? entity.GetComponent<ButtonControl>().PressedThisFrame : false;
    } 

    CH_SCRIPT_FUNC bool CheckboxControl_GetChecked(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<CheckboxControl>() ? entity.GetComponent<CheckboxControl>().Checked : false;
    }

    CH_SCRIPT_FUNC int ComboBoxControl_GetSelectedIndex(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return 0;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<ComboBoxControl>() ? entity.GetComponent<ComboBoxControl>().SelectedIndex : 0;
    }

    CH_SCRIPT_FUNC Coral::String ComboBoxControl_GetItem(uint64_t entityID, int index) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return Coral::String::New("");
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<ComboBoxControl>()) {
            auto& combo = entity.GetComponent<ComboBoxControl>();
            if (index >= 0 && index < (int)combo.Items.size())
                return Coral::String::New(combo.Items[index]);
        }
        return Coral::String::New("");
    }

    // ── Spawn / Transition ────────────────────────────────────────────────
    CH_SCRIPT_FUNC bool SpawnComponent_IsActive(uint64_t entityID) { return false; }
    CH_SCRIPT_FUNC void SpawnComponent_GetSpawnPoint(uint64_t entityID, Vector3* point) { *point = {0,0,0}; }
    CH_SCRIPT_FUNC Coral::String SceneTransitionComponent_GetTargetScene(uint64_t entityID) { return Coral::String::New(""); }

    // ── PlayerComponent ───────────────────────────────────────────────────
    CH_SCRIPT_FUNC float PlayerComponent_GetMovementSpeed(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return 0.0f;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<PlayerComponent>() ? entity.GetComponent<PlayerComponent>().MovementSpeed : 0.0f;
    }

    CH_SCRIPT_FUNC void PlayerComponent_SetMovementSpeed(uint64_t entityID, float speed) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<PlayerComponent>()) 
            entity.GetComponent<PlayerComponent>().MovementSpeed = speed;
    }

    // ── Audio ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Audio_Play(Coral::String path, float volume, float pitch, bool loop) {
        if (Project::GetActive() != nullptr) {
            auto asset = AssetManager::Get().Get<SoundAsset>((std::string)path);
            if (asset) Audio::Get().Play(asset, volume, pitch, loop);
        }
    }

    CH_SCRIPT_FUNC void Audio_Stop(Coral::String path) {
        if (Project::GetActive() != nullptr) {
            auto asset = AssetManager::Get().Get<SoundAsset>((std::string)path);
            if (asset) Audio::Get().Stop(asset);
        }
    }

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Volume = volume;
    }

    CH_SCRIPT_FUNC void AudioComponent_SetLoop(uint64_t entityID, bool loop) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        if (entity && entity.HasComponent<AudioComponent>()) entity.GetComponent<AudioComponent>().Loop = loop;
    }

    CH_SCRIPT_FUNC bool AudioComponent_IsPlaying(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return false;
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<AudioComponent>() ? entity.GetComponent<AudioComponent>().IsPlaying : false;
    }

    CH_SCRIPT_FUNC Coral::String AudioComponent_GetSoundPath(uint64_t entityID) {
        Scene* scene = ScriptEngine::Get().GetActiveScene();
        if (!scene) return Coral::String::New("");
        Entity entity((entt::entity)(uint32_t)entityID, &scene->GetRegistry());
        return entity && entity.HasComponent<AudioComponent>() ? Coral::String::New(entity.GetComponent<AudioComponent>().SoundPath) : Coral::String::New("");
    }

    // ── UI ────────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void UI_Text(Coral::String text) {
        // Since we are in OnGUI, the ImGui context should be active.
        // We use the engine's UIRenderer or direct ImGui calls.
        // For simplicity and to avoid including imgui.h everywhere, 
        // we can use a helper or just CH_CORE_INFO for now if imgui is not linked.
        // But wait, scripting module should probably have access to imgui if it's for UI.
        
        // Actually, let's just log for now if we want to avoid deep dependencies, 
        // OR better, use RAYLIB's DrawText if it's simpler for "HUD" style UI.
        // But the user mentioned ImGui in the past.
        
        // If we want REAL ImGui:
        // ImGui::Text("%s", ((std::string)text).c_str());
        
        // Let's use CH_CORE_INFO as a placeholder that also "renders" to log
        CH_CORE_TRACE("[UI] {}", (std::string)text);
    }
    CH_SCRIPT_FUNC void Application_Close() { Application::Get().Close(); }
    CH_SCRIPT_FUNC void Window_SetSize(int w, int h) { Application::Get().GetWindow().SetSize(w, h); }
    CH_SCRIPT_FUNC void Window_SetFullscreen(bool enabled) { Application::Get().GetWindow().SetFullscreen(enabled); }
    CH_SCRIPT_FUNC void Window_SetVSync(bool enabled) { Application::Get().GetWindow().SetVSync(enabled); }
    CH_SCRIPT_FUNC void Window_SetAntialiasing(bool enabled) { Application::Get().GetWindow().SetAntialiasing(enabled); }

    ManagedScriptInstance* s_PendingScriptInstance = nullptr;

    void ScriptGlue::SetPendingScriptInstance(ManagedScriptInstance* instance) {
        s_PendingScriptInstance = instance;
    }

    CH_SCRIPT_FUNC void RegisterLifecyclePointers(uint64_t entityID, void* onCreate, void* onStart, void* onUpdate, void* onDestroy, void* onGUI, void* onCollisionEnter) {
        if (s_PendingScriptInstance) {
            s_PendingScriptInstance->OnCreate = (void(*)())onCreate;
            s_PendingScriptInstance->OnStart = (void(*)())onStart;
            s_PendingScriptInstance->OnUpdate = (void(*)(float))onUpdate;
            s_PendingScriptInstance->OnDestroy = (void(*)())onDestroy;
            s_PendingScriptInstance->OnGUI = (void(*)())onGUI;
            s_PendingScriptInstance->OnCollisionEnter = (void(*)(uint64_t))onCollisionEnter;
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
        CH_ADD_INTERNAL_CALL(Entity, Entity_FindAllWithComponent_Ptr, Entity_FindAllWithComponent);
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

        // UI
        CH_ADD_INTERNAL_CALL(UI, UI_Text_Ptr, UI_Text);

        // Application
        CH_ADD_INTERNAL_CALL(Application, Application_Close_Ptr, Application_Close);

        // AppWindow
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetSize_Ptr, Window_SetSize);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetFullscreen_Ptr, Window_SetFullscreen);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetVSync_Ptr, Window_SetVSync);
        CH_ADD_INTERNAL_CALL(AppWindow, Window_SetAntialiasing_Ptr, Window_SetAntialiasing);

        // Audio
        CH_ADD_INTERNAL_CALL(Audio, Audio_Play_Ptr, Audio_Play);
        CH_ADD_INTERNAL_CALL(Audio, Audio_Stop_Ptr, Audio_Stop);

        // Scene
        CH_ADD_INTERNAL_CALL(Scene, Scene_FindEntityByTag_Ptr, Scene_FindEntityByTag);
        CH_ADD_INTERNAL_CALL(Scene, Scene_LoadScene_Ptr, Scene_LoadScene);
        CH_ADD_INTERNAL_CALL(Scene, Scene_GetPrimaryCameraEntity_Ptr, Scene_GetPrimaryCameraEntity);

        // TagComponent
        CH_ADD_INTERNAL_CALL(TagComponent, TagComponent_GetTag_Ptr, TagComponent_GetTag);

        // CameraComponent
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetForward_Ptr, Camera_GetForward);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetRight_Ptr, Camera_GetRight);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetOrbit_Ptr, Camera_GetOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetOrbit_Ptr, Camera_SetOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetPrimary_Ptr, Camera_GetPrimary);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetPrimary_Ptr, Camera_SetPrimary);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetIsOrbit_Ptr, Camera_GetIsOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetIsOrbit_Ptr, Camera_SetIsOrbit);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_GetTargetTag_Ptr, Camera_GetTargetTag);
        CH_ADD_INTERNAL_CALL(CameraComponent, Camera_SetTargetTag_Ptr, Camera_SetTargetTag);

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
