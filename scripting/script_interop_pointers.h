#ifndef CH_SCRIPT_INTEROP_POINTERS_H
#define CH_SCRIPT_INTEROP_POINTERS_H

#include "script_glue_audio.h"
#include "script_glue_camera.h"
#include "script_glue_entity.h"
#include "script_glue_internal.h"
#include "script_glue_scene.h"
#include "script_glue_system.h"
#include "script_glue_ui.h"
#include <stdint.h>


#include "script_glue_input.h"
#include "script_internal_call_registry.h"

namespace Chained
{

struct ChainedNativePointers
{
    void* Entity_HasComponent_Ptr;
    void* Entity_FindAllWithComponent_Ptr;
    void* Entity_AddComponent_Ptr;
    void* Transform_GetTranslation_Ptr;
    void* Transform_SetTranslation_Ptr;
    void* Transform_GetRotation_Ptr;
    void* Transform_SetRotation_Ptr;
    void* Transform_GetScale_Ptr;
    void* Transform_SetScale_Ptr;
    void* Model_GetModelPath_Ptr;
    void* Model_SetModelPath_Ptr;
    void* RigidBody_GetVelocity_Ptr;
    void* RigidBody_SetVelocity_Ptr;
    void* RigidBody_IsGrounded_Ptr;
    void* RigidBody_IsKinematic_Ptr;
    void* RigidBody_SetKinematic_Ptr;
    void* TagComponent_GetTag_Ptr;
    void* Camera_GetForward_Ptr;
    void* Camera_GetRight_Ptr;
    void* Camera_GetOrbit_Ptr;
    void* Camera_SetOrbit_Ptr;
    void* Camera_GetPrimary_Ptr;
    void* Camera_SetPrimary_Ptr;
    void* Camera_GetIsOrbit_Ptr;
    void* Camera_SetIsOrbit_Ptr;
    void* Camera_GetTargetTag_Ptr;
    void* Camera_SetTargetTag_Ptr;
    void* PlayerComponent_GetMovementSpeed_Ptr;
    void* PlayerComponent_SetMovementSpeed_Ptr;
    void* PlayerComponent_GetJumpForce_Ptr;
    void* PlayerComponent_SetJumpForce_Ptr;
    void* PlayerComponent_GetLookSensitivity_Ptr;
    void* PlayerComponent_SetLookSensitivity_Ptr;
    void* AudioComponent_SetVolume_Ptr;
    void* AudioComponent_SetLoop_Ptr;
    void* AudioComponent_IsPlaying_Ptr;
    void* AudioComponent_GetSoundPath_Ptr;
    void* AudioComponent_Play_Ptr;
    void* AudioComponent_Stop_Ptr;
    void* SpriteComponent_GetTexturePath_Ptr;
    void* SpriteComponent_SetTexturePath_Ptr;
    void* SpriteComponent_GetTint_Ptr;
    void* SpriteComponent_SetTint_Ptr;
    void* SpriteComponent_GetFlipX_Ptr;
    void* SpriteComponent_SetFlipX_Ptr;
    void* SpriteComponent_GetFlipY_Ptr;
    void* SpriteComponent_SetFlipY_Ptr;
    void* SpriteComponent_GetZOrder_Ptr;
    void* SpriteComponent_SetZOrder_Ptr;
    void* ButtonControl_IsClicked_Ptr;
    void* ButtonControl_IsDown_Ptr;
    void* CheckboxControl_GetChecked_Ptr;
    void* ComboBoxControl_GetSelectedIndex_Ptr;
    void* ComboBoxControl_SetSelectedIndex_Ptr;
    void* ComboBoxControl_AddItem_Ptr;
    void* ComboBoxControl_ClearItems_Ptr;
    void* ComboBoxControl_GetItemCount_Ptr;
    void* ComboBoxControl_GetItem_Ptr;
    void* SpawnComponent_IsActive_Ptr;
    void* SpawnComponent_GetSpawnPoint_Ptr;
    void* SceneTransitionComponent_GetTargetScene_Ptr;
    void* Shader_SetFloat_Ptr;
    void* Shader_SetVec3_Ptr;
    void* Shader_GetEnabled_Ptr;
    void* Shader_SetEnabled_Ptr;
    void* NetworkIdentity_GetNetworkID_Ptr;
    void* NetworkIdentity_IsOwned_Ptr;
    void* Input_IsKeyDown_Ptr;
    void* Input_IsKeyPressed_Ptr;
    void* Input_IsKeyReleased_Ptr;
    void* Input_IsMouseButtonDown_Ptr;
    void* Input_IsMouseButtonPressed_Ptr;
    void* Input_GetMouseWheelMove_Ptr;
    void* Input_GetMouseDelta_Ptr;
    void* Log_Info_Ptr;
    void* Log_Warn_Ptr;
    void* Log_Error_Ptr;
    void* Network_Host_Ptr;
    void* Network_Connect_Ptr;
    void* Network_Disconnect_Ptr;
    void* Network_IsActive_Ptr;
    void* Network_IsServer_Ptr;
    void* Network_SendData_Ptr;
    void* Network_HasMessages_Ptr;
    void* Network_GetNextMessage_Ptr;
    void* Scene_FindEntityByTag_Ptr;
    void* Scene_LoadScene_Ptr;
    void* Scene_GetPrimaryCameraEntity_Ptr;
    void* Scene_CopyEntity_Ptr;
    void* Audio_Play_Ptr;
    void* Audio_Stop_Ptr;
    void* Audio_StopAll_Ptr;
    void* Application_Close_Ptr;
    void* Application_GetFPS_Ptr;
    void* Application_GetFrameTime_Ptr;
    void* Window_SetSize_Ptr;
    void* Window_SetFullscreen_Ptr;
    void* Window_SetVSync_Ptr;
    void* Window_SetAntialiasing_Ptr;
};

struct ChainedManagedPointers
{
    void (*ScriptEngine_OnUpdate)(float);
    void (*ScriptEngine_OnEvent)(int);
    void (*ScriptEngine_OnRenderUI)();
    void (*ScriptEngine_OnCollisionEnter)(uint64_t, uint64_t);
    void (*ScriptEngine_ClearAll)();
    void (*ScriptEngine_InstantiateScript)(uint64_t, const char16_t*);
    void (*ScriptEngine_DestroyScript)(uint64_t, const char16_t*);
};

extern ChainedManagedPointers g_ManagedPointers;

inline void FillNativePointers(ChainedNativePointers* ptrs)
{
    ptrs->Entity_HasComponent_Ptr = (void*)Entity_HasComponent;
    ptrs->Entity_FindAllWithComponent_Ptr = (void*)Entity_FindAllWithComponent;
    ptrs->Entity_AddComponent_Ptr = (void*)Entity_AddComponent;
    ptrs->Transform_GetTranslation_Ptr = (void*)Transform_GetTranslation;
    ptrs->Transform_SetTranslation_Ptr = (void*)Transform_SetTranslation;
    ptrs->Transform_GetRotation_Ptr = (void*)Transform_GetRotation;
    ptrs->Transform_SetRotation_Ptr = (void*)Transform_SetRotation;
    ptrs->Transform_GetScale_Ptr = (void*)Transform_GetScale;
    ptrs->Transform_SetScale_Ptr = (void*)Transform_SetScale;
    ptrs->Model_GetModelPath_Ptr = (void*)Model_GetModelPath;
    ptrs->Model_SetModelPath_Ptr = (void*)Model_SetModelPath;
    ptrs->RigidBody_GetVelocity_Ptr = (void*)RigidBody_GetVelocity;
    ptrs->RigidBody_SetVelocity_Ptr = (void*)RigidBody_SetVelocity;
    ptrs->RigidBody_IsGrounded_Ptr = (void*)RigidBody_IsGrounded;
    ptrs->RigidBody_IsKinematic_Ptr = (void*)RigidBody_IsKinematic;
    ptrs->RigidBody_SetKinematic_Ptr = (void*)RigidBody_SetKinematic;
    ptrs->TagComponent_GetTag_Ptr = (void*)TagComponent_GetTag;
    ptrs->Camera_GetForward_Ptr = (void*)Camera_GetForward;
    ptrs->Camera_GetRight_Ptr = (void*)Camera_GetRight;
    ptrs->Camera_GetOrbit_Ptr = (void*)Camera_GetOrbit;
    ptrs->Camera_SetOrbit_Ptr = (void*)Camera_SetOrbit;
    ptrs->Camera_GetPrimary_Ptr = (void*)Camera_GetPrimary;
    ptrs->Camera_SetPrimary_Ptr = (void*)Camera_SetPrimary;
    ptrs->Camera_GetIsOrbit_Ptr = (void*)Camera_GetIsOrbit;
    ptrs->Camera_SetIsOrbit_Ptr = (void*)Camera_SetIsOrbit;
    ptrs->Camera_GetTargetTag_Ptr = (void*)Camera_GetTargetTag;
    ptrs->Camera_SetTargetTag_Ptr = (void*)Camera_SetTargetTag;
    ptrs->AudioComponent_SetVolume_Ptr = (void*)AudioComponent_SetVolume;
    ptrs->AudioComponent_SetLoop_Ptr = (void*)AudioComponent_SetLoop;
    ptrs->AudioComponent_IsPlaying_Ptr = (void*)AudioComponent_IsPlaying;
    ptrs->AudioComponent_GetSoundPath_Ptr = (void*)AudioComponent_GetSoundPath;
    ptrs->AudioComponent_Play_Ptr = (void*)AudioComponent_Play;
    ptrs->AudioComponent_Stop_Ptr = (void*)AudioComponent_Stop;
    ptrs->SpriteComponent_GetTexturePath_Ptr = (void*)SpriteComponent_GetTexturePath;
    ptrs->SpriteComponent_SetTexturePath_Ptr = (void*)SpriteComponent_SetTexturePath;
    ptrs->SpriteComponent_GetTint_Ptr = (void*)SpriteComponent_GetTint;
    ptrs->SpriteComponent_SetTint_Ptr = (void*)SpriteComponent_SetTint;
    ptrs->SpriteComponent_GetFlipX_Ptr = (void*)SpriteComponent_GetFlipX;
    ptrs->SpriteComponent_SetFlipX_Ptr = (void*)SpriteComponent_SetFlipX;
    ptrs->SpriteComponent_GetFlipY_Ptr = (void*)SpriteComponent_GetFlipY;
    ptrs->SpriteComponent_SetFlipY_Ptr = (void*)SpriteComponent_SetFlipY;
    ptrs->SpriteComponent_GetZOrder_Ptr = (void*)SpriteComponent_GetZOrder;
    ptrs->SpriteComponent_SetZOrder_Ptr = (void*)SpriteComponent_SetZOrder;
    ptrs->ButtonControl_IsClicked_Ptr = (void*)ButtonControl_IsClicked;
    ptrs->ButtonControl_IsDown_Ptr = (void*)ButtonControl_IsDown;
    ptrs->CheckboxControl_GetChecked_Ptr = (void*)CheckboxControl_GetChecked;
    ptrs->ComboBoxControl_GetSelectedIndex_Ptr = (void*)ComboBoxControl_GetSelectedIndex;
    ptrs->ComboBoxControl_SetSelectedIndex_Ptr = (void*)ComboBoxControl_SetSelectedIndex;
    ptrs->ComboBoxControl_AddItem_Ptr = (void*)ComboBoxControl_AddItem;
    ptrs->ComboBoxControl_ClearItems_Ptr = (void*)ComboBoxControl_ClearItems;
    ptrs->ComboBoxControl_GetItemCount_Ptr = (void*)ComboBoxControl_GetItemCount;
    ptrs->ComboBoxControl_GetItem_Ptr = (void*)ComboBoxControl_GetItem;
    ptrs->Shader_SetFloat_Ptr = (void*)Shader_SetFloat;
    ptrs->Shader_SetVec3_Ptr = (void*)Shader_SetVec3;
    ptrs->Shader_GetEnabled_Ptr = (void*)Shader_GetEnabled;
    ptrs->Shader_SetEnabled_Ptr = (void*)Shader_SetEnabled;
    ptrs->Input_IsKeyDown_Ptr = (void*)Input_IsKeyDown;
    ptrs->Input_IsKeyPressed_Ptr = (void*)Input_IsKeyPressed;
    ptrs->Input_IsKeyReleased_Ptr = (void*)Input_IsKeyReleased;
    ptrs->Input_IsMouseButtonDown_Ptr = (void*)Input_IsMouseButtonDown;
    ptrs->Input_IsMouseButtonPressed_Ptr = (void*)Input_IsMouseButtonPressed;
    ptrs->Input_GetMouseWheelMove_Ptr = (void*)Input_GetMouseWheelMove;
    ptrs->Input_GetMouseDelta_Ptr = (void*)Input_GetMouseDelta;
    ptrs->Log_Info_Ptr = (void*)Log_Info;
    ptrs->Log_Warn_Ptr = (void*)Log_Warn;
    ptrs->Log_Error_Ptr = (void*)Log_Error;
    ptrs->Scene_FindEntityByTag_Ptr = (void*)Scene_FindEntityByTag;
    ptrs->Scene_LoadScene_Ptr = (void*)Scene_LoadScene;
    ptrs->Scene_GetPrimaryCameraEntity_Ptr = (void*)Scene_GetPrimaryCameraEntity;
    ptrs->Scene_CopyEntity_Ptr = (void*)Scene_CopyEntity;
    ptrs->Audio_Play_Ptr = (void*)Audio_Play;
    ptrs->Audio_Stop_Ptr = (void*)Audio_Stop;
    ptrs->Audio_StopAll_Ptr = (void*)Audio_StopAll;
    ptrs->Application_Close_Ptr = (void*)Application_Close;
    ptrs->Application_GetFPS_Ptr = (void*)Application_GetFPS;
    ptrs->Application_GetFrameTime_Ptr = (void*)Application_GetFrameTime;
    ptrs->Window_SetSize_Ptr = (void*)Window_SetSize;
    ptrs->Window_SetFullscreen_Ptr = (void*)Window_SetFullscreen;
    ptrs->Window_SetVSync_Ptr = (void*)Window_SetVSync;
    ptrs->Window_SetAntialiasing_Ptr = (void*)Window_SetAntialiasing;
}

CH_SCRIPT_FUNC inline void Native_BypassInit(ChainedNativePointers* outNative, ChainedManagedPointers* inManaged)
{
    FillNativePointers(outNative);
    g_ManagedPointers = *inManaged;
}

CH_ADD_INTERNAL_CALL(Interop, Native_BypassInit, Native_BypassInit);


extern ChainedManagedPointers g_ManagedPointers;

} // namespace Chained
#endif // CH_SCRIPT_INTEROP_POINTERS_H
