// script_glue.cpp
// Registers every C++ function as a Coral internal call.
// Coral automatically fills the matching delegate* fields on the C# side.
// Adding a new function: 1) declare in script_glue_*.h, 2) implement in script_glue_*.cpp, 3) add one AddInternalCall
// line here.
#include "script_glue.h"
#include "script_interop_pointers.h"
#include "engine/core/log.h"
#include "script_glue_internal.h"
#include "script_glue_system.h"
#include "script_glue_scene.h"
#include "script_glue_entity.h"
#include "script_glue_camera.h"
#include "script_glue_ui.h"
#include "script_glue_audio.h"
#include "script_glue_input.h"
#include <Coral/Assembly.hpp>

namespace Chained
{

// Called from C# Interop.RegisterCallbacks() — receives the 7 lifecycle function pointers.
static void RegisterManagedCallbacks(ManagedCallbacks* cb)
{
    if (!cb)
    {
        return;
    }
    ManagedCallbacks_::OnUpdate = cb->OnUpdate;
    ManagedCallbacks_::OnEvent = cb->OnEvent;
    ManagedCallbacks_::OnRenderUI = cb->OnRenderUI;
    ManagedCallbacks_::OnCollisionEnter = cb->OnCollisionEnter;
    ManagedCallbacks_::ClearAll = cb->ClearAll;
    ManagedCallbacks_::InstantiateScript = cb->InstantiateScript;
    ManagedCallbacks_::DestroyScript = cb->DestroyScript;
    CH_CORE_INFO("[ScriptGlue] ManagedCallbacks registered.");
}

void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly)
{
    // ── RegisterManagedCallbacks (C# → C++ lifecycle) ─────────────────
    assembly.AddInternalCall("Chained.Interop", "RegisterManagedCallbacks_Ptr", (void*)&RegisterManagedCallbacks);

    // ── Entity ────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Entity", "Entity_HasComponent_Ptr", (void*)&Entity_HasComponent);
    assembly.AddInternalCall("Chained.Entity", "Entity_FindAllWithComponent_Ptr", (void*)&Entity_FindAllWithComponent);
    assembly.AddInternalCall("Chained.Entity", "Entity_AddComponent_Ptr", (void*)&Entity_AddComponent);

    // ── Transform ─────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.TransformComponent", "Transform_GetTranslation_Ptr",
                             (void*)&Transform_GetTranslation);
    assembly.AddInternalCall("Chained.TransformComponent", "Transform_SetTranslation_Ptr",
                             (void*)&Transform_SetTranslation);
    assembly.AddInternalCall("Chained.TransformComponent", "Transform_GetRotation_Ptr", (void*)&Transform_GetRotation);
    assembly.AddInternalCall("Chained.TransformComponent", "Transform_SetRotation_Ptr", (void*)&Transform_SetRotation);
    assembly.AddInternalCall("Chained.TransformComponent", "Transform_GetScale_Ptr", (void*)&Transform_GetScale);
    assembly.AddInternalCall("Chained.TransformComponent", "Transform_SetScale_Ptr", (void*)&Transform_SetScale);

    // ── Model ─────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.ModelComponent", "Model_GetModelPath_Ptr", (void*)&Model_GetModelPath);
    assembly.AddInternalCall("Chained.ModelComponent", "Model_SetModelPath_Ptr", (void*)&Model_SetModelPath);

    // ── RigidBody ─────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.RigidBodyComponent", "RigidBody_GetVelocity_Ptr", (void*)&RigidBody_GetVelocity);
    assembly.AddInternalCall("Chained.RigidBodyComponent", "RigidBody_SetVelocity_Ptr", (void*)&RigidBody_SetVelocity);
    assembly.AddInternalCall("Chained.RigidBodyComponent", "RigidBody_IsGrounded_Ptr", (void*)&RigidBody_IsGrounded);
    assembly.AddInternalCall("Chained.RigidBodyComponent", "RigidBody_IsKinematic_Ptr", (void*)&RigidBody_IsKinematic);
    assembly.AddInternalCall("Chained.RigidBodyComponent", "RigidBody_SetKinematic_Ptr",
                             (void*)&RigidBody_SetKinematic);

    // ── Tag ───────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.TagComponent", "TagComponent_GetTag_Ptr", (void*)&TagComponent_GetTag);

    // ── Camera ────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_GetForward_Ptr", (void*)&Camera_GetForward);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_GetRight_Ptr", (void*)&Camera_GetRight);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_GetOrbit_Ptr", (void*)&Camera_GetOrbit);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_SetOrbit_Ptr", (void*)&Camera_SetOrbit);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_GetPrimary_Ptr", (void*)&Camera_GetPrimary);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_SetPrimary_Ptr", (void*)&Camera_SetPrimary);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_GetIsOrbit_Ptr", (void*)&Camera_GetIsOrbit);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_SetIsOrbit_Ptr", (void*)&Camera_SetIsOrbit);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_GetTargetTag_Ptr", (void*)&Camera_GetTargetTag);
    assembly.AddInternalCall("Chained.CameraComponent", "Camera_SetTargetTag_Ptr", (void*)&Camera_SetTargetTag);

    // ── PlayerComponent ───────────────────────────────────────────────
    assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_GetMovementSpeed_Ptr",
                             (void*)&PlayerComponent_GetMovementSpeed);
    assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_SetMovementSpeed_Ptr",
                             (void*)&PlayerComponent_SetMovementSpeed);
    assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_GetJumpForce_Ptr",
                             (void*)&PlayerComponent_GetJumpForce);
    assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_SetJumpForce_Ptr",
                             (void*)&PlayerComponent_SetJumpForce);
    assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_GetLookSensitivity_Ptr",
                             (void*)&PlayerComponent_GetLookSensitivity);
    assembly.AddInternalCall("Chained.PlayerComponent", "PlayerComponent_SetLookSensitivity_Ptr",
                             (void*)&PlayerComponent_SetLookSensitivity);

    // ── AudioComponent ────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.AudioComponent", "AudioComponent_SetVolume_Ptr",
                             (void*)&AudioComponent_SetVolume);
    assembly.AddInternalCall("Chained.AudioComponent", "AudioComponent_SetLoop_Ptr", (void*)&AudioComponent_SetLoop);
    assembly.AddInternalCall("Chained.AudioComponent", "AudioComponent_IsPlaying_Ptr",
                             (void*)&AudioComponent_IsPlaying);
    assembly.AddInternalCall("Chained.AudioComponent", "AudioComponent_GetSoundPath_Ptr",
                             (void*)&AudioComponent_GetSoundPath);
    assembly.AddInternalCall("Chained.AudioComponent", "AudioComponent_Play_Ptr", (void*)&AudioComponent_Play);
    assembly.AddInternalCall("Chained.AudioComponent", "AudioComponent_Stop_Ptr", (void*)&AudioComponent_Stop);

    // ── SpriteComponent ───────────────────────────────────────────────
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_GetTexturePath_Ptr",
                             (void*)&SpriteComponent_GetTexturePath);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_SetTexturePath_Ptr",
                             (void*)&SpriteComponent_SetTexturePath);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_GetTint_Ptr", (void*)&SpriteComponent_GetTint);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_SetTint_Ptr", (void*)&SpriteComponent_SetTint);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_GetFlipX_Ptr",
                             (void*)&SpriteComponent_GetFlipX);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_SetFlipX_Ptr",
                             (void*)&SpriteComponent_SetFlipX);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_GetFlipY_Ptr",
                             (void*)&SpriteComponent_GetFlipY);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_SetFlipY_Ptr",
                             (void*)&SpriteComponent_SetFlipY);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_GetZOrder_Ptr",
                             (void*)&SpriteComponent_GetZOrder);
    assembly.AddInternalCall("Chained.SpriteComponent", "SpriteComponent_SetZOrder_Ptr",
                             (void*)&SpriteComponent_SetZOrder);

    // ── UI Controls ───────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.ButtonControl", "ButtonControl_IsClicked_Ptr", (void*)&ButtonControl_IsClicked);
    assembly.AddInternalCall("Chained.ButtonControl", "ButtonControl_IsDown_Ptr", (void*)&ButtonControl_IsDown);
    assembly.AddInternalCall("Chained.ButtonControl", "ButtonControl_GetLabel_Ptr", (void*)&ButtonControl_GetLabel);
    assembly.AddInternalCall("Chained.ButtonControl", "ButtonControl_SetLabel_Ptr", (void*)&ButtonControl_SetLabel);

    assembly.AddInternalCall("Chained.CheckboxControl", "CheckboxControl_GetChecked_Ptr",
                             (void*)&CheckboxControl_GetChecked);
    assembly.AddInternalCall("Chained.CheckboxControl", "CheckboxControl_SetChecked_Ptr",
                             (void*)&CheckboxControl_SetChecked);

    assembly.AddInternalCall("Chained.LabelControl", "LabelControl_GetText_Ptr", (void*)&LabelControl_GetText);
    assembly.AddInternalCall("Chained.LabelControl", "LabelControl_SetText_Ptr", (void*)&LabelControl_SetText);

    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_GetLabel_Ptr", (void*)&SliderControl_GetLabel);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_SetLabel_Ptr", (void*)&SliderControl_SetLabel);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_GetValue_Ptr", (void*)&SliderControl_GetValue);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_SetValue_Ptr", (void*)&SliderControl_SetValue);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_GetMin_Ptr", (void*)&SliderControl_GetMin);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_SetMin_Ptr", (void*)&SliderControl_SetMin);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_GetMax_Ptr", (void*)&SliderControl_GetMax);
    assembly.AddInternalCall("Chained.SliderControl", "SliderControl_SetMax_Ptr", (void*)&SliderControl_SetMax);

    assembly.AddInternalCall("Chained.ProgressBarControl", "ProgressBarControl_GetProgress_Ptr",
                             (void*)&ProgressBarControl_GetProgress);
    assembly.AddInternalCall("Chained.ProgressBarControl", "ProgressBarControl_SetProgress_Ptr",
                             (void*)&ProgressBarControl_SetProgress);
    assembly.AddInternalCall("Chained.ProgressBarControl", "ProgressBarControl_GetOverlayText_Ptr",
                             (void*)&ProgressBarControl_GetOverlayText);
    assembly.AddInternalCall("Chained.ProgressBarControl", "ProgressBarControl_SetOverlayText_Ptr",
                             (void*)&ProgressBarControl_SetOverlayText);
    assembly.AddInternalCall("Chained.ProgressBarControl", "ProgressBarControl_GetShowPercentage_Ptr",
                             (void*)&ProgressBarControl_GetShowPercentage);
    assembly.AddInternalCall("Chained.ProgressBarControl", "ProgressBarControl_SetShowPercentage_Ptr",
                             (void*)&ProgressBarControl_SetShowPercentage);

    assembly.AddInternalCall("Chained.WidgetControl", "WidgetControl_GetActive_Ptr", (void*)&WidgetControl_GetActive);
    assembly.AddInternalCall("Chained.WidgetControl", "WidgetControl_SetActive_Ptr", (void*)&WidgetControl_SetActive);
    assembly.AddInternalCall("Chained.WidgetControl", "WidgetControl_GetTextColor_Ptr",
                             (void*)&WidgetControl_GetTextColor);
    assembly.AddInternalCall("Chained.WidgetControl", "WidgetControl_SetTextColorRGBA_Ptr",
                             (void*)&WidgetControl_SetTextColorRGBA);

    assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_GetSelectedIndex_Ptr",
                             (void*)&ComboBoxControl_GetSelectedIndex);
    assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_SetSelectedIndex_Ptr",
                             (void*)&ComboBoxControl_SetSelectedIndex);
    assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_AddItem_Ptr", (void*)&ComboBoxControl_AddItem);
    assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_ClearItems_Ptr",
                             (void*)&ComboBoxControl_ClearItems);
    assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_GetItemCount_Ptr",
                             (void*)&ComboBoxControl_GetItemCount);
    assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_GetItem_Ptr", (void*)&ComboBoxControl_GetItem);

    // ── SpawnComponent ────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.SpawnComponent", "SpawnComponent_IsActive_Ptr", (void*)&SpawnComponent_IsActive);
    assembly.AddInternalCall("Chained.SpawnComponent", "SpawnComponent_GetSpawnPoint_Ptr",
                             (void*)&SpawnComponent_GetSpawnPoint);
    assembly.AddInternalCall("Chained.SpawnComponent", "SpawnComponent_GetRenderSpawnZoneInScene_Ptr",
                             (void*)&SpawnComponent_GetRenderSpawnZoneInScene);
    assembly.AddInternalCall("Chained.SpawnComponent", "SpawnComponent_GetZoneSize_Ptr",
                             (void*)&SpawnComponent_GetZoneSize);

    // ── Shader ────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.ShaderComponent", "Shader_SetFloat_Ptr", (void*)&Shader_SetFloat);
    assembly.AddInternalCall("Chained.ShaderComponent", "Shader_SetVec3_Ptr", (void*)&Shader_SetVec3);
    assembly.AddInternalCall("Chained.ShaderComponent", "Shader_GetEnabled_Ptr", (void*)&Shader_GetEnabled);
    assembly.AddInternalCall("Chained.ShaderComponent", "Shader_SetEnabled_Ptr", (void*)&Shader_SetEnabled);

    // ── UI static ─────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.UI", "UI_Text_Ptr", (void*)&UI_Text);

    // ── Input ─────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Input", "Input_IsKeyDown_Ptr", (void*)&Input_IsKeyDown);
    assembly.AddInternalCall("Chained.Input", "Input_IsKeyPressed_Ptr", (void*)&Input_IsKeyPressed);
    assembly.AddInternalCall("Chained.Input", "Input_IsKeyReleased_Ptr", (void*)&Input_IsKeyReleased);
    assembly.AddInternalCall("Chained.Input", "Input_IsMouseButtonDown_Ptr", (void*)&Input_IsMouseButtonDown);
    assembly.AddInternalCall("Chained.Input", "Input_IsMouseButtonPressed_Ptr", (void*)&Input_IsMouseButtonPressed);
    assembly.AddInternalCall("Chained.Input", "Input_GetMouseWheelMove_Ptr", (void*)&Input_GetMouseWheelMove);
    assembly.AddInternalCall("Chained.Input", "Input_GetMouseDelta_Ptr", (void*)&Input_GetMouseDelta);

    // ── Log ───────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Log", "Log_Info_Ptr", (void*)&Log_Info);
    assembly.AddInternalCall("Chained.Log", "Log_Warn_Ptr", (void*)&Log_Warn);
    assembly.AddInternalCall("Chained.Log", "Log_Error_Ptr", (void*)&Log_Error);

    // ── Scene ─────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Scene", "Scene_FindEntityByTag_Ptr", (void*)&Scene_FindEntityByTag);
    assembly.AddInternalCall("Chained.Scene", "Scene_LoadScene_Ptr", (void*)&Scene_LoadScene);
    assembly.AddInternalCall("Chained.Scene", "Scene_GetPrimaryCameraEntity_Ptr", (void*)&Scene_GetPrimaryCameraEntity);
    assembly.AddInternalCall("Chained.Scene", "Scene_CopyEntity_Ptr", (void*)&Scene_CopyEntity);

    // ── Audio static ──────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Audio", "Audio_Play_Ptr", (void*)&Audio_Play);
    assembly.AddInternalCall("Chained.Audio", "Audio_Stop_Ptr", (void*)&Audio_Stop);
    assembly.AddInternalCall("Chained.Audio", "Audio_StopAll_Ptr", (void*)&Audio_StopAll);

    // ── Application ───────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Application", "Application_Close_Ptr", (void*)&Application_Close);
    assembly.AddInternalCall("Chained.Application", "Application_GetFPS_Ptr", (void*)&Application_GetFPS);
    assembly.AddInternalCall("Chained.Application", "Application_GetFrameTime_Ptr", (void*)&Application_GetFrameTime);

    // ── Window ────────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.AppWindow", "Window_SetSize_Ptr", (void*)&Window_SetSize);
    assembly.AddInternalCall("Chained.AppWindow", "Window_SetFullscreen_Ptr", (void*)&Window_SetFullscreen);
    assembly.AddInternalCall("Chained.AppWindow", "Window_SetVSync_Ptr", (void*)&Window_SetVSync);
    assembly.AddInternalCall("Chained.AppWindow", "Window_SetAntialiasing_Ptr", (void*)&Window_SetAntialiasing);
    assembly.AddInternalCall("Chained.AppWindow", "Window_SetAntiAliasingSamples_Ptr",
                             (void*)&Window_SetAntiAliasingSamples);
    assembly.AddInternalCall("Chained.AppWindow", "Window_GetSupportedResolution_Ptr",
                             (void*)&Window_GetSupportedResolution);

    // ── Physics ─────────────────────────────────────────────────────
    assembly.AddInternalCall("Chained.Physics", "Physics_GetGravity_Ptr", (void*)&Physics_GetGravity);

    assembly.UploadInternalCalls();
    CH_CORE_INFO("[ScriptGlue] Registered {} internal calls for '{}'.", 121, (std::string)assembly.GetName());
}

} // namespace Chained
