#include "script_glue.h"
#include "script_interop_pointers.h"
#include "script_internal_call_registry.h"
#include "engine/core/log.h" 


#include "script_glue_internal.h"
#include "script_glue_system.h"
#include "script_glue_scene.h"
#include "script_glue_entity.h"
#include "script_glue_camera.h"
#include "script_glue_ui.h"
#include "script_glue_audio.h"
#include "script_glue_input.h"


namespace Chained
{
    
    extern ChainedManagedPointers g_ManagedPointers; 

    
    void ScriptGlue::Initialize()
    {
        // No longer relying on explicit CH_ADD_INTERNAL_CALL for each API endpoint.
        // We solely rely on Native_BypassInit to exchange pointer structs!
    }

    
    void ScriptGlue::FillNativePointers(ChainedNativePointers* ptrs)
    {
        if (!ptrs) return;

        // Entity & Components
        ptrs->Entity_HasComponent_Ptr            = (void*)Entity_HasComponent;
        ptrs->Entity_FindAllWithComponent_Ptr    = (void*)Entity_FindAllWithComponent;
        ptrs->Entity_AddComponent_Ptr            = (void*)Entity_AddComponent;
        
        // Transform
        ptrs->Transform_GetTranslation_Ptr       = (void*)Transform_GetTranslation;
        ptrs->Transform_SetTranslation_Ptr       = (void*)Transform_SetTranslation;
        ptrs->Transform_GetRotation_Ptr          = (void*)Transform_GetRotation;
        ptrs->Transform_SetRotation_Ptr          = (void*)Transform_SetRotation;
        ptrs->Transform_GetScale_Ptr             = (void*)Transform_GetScale;
        ptrs->Transform_SetScale_Ptr             = (void*)Transform_SetScale;
        
        // Model
        ptrs->Model_GetModelPath_Ptr             = (void*)Model_GetModelPath;
        ptrs->Model_SetModelPath_Ptr             = (void*)Model_SetModelPath;
        
        // RigidBody
        ptrs->RigidBody_GetVelocity_Ptr          = (void*)RigidBody_GetVelocity;
        ptrs->RigidBody_SetVelocity_Ptr          = (void*)RigidBody_SetVelocity;
        ptrs->RigidBody_IsGrounded_Ptr           = (void*)RigidBody_IsGrounded;
        ptrs->RigidBody_IsKinematic_Ptr          = (void*)RigidBody_IsKinematic;
        ptrs->RigidBody_SetKinematic_Ptr         = (void*)RigidBody_SetKinematic;
        
        // Tags & Camera
        ptrs->TagComponent_GetTag_Ptr            = (void*)TagComponent_GetTag;
        ptrs->Camera_GetForward_Ptr              = (void*)Camera_GetForward;
        ptrs->Camera_GetRight_Ptr                = (void*)Camera_GetRight;
        ptrs->Camera_GetOrbit_Ptr                = (void*)Camera_GetOrbit;
        ptrs->Camera_SetOrbit_Ptr                = (void*)Camera_SetOrbit;
        ptrs->Camera_GetPrimary_Ptr              = (void*)Camera_GetPrimary;
        ptrs->Camera_SetPrimary_Ptr              = (void*)Camera_SetPrimary;
        ptrs->Camera_GetIsOrbit_Ptr              = (void*)Camera_GetIsOrbit;
        ptrs->Camera_SetIsOrbit_Ptr              = (void*)Camera_SetIsOrbit;
        ptrs->Camera_GetTargetTag_Ptr            = (void*)Camera_GetTargetTag;
        ptrs->Camera_SetTargetTag_Ptr            = (void*)Camera_SetTargetTag;
        
        // Audio Component
        ptrs->AudioComponent_SetVolume_Ptr       = (void*)AudioComponent_SetVolume;
        ptrs->AudioComponent_SetLoop_Ptr         = (void*)AudioComponent_SetLoop;
        ptrs->AudioComponent_IsPlaying_Ptr       = (void*)AudioComponent_IsPlaying;
        ptrs->AudioComponent_GetSoundPath_Ptr    = (void*)AudioComponent_GetSoundPath;
        ptrs->AudioComponent_Play_Ptr            = (void*)AudioComponent_Play;
        ptrs->AudioComponent_Stop_Ptr            = (void*)AudioComponent_Stop;
        
        // Sprite Component
        ptrs->SpriteComponent_GetTexturePath_Ptr = (void*)SpriteComponent_GetTexturePath;
        ptrs->SpriteComponent_SetTexturePath_Ptr = (void*)SpriteComponent_SetTexturePath;
        ptrs->SpriteComponent_GetTint_Ptr        = (void*)SpriteComponent_GetTint;
        ptrs->SpriteComponent_SetTint_Ptr        = (void*)SpriteComponent_SetTint;
        ptrs->SpriteComponent_GetFlipX_Ptr       = (void*)SpriteComponent_GetFlipX;
        ptrs->SpriteComponent_SetFlipX_Ptr       = (void*)SpriteComponent_SetFlipX;
        ptrs->SpriteComponent_GetFlipY_Ptr       = (void*)SpriteComponent_GetFlipY;
        ptrs->SpriteComponent_SetFlipY_Ptr       = (void*)SpriteComponent_SetFlipY;
        ptrs->SpriteComponent_GetZOrder_Ptr      = (void*)SpriteComponent_GetZOrder;
        ptrs->SpriteComponent_SetZOrder_Ptr      = (void*)SpriteComponent_SetZOrder;
        
        // UI Controls
        ptrs->ButtonControl_IsClicked_Ptr        = (void*)ButtonControl_IsClicked;
        ptrs->ButtonControl_IsDown_Ptr           = (void*)ButtonControl_IsDown;
        ptrs->CheckboxControl_GetChecked_Ptr     = (void*)CheckboxControl_GetChecked;
        ptrs->ComboBoxControl_GetSelectedIndex_Ptr = (void*)ComboBoxControl_GetSelectedIndex;
        ptrs->ComboBoxControl_SetSelectedIndex_Ptr = (void*)ComboBoxControl_SetSelectedIndex;
        ptrs->ComboBoxControl_AddItem_Ptr        = (void*)ComboBoxControl_AddItem;
        ptrs->ComboBoxControl_ClearItems_Ptr     = (void*)ComboBoxControl_ClearItems;
        ptrs->ComboBoxControl_GetItemCount_Ptr   = (void*)ComboBoxControl_GetItemCount;
        ptrs->ComboBoxControl_GetItem_Ptr        = (void*)ComboBoxControl_GetItem;
        
        // Shaders
        ptrs->Shader_SetFloat_Ptr                = (void*)Shader_SetFloat;
        ptrs->Shader_SetVec3_Ptr                 = (void*)Shader_SetVec3;
        ptrs->Shader_GetEnabled_Ptr              = (void*)Shader_GetEnabled;
        ptrs->Shader_SetEnabled_Ptr              = (void*)Shader_SetEnabled;
        
        // Input Subsystem
        ptrs->Input_IsKeyDown_Ptr                = (void*)Input_IsKeyDown;
        ptrs->Input_IsKeyPressed_Ptr             = (void*)Input_IsKeyPressed;
        ptrs->Input_IsKeyReleased_Ptr            = (void*)Input_IsKeyReleased;
        ptrs->Input_IsMouseButtonDown_Ptr        = (void*)Input_IsMouseButtonDown;
        ptrs->Input_IsMouseButtonPressed_Ptr     = (void*)Input_IsMouseButtonPressed;
        ptrs->Input_GetMouseWheelMove_Ptr        = (void*)Input_GetMouseWheelMove;
        ptrs->Input_GetMouseDelta_Ptr            = (void*)Input_GetMouseDelta;
        
        // Engine Systems
        ptrs->Log_Info_Ptr                       = (void*)Log_Info;
        ptrs->Log_Warn_Ptr                       = (void*)Log_Warn;
        ptrs->Log_Error_Ptr                      = (void*)Log_Error;
        
        ptrs->Scene_FindEntityByTag_Ptr          = (void*)Scene_FindEntityByTag;
        ptrs->Scene_LoadScene_Ptr                = (void*)Scene_LoadScene;
        ptrs->Scene_GetPrimaryCameraEntity_Ptr   = (void*)Scene_GetPrimaryCameraEntity;
        ptrs->Scene_CopyEntity_Ptr               = (void*)Scene_CopyEntity;
        
        ptrs->Audio_Play_Ptr                     = (void*)Audio_Play;
        ptrs->Audio_Stop_Ptr                     = (void*)Audio_Stop;
        ptrs->Audio_StopAll_Ptr                  = (void*)Audio_StopAll;
        
        ptrs->Application_Close_Ptr              = (void*)Application_Close;
        ptrs->Application_GetFPS_Ptr             = (void*)Application_GetFPS;
        ptrs->Application_GetFrameTime_Ptr       = (void*)Application_GetFrameTime;
        
        ptrs->Window_SetSize_Ptr                 = (void*)Window_SetSize;
        ptrs->Window_SetFullscreen_Ptr           = (void*)Window_SetFullscreen;
        ptrs->Window_SetVSync_Ptr                = (void*)Window_SetVSync;
        ptrs->Window_SetAntialiasing_Ptr         = (void*)Window_SetAntialiasing;
    }

    
    void ScriptGlue::Native_BypassInit(void* outNative, void* inManaged) 
    {
        ChainedNativePointers* n = (ChainedNativePointers*)outNative;
        ChainedManagedPointers* m = (ChainedManagedPointers*)inManaged;

        ScriptGlue::FillNativePointers(n);
        if (m)
        {
            g_ManagedPointers = *m;
        }
    }

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly)
    {
        
        if (assembly.GetLocalType("Chained.Interop"))
        {
            assembly.AddInternalCall("Chained.Interop", "Native_BypassInit", (void*)&Native_BypassInit);
            assembly.UploadInternalCalls();
            CH_CORE_INFO("ScriptEngine: Registered Native_BypassInit successfully.");
        }
    }
} // namespace Chained