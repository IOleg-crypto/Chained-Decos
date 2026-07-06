using System;
using System.Runtime.InteropServices;

namespace Chained
{
    
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct NativeString
    {
        public char* Buffer;
        public uint Length;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ChainedNativePointers 
    {
        public delegate* unmanaged<ulong, char*, bool> Entity_HasComponent_Ptr;
        public delegate* unmanaged<char*, ulong*, int, int> Entity_FindAllWithComponent_Ptr;
        public delegate* unmanaged<ulong, char*, void> Entity_AddComponent_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Transform_GetTranslation_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Transform_SetTranslation_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Transform_GetRotation_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Transform_SetRotation_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Transform_GetScale_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Transform_SetScale_Ptr;
        public delegate* unmanaged<ulong, char*> Model_GetModelPath_Ptr;
        public delegate* unmanaged<ulong, char*, void> Model_SetModelPath_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> RigidBody_GetVelocity_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> RigidBody_SetVelocity_Ptr;
        public delegate* unmanaged<ulong, bool> RigidBody_IsGrounded_Ptr;
        public delegate* unmanaged<ulong, bool> RigidBody_IsKinematic_Ptr;
        public delegate* unmanaged<ulong, bool, void> RigidBody_SetKinematic_Ptr;
        public delegate* unmanaged<ulong, char*> TagComponent_GetTag_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Camera_GetForward_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> Camera_GetRight_Ptr;
        public delegate* unmanaged<ulong, float*, float*, float*, void> Camera_GetOrbit_Ptr;
        public delegate* unmanaged<ulong, float, float, float, void> Camera_SetOrbit_Ptr;
        public delegate* unmanaged<ulong, bool> Camera_GetPrimary_Ptr;
        public delegate* unmanaged<ulong, bool, void> Camera_SetPrimary_Ptr;
        public delegate* unmanaged<ulong, bool> Camera_GetIsOrbit_Ptr;
        public delegate* unmanaged<ulong, bool, void> Camera_SetIsOrbit_Ptr;
        public delegate* unmanaged<ulong, char*> Camera_GetTargetTag_Ptr;
        public delegate* unmanaged<ulong, char*, void> Camera_SetTargetTag_Ptr;
        public delegate* unmanaged<ulong, float> PlayerComponent_GetMovementSpeed_Ptr;
        public delegate* unmanaged<ulong, float, void> PlayerComponent_SetMovementSpeed_Ptr;
        public delegate* unmanaged<ulong, float> PlayerComponent_GetJumpForce_Ptr;
        public delegate* unmanaged<ulong, float, void> PlayerComponent_SetJumpForce_Ptr;
        public delegate* unmanaged<ulong, float> PlayerComponent_GetLookSensitivity_Ptr;
        public delegate* unmanaged<ulong, float, void> PlayerComponent_SetLookSensitivity_Ptr;
        public delegate* unmanaged<ulong, float, void> AudioComponent_SetVolume_Ptr;
        public delegate* unmanaged<ulong, bool, void> AudioComponent_SetLoop_Ptr;
        public delegate* unmanaged<ulong, bool> AudioComponent_IsPlaying_Ptr;
        public delegate* unmanaged<ulong, char*> AudioComponent_GetSoundPath_Ptr;
        public delegate* unmanaged<ulong, void> AudioComponent_Play_Ptr;
        public delegate* unmanaged<ulong, void> AudioComponent_Stop_Ptr;
        public delegate* unmanaged<ulong, char*> SpriteComponent_GetTexturePath_Ptr;
        public delegate* unmanaged<ulong, char*, void> SpriteComponent_SetTexturePath_Ptr;
        public delegate* unmanaged<ulong, Vector4*, void> SpriteComponent_GetTint_Ptr;
        public delegate* unmanaged<ulong, Vector4, void> SpriteComponent_SetTint_Ptr;
        public delegate* unmanaged<ulong, bool> SpriteComponent_GetFlipX_Ptr;
        public delegate* unmanaged<ulong, bool, void> SpriteComponent_SetFlipX_Ptr;
        public delegate* unmanaged<ulong, bool> SpriteComponent_GetFlipY_Ptr;
        public delegate* unmanaged<ulong, bool, void> SpriteComponent_SetFlipY_Ptr;
        public delegate* unmanaged<ulong, int> SpriteComponent_GetZOrder_Ptr;
        public delegate* unmanaged<ulong, int, void> SpriteComponent_SetZOrder_Ptr;
        public delegate* unmanaged<ulong, bool> ButtonControl_IsClicked_Ptr;
        public delegate* unmanaged<ulong, bool> ButtonControl_IsDown_Ptr;
        public delegate* unmanaged<ulong, bool> CheckboxControl_GetChecked_Ptr;
        public delegate* unmanaged<ulong, int> ComboBoxControl_GetSelectedIndex_Ptr;
        public delegate* unmanaged<ulong, int, void> ComboBoxControl_SetSelectedIndex_Ptr;
        public delegate* unmanaged<ulong, char*, void> ComboBoxControl_AddItem_Ptr;
        public delegate* unmanaged<ulong, void> ComboBoxControl_ClearItems_Ptr;
        public delegate* unmanaged<ulong, int> ComboBoxControl_GetItemCount_Ptr;
        public delegate* unmanaged<ulong, int, char*> ComboBoxControl_GetItem_Ptr;
        public delegate* unmanaged<ulong, bool> SpawnComponent_IsActive_Ptr;
        public delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_GetSpawnPoint_Ptr;
        public delegate* unmanaged<ulong, char*> SceneTransitionComponent_GetTargetScene_Ptr;
        public delegate* unmanaged<ulong, char*, float, void> Shader_SetFloat_Ptr;
        public delegate* unmanaged<ulong, char*, Vector3*, void> Shader_SetVec3_Ptr;
        public delegate* unmanaged<ulong, bool> Shader_GetEnabled_Ptr;
        public delegate* unmanaged<ulong, bool, void> Shader_SetEnabled_Ptr;
        public delegate* unmanaged<ulong, ulong> NetworkIdentity_GetNetworkID_Ptr;
        public delegate* unmanaged<ulong, bool> NetworkIdentity_IsOwned_Ptr;
        public delegate* unmanaged<int, bool> Input_IsKeyDown_Ptr;
        public delegate* unmanaged<int, bool> Input_IsKeyPressed_Ptr;
        public delegate* unmanaged<int, bool> Input_IsKeyReleased_Ptr;
        public delegate* unmanaged<int, bool> Input_IsMouseButtonDown_Ptr;
        public delegate* unmanaged<int, bool> Input_IsMouseButtonPressed_Ptr;
        public delegate* unmanaged<float> Input_GetMouseWheelMove_Ptr;
        public delegate* unmanaged<Vector3*, void> Input_GetMouseDelta_Ptr;
        public delegate* unmanaged<char*, void> Log_Info_Ptr;
        public delegate* unmanaged<char*, void> Log_Warn_Ptr;
        public delegate* unmanaged<char*, void> Log_Error_Ptr;
        public delegate* unmanaged<ushort, bool> Network_Host_Ptr;
        public delegate* unmanaged<char*, bool> Network_Connect_Ptr;
        public delegate* unmanaged<void> Network_Disconnect_Ptr;
        public delegate* unmanaged<bool> Network_IsActive_Ptr;
        public delegate* unmanaged<bool> Network_IsServer_Ptr;
        public delegate* unmanaged<byte*, uint, bool, bool> Network_SendData_Ptr;
        public delegate* unmanaged<bool> Network_HasMessages_Ptr;
        
        
        public delegate* unmanaged<byte*, uint, uint> Network_GetNextMessage_Ptr; 
        
        public delegate* unmanaged<char*, ulong> Scene_FindEntityByTag_Ptr;
        public delegate* unmanaged<char*, void> Scene_LoadScene_Ptr;
        public delegate* unmanaged<ulong> Scene_GetPrimaryCameraEntity_Ptr;
        public delegate* unmanaged<ulong, ulong> Scene_CopyEntity_Ptr;
        public delegate* unmanaged<char*, float, float, bool, void> Audio_Play_Ptr;
        public delegate* unmanaged<char*, void> Audio_Stop_Ptr;
        public delegate* unmanaged<void> Audio_StopAll_Ptr;
        public delegate* unmanaged<void> Application_Close_Ptr;
        public delegate* unmanaged<int> Application_GetFPS_Ptr;
        public delegate* unmanaged<float> Application_GetFrameTime_Ptr;
        public delegate* unmanaged<int, int, void> Window_SetSize_Ptr;
        public delegate* unmanaged<bool, void> Window_SetFullscreen_Ptr;
        public delegate* unmanaged<bool, void> Window_SetVSync_Ptr;
        public delegate* unmanaged<bool, void> Window_SetAntialiasing_Ptr;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ChainedManagedPointers 
    {
        public delegate* unmanaged<float, void> ScriptEngine_OnUpdate;
        public delegate* unmanaged<int, void> ScriptEngine_OnEvent;
        public delegate* unmanaged<void> ScriptEngine_OnRenderUI;
        public delegate* unmanaged<ulong, ulong, void> ScriptEngine_OnCollisionEnter;
        public delegate* unmanaged<void> ScriptEngine_ClearAll;
        public delegate* unmanaged<ulong, char*, void> ScriptEngine_InstantiateScript;
        public delegate* unmanaged<ulong, char*, void> ScriptEngine_DestroyScript;
    }

    public static unsafe class Interop
    {
        internal static delegate* unmanaged<void*, void*, void> Native_BypassInit;

        public static void InitInterop()
        {
            ChainedNativePointers np = new ChainedNativePointers();
            ChainedManagedPointers mp = new ChainedManagedPointers();

            
            mp.ScriptEngine_OnUpdate = &ScriptEngine.OnUpdate;
            mp.ScriptEngine_OnEvent = &ScriptEngine.OnEvent;
            mp.ScriptEngine_OnRenderUI = &ScriptEngine.OnRenderUI;
            mp.ScriptEngine_OnCollisionEnter = &ScriptEngine.OnCollisionEnter;
            mp.ScriptEngine_ClearAll = &ScriptEngine.ClearAll;
            mp.ScriptEngine_InstantiateScript = &ScriptEngine.InstantiateScript;
            mp.ScriptEngine_DestroyScript = &ScriptEngine.DestroyScript;

            
            Native_BypassInit(&np, &mp);

            
            Entity.Entity_HasComponent_Ptr = np.Entity_HasComponent_Ptr;
            Entity.Entity_FindAllWithComponent_Ptr = np.Entity_FindAllWithComponent_Ptr;
            Entity.Entity_AddComponent_Ptr = np.Entity_AddComponent_Ptr;
            TransformComponent.Transform_GetTranslation_Ptr = np.Transform_GetTranslation_Ptr;
            TransformComponent.Transform_SetTranslation_Ptr = np.Transform_SetTranslation_Ptr;
            TransformComponent.Transform_GetRotation_Ptr = np.Transform_GetRotation_Ptr;
            TransformComponent.Transform_SetRotation_Ptr = np.Transform_SetRotation_Ptr;
            TransformComponent.Transform_GetScale_Ptr = np.Transform_GetScale_Ptr;
            TransformComponent.Transform_SetScale_Ptr = np.Transform_SetScale_Ptr;
            ModelComponent.Model_GetModelPath_Ptr = np.Model_GetModelPath_Ptr;
            ModelComponent.Model_SetModelPath_Ptr = np.Model_SetModelPath_Ptr;
            RigidBodyComponent.RigidBody_GetVelocity_Ptr = np.RigidBody_GetVelocity_Ptr;
            RigidBodyComponent.RigidBody_SetVelocity_Ptr = np.RigidBody_SetVelocity_Ptr;
            RigidBodyComponent.RigidBody_IsGrounded_Ptr = np.RigidBody_IsGrounded_Ptr;
            RigidBodyComponent.RigidBody_IsKinematic_Ptr = np.RigidBody_IsKinematic_Ptr;
            RigidBodyComponent.RigidBody_SetKinematic_Ptr = np.RigidBody_SetKinematic_Ptr;
            TagComponent.TagComponent_GetTag_Ptr = np.TagComponent_GetTag_Ptr;
            CameraComponent.Camera_GetForward_Ptr = np.Camera_GetForward_Ptr;
            CameraComponent.Camera_GetRight_Ptr = np.Camera_GetRight_Ptr;
            CameraComponent.Camera_GetOrbit_Ptr = np.Camera_GetOrbit_Ptr;
            CameraComponent.Camera_SetOrbit_Ptr = np.Camera_SetOrbit_Ptr;
            CameraComponent.Camera_GetPrimary_Ptr = np.Camera_GetPrimary_Ptr;
            CameraComponent.Camera_SetPrimary_Ptr = np.Camera_SetPrimary_Ptr;
            CameraComponent.Camera_GetIsOrbit_Ptr = np.Camera_GetIsOrbit_Ptr;
            CameraComponent.Camera_SetIsOrbit_Ptr = np.Camera_SetIsOrbit_Ptr;
            CameraComponent.Camera_GetTargetTag_Ptr = np.Camera_GetTargetTag_Ptr;
            CameraComponent.Camera_SetTargetTag_Ptr = np.Camera_SetTargetTag_Ptr;
    
            AudioComponent.AudioComponent_SetVolume_Ptr = np.AudioComponent_SetVolume_Ptr;
            AudioComponent.AudioComponent_SetLoop_Ptr = np.AudioComponent_SetLoop_Ptr;
            AudioComponent.AudioComponent_IsPlaying_Ptr = np.AudioComponent_IsPlaying_Ptr;
            AudioComponent.AudioComponent_GetSoundPath_Ptr = np.AudioComponent_GetSoundPath_Ptr;
            AudioComponent.AudioComponent_Play_Ptr = np.AudioComponent_Play_Ptr;
            AudioComponent.AudioComponent_Stop_Ptr = np.AudioComponent_Stop_Ptr;
            SpriteComponent.SpriteComponent_GetTexturePath_Ptr = np.SpriteComponent_GetTexturePath_Ptr;
            SpriteComponent.SpriteComponent_SetTexturePath_Ptr = np.SpriteComponent_SetTexturePath_Ptr;
            SpriteComponent.SpriteComponent_GetTint_Ptr = np.SpriteComponent_GetTint_Ptr;
            SpriteComponent.SpriteComponent_SetTint_Ptr = np.SpriteComponent_SetTint_Ptr;
            SpriteComponent.SpriteComponent_GetFlipX_Ptr = np.SpriteComponent_GetFlipX_Ptr;
            SpriteComponent.SpriteComponent_SetFlipX_Ptr = np.SpriteComponent_SetFlipX_Ptr;
            SpriteComponent.SpriteComponent_GetFlipY_Ptr = np.SpriteComponent_GetFlipY_Ptr;
            SpriteComponent.SpriteComponent_SetFlipY_Ptr = np.SpriteComponent_SetFlipY_Ptr;
            SpriteComponent.SpriteComponent_GetZOrder_Ptr = np.SpriteComponent_GetZOrder_Ptr;
            SpriteComponent.SpriteComponent_SetZOrder_Ptr = np.SpriteComponent_SetZOrder_Ptr;
            ButtonControl.ButtonControl_IsClicked_Ptr = np.ButtonControl_IsClicked_Ptr;
            ButtonControl.ButtonControl_IsDown_Ptr = np.ButtonControl_IsDown_Ptr;
            CheckboxControl.CheckboxControl_GetChecked_Ptr = np.CheckboxControl_GetChecked_Ptr;
            ComboBoxControl.ComboBoxControl_GetSelectedIndex_Ptr = np.ComboBoxControl_GetSelectedIndex_Ptr;
            ComboBoxControl.ComboBoxControl_SetSelectedIndex_Ptr = np.ComboBoxControl_SetSelectedIndex_Ptr;
            ComboBoxControl.ComboBoxControl_AddItem_Ptr = np.ComboBoxControl_AddItem_Ptr;
            ComboBoxControl.ComboBoxControl_ClearItems_Ptr = np.ComboBoxControl_ClearItems_Ptr;
            ComboBoxControl.ComboBoxControl_GetItemCount_Ptr = np.ComboBoxControl_GetItemCount_Ptr;
            ComboBoxControl.ComboBoxControl_GetItem_Ptr = np.ComboBoxControl_GetItem_Ptr;
            ShaderComponent.Shader_SetFloat_Ptr = np.Shader_SetFloat_Ptr;
            ShaderComponent.Shader_SetVec3_Ptr = np.Shader_SetVec3_Ptr;
            ShaderComponent.Shader_GetEnabled_Ptr = np.Shader_GetEnabled_Ptr;
            ShaderComponent.Shader_SetEnabled_Ptr = np.Shader_SetEnabled_Ptr;
            NetworkIdentity.NetworkIdentity_GetNetworkID_Ptr = np.NetworkIdentity_GetNetworkID_Ptr;
            NetworkIdentity.NetworkIdentity_IsOwned_Ptr = np.NetworkIdentity_IsOwned_Ptr;
            Input.Input_IsKeyDown_Ptr = np.Input_IsKeyDown_Ptr;
            Input.Input_IsKeyPressed_Ptr = np.Input_IsKeyPressed_Ptr;
            Input.Input_IsKeyReleased_Ptr = np.Input_IsKeyReleased_Ptr;
            Input.Input_IsMouseButtonDown_Ptr = np.Input_IsMouseButtonDown_Ptr;
            Input.Input_IsMouseButtonPressed_Ptr = np.Input_IsMouseButtonPressed_Ptr;
            Input.Input_GetMouseWheelMove_Ptr = np.Input_GetMouseWheelMove_Ptr;
            Input.Input_GetMouseDelta_Ptr = np.Input_GetMouseDelta_Ptr;
            Log.Log_Info_Ptr = np.Log_Info_Ptr;
            Log.Log_Warn_Ptr = np.Log_Warn_Ptr;
            Log.Log_Error_Ptr = np.Log_Error_Ptr;
            Network.Network_Host_Ptr = np.Network_Host_Ptr;
            Network.Network_Connect_Ptr = np.Network_Connect_Ptr;
            Network.Network_Disconnect_Ptr = np.Network_Disconnect_Ptr;
            Network.Network_IsActive_Ptr = np.Network_IsActive_Ptr;
            Network.Network_IsServer_Ptr = np.Network_IsServer_Ptr;
            Network.Network_SendData_Ptr = np.Network_SendData_Ptr;
            Network.Network_HasMessages_Ptr = np.Network_HasMessages_Ptr;
        
            Scene.Scene_FindEntityByTag_Ptr = np.Scene_FindEntityByTag_Ptr;
            Scene.Scene_LoadScene_Ptr = np.Scene_LoadScene_Ptr;
            Scene.Scene_GetPrimaryCameraEntity_Ptr = np.Scene_GetPrimaryCameraEntity_Ptr;
            Scene.Scene_CopyEntity_Ptr = np.Scene_CopyEntity_Ptr;
            Audio.Audio_Play_Ptr = np.Audio_Play_Ptr;
            Audio.Audio_Stop_Ptr = np.Audio_Stop_Ptr;
            Audio.Audio_StopAll_Ptr = np.Audio_StopAll_Ptr;
            Application.Application_Close_Ptr = np.Application_Close_Ptr;
            Application.Application_GetFPS_Ptr = np.Application_GetFPS_Ptr;
            Application.Application_GetFrameTime_Ptr = np.Application_GetFrameTime_Ptr;
            AppWindow.Window_SetSize_Ptr = np.Window_SetSize_Ptr;
            AppWindow.Window_SetFullscreen_Ptr = np.Window_SetFullscreen_Ptr;
            AppWindow.Window_SetVSync_Ptr = np.Window_SetVSync_Ptr;
            AppWindow.Window_SetAntialiasing_Ptr = np.Window_SetAntialiasing_Ptr;
        }
    }
}