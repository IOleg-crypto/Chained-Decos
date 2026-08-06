using System;
using System.Runtime.InteropServices;
using Coral.Managed.Interop;

namespace Chained
{

/// <summary>Model/Mesh wrapper.</summary>
public class ModelComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*> Model_GetModelPath_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> Model_SetModelPath_Ptr;
#pragma warning restore 0649

    public string ModelPath
    {
        get { unsafe { string? result = Marshal.PtrToStringUni(new IntPtr(Model_GetModelPath_Ptr(Entity.ID))); return result ?? string.Empty; } }
        set { unsafe { if (Model_SetModelPath_Ptr != null) fixed (char* ptr = value) Model_SetModelPath_Ptr(Entity.ID, ptr); } }
    }
}

/// <summary>Rigid-body wrapper.</summary>
public class RigidBodyComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> RigidBody_GetVelocity_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> RigidBody_SetVelocity_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> RigidBody_ForceSetVelocity_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> RigidBody_IsGrounded_Ptr;
    internal static unsafe delegate* unmanaged<ulong, uint> RigidBody_IsKinematic_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody_SetKinematic_Ptr;
#pragma warning restore 0649

    public Vector3 Velocity
    {
        get { unsafe { Vector3 velocity; RigidBody_GetVelocity_Ptr(Entity.ID, &velocity); return velocity; } }
        set { unsafe { RigidBody_SetVelocity_Ptr(Entity.ID, &value); } }
    }

    /// <summary>
    /// Sets velocity unconditionally, bypassing the Y-velocity override
    /// that the normal Velocity setter applies for Dynamic bodies.
    /// Use this to truly zero velocity (e.g. after a respawn teleport).
    /// </summary>
    public void ForceSetVelocity(Vector3 velocity)
    {
        unsafe { RigidBody_ForceSetVelocity_Ptr(Entity.ID, &velocity); }
    }

    public bool IsGrounded { get { unsafe { return RigidBody_IsGrounded_Ptr(Entity.ID) != 0; } } }

    public bool IsKinematic
    {
        get { unsafe { return RigidBody_IsKinematic_Ptr(Entity.ID) != 0; } }
        set { unsafe { RigidBody_SetKinematic_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Tag wrapper.</summary>
public class TagComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*> TagComponent_GetTag_Ptr;
#pragma warning restore 0649

    private static unsafe string GetTag_Native(ulong entityID) => Marshal.PtrToStringUni(new IntPtr(TagComponent_GetTag_Ptr(entityID))) ?? string.Empty;
    public string Tag => GetTag_Native(Entity.ID);
}

/// <summary>Camera wrapper.</summary>
public class CameraComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Camera_GetForward_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Camera_GetRight_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float*, float*, float*, void> Camera_GetOrbit_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, float, float, void> Camera_SetOrbit_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> Camera_GetPrimary_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> Camera_SetPrimary_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> Camera_GetIsOrbit_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> Camera_SetIsOrbit_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*> Camera_GetTargetTag_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> Camera_SetTargetTag_Ptr;
#pragma warning restore 0649

    private static void GetForward(ulong entityID, out Vector3 outForward)
    {
        unsafe { fixed (Vector3* p = &outForward) Camera_GetForward_Ptr(entityID, p); }
    }

    private static void GetRight(ulong entityID, out Vector3 outRight)
    {
        unsafe { fixed (Vector3* p = &outRight) Camera_GetRight_Ptr(entityID, p); }
    }

    private static void GetOrbit(ulong entityID, out float yaw, out float pitch, out float distance)
    {
        unsafe { fixed (float* yawPtr = &yaw, pitchPtr = &pitch, distancePtr = &distance) Camera_GetOrbit_Ptr(entityID, yawPtr, pitchPtr, distancePtr); }
    }

    private static void SetOrbit(ulong entityID, float yaw, float pitch, float distance)
    {
        unsafe { Camera_SetOrbit_Ptr(entityID, yaw, pitch, distance); }
    }

    public Vector3 Forward
    {
        get { GetForward(Entity.ID, out Vector3 forward); return forward; }
    }

    public Vector3 Right
    {
        get { GetRight(Entity.ID, out Vector3 right); return right; }
    }

    public void GetOrbit(out float yaw, out float pitch, out float distance)
        => GetOrbit(Entity.ID, out yaw, out pitch, out distance);

    public void SetOrbit(float yaw, float pitch, float distance)
        => SetOrbit(Entity.ID, yaw, pitch, distance);

    public bool Primary
    {
        get { unsafe { return Camera_GetPrimary_Ptr(Entity.ID) != 0; } }
        set { unsafe { Camera_SetPrimary_Ptr(Entity.ID, value); } }
    }

    public bool IsOrbitCamera
    {
        get { unsafe { return Camera_GetIsOrbit_Ptr(Entity.ID) != 0; } }
        set { unsafe { Camera_SetIsOrbit_Ptr(Entity.ID, value); } }
    }

    public string TargetEntityTag
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(Camera_GetTargetTag_Ptr(Entity.ID))) ?? string.Empty; } }
        set { unsafe { if (Camera_SetTargetTag_Ptr != null) fixed (char* ptr = value) Camera_SetTargetTag_Ptr(Entity.ID, ptr); } }
    }
}

/// <summary>Audio wrapper.</summary>
public class AudioComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, float, void> AudioComponent_SetVolume_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> AudioComponent_SetLoop_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> AudioComponent_IsPlaying_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*> AudioComponent_GetSoundPath_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Play_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Stop_Ptr;
#pragma warning restore 0649

    private static unsafe void SetVolume(ulong entityID, float volume) => AudioComponent_SetVolume_Ptr(entityID, volume);
    private static unsafe void SetLoop(ulong entityID, bool loop) => AudioComponent_SetLoop_Ptr(entityID, loop);
    private static unsafe bool IsPlaying_Native(ulong entityID) => AudioComponent_IsPlaying_Ptr(entityID) != 0;
    private static unsafe string GetSoundPath(ulong entityID) => Marshal.PtrToStringUni(new IntPtr(AudioComponent_GetSoundPath_Ptr(entityID))) ?? string.Empty;

    public float Volume { set => SetVolume(Entity.ID, value); }
    public bool  Loop   { set => SetLoop(Entity.ID, value); }
    public bool  IsPlaying  => IsPlaying_Native(Entity.ID);
    public string SoundPath => GetSoundPath(Entity.ID) ?? string.Empty;

    public void Play() 
    {
        unsafe { AudioComponent_Play_Ptr(Entity.ID); }
    }

    public void Stop()
    {
        unsafe { AudioComponent_Stop_Ptr(Entity.ID); }
    }
}

/// <summary>2D Sprite wrapper.</summary>
public class SpriteComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*> SpriteComponent_GetTexturePath_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> SpriteComponent_SetTexturePath_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector4*, void> SpriteComponent_GetTint_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector4, void> SpriteComponent_SetTint_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> SpriteComponent_GetFlipX_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> SpriteComponent_SetFlipX_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> SpriteComponent_GetFlipY_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> SpriteComponent_SetFlipY_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int> SpriteComponent_GetZOrder_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, void> SpriteComponent_SetZOrder_Ptr;
#pragma warning restore 0649

    public string TexturePath
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(SpriteComponent_GetTexturePath_Ptr(Entity.ID))) ?? string.Empty; } }
        set { unsafe { if (SpriteComponent_SetTexturePath_Ptr != null) fixed (char* ptr = value) SpriteComponent_SetTexturePath_Ptr(Entity.ID, ptr); } }
    }

    public Vector4 Tint
    {
        get { unsafe { Vector4 v; SpriteComponent_GetTint_Ptr(Entity.ID, &v); return v; } }
        set { unsafe { SpriteComponent_SetTint_Ptr(Entity.ID, value); } }
    }

    public bool FlipX
    {
        get { unsafe { return SpriteComponent_GetFlipX_Ptr(Entity.ID) != 0; } }
        set { unsafe { SpriteComponent_SetFlipX_Ptr(Entity.ID, value); } }
    }

    public bool FlipY
    {
        get { unsafe { return SpriteComponent_GetFlipY_Ptr(Entity.ID) != 0; } }
        set { unsafe { SpriteComponent_SetFlipY_Ptr(Entity.ID, value); } }
    }

    public int ZOrder
    {
        get { unsafe { return SpriteComponent_GetZOrder_Ptr(Entity.ID); } }
        set { unsafe { SpriteComponent_SetZOrder_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Shader control wrapper.</summary>
public class ShaderComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*, float, void> Shader_SetFloat_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, Vector3*, void> Shader_SetVec3_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> Shader_GetEnabled_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> Shader_SetEnabled_Ptr;
#pragma warning restore 0649

    public bool Enabled
    {
        get { unsafe { return Shader_GetEnabled_Ptr(Entity.ID) != 0; } }
        set { unsafe { Shader_SetEnabled_Ptr(Entity.ID, value); } }
    }

    public unsafe void SetFloat(string name, float value)
    {
        if (Shader_SetFloat_Ptr == null) return;
        fixed (char* ptr = name) Shader_SetFloat_Ptr(Entity.ID, ptr, value);
    }

    public unsafe void SetVector3(string vname, Vector3 value)
    {
        if (Shader_SetVec3_Ptr == null) return;
        Vector3 v = value;
        fixed (char* ptr = vname) Shader_SetVec3_Ptr(Entity.ID, ptr, &v);
    }
}

/// <summary>Player component wrapper.</summary>
public class PlayerComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, float> PlayerComponent_GetMovementSpeed_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> PlayerComponent_SetMovementSpeed_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> PlayerComponent_GetJumpForce_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> PlayerComponent_SetJumpForce_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> PlayerComponent_GetLookSensitivity_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> PlayerComponent_SetLookSensitivity_Ptr;
#pragma warning restore 0649

    public float MovementSpeed
    {
        get { unsafe { return PlayerComponent_GetMovementSpeed_Ptr == null ? 0 : PlayerComponent_GetMovementSpeed_Ptr(Entity.ID); } }
        set { unsafe { if (PlayerComponent_SetMovementSpeed_Ptr != null) PlayerComponent_SetMovementSpeed_Ptr(Entity.ID, value); } }
    }

    public float JumpForce
    {
        get { unsafe { return PlayerComponent_GetJumpForce_Ptr == null ? 0 : PlayerComponent_GetJumpForce_Ptr(Entity.ID); } }
        set { unsafe { if (PlayerComponent_SetJumpForce_Ptr != null) PlayerComponent_SetJumpForce_Ptr(Entity.ID, value); } }
    }

    public float LookSensitivity
    {
        get { unsafe { return PlayerComponent_GetLookSensitivity_Ptr == null ? 0 : PlayerComponent_GetLookSensitivity_Ptr(Entity.ID); } }
        set { unsafe { if (PlayerComponent_SetLookSensitivity_Ptr != null) PlayerComponent_SetLookSensitivity_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Spawn point component.</summary>
public class SpawnComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, byte> SpawnComponent_GetIsActive_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> SpawnComponent_SetIsActive_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> SpawnComponent_IsCheckpoint_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> SpawnComponent_SetIsCheckpoint_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_GetSpawnPoint_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_SetSpawnPoint_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> SpawnComponent_GetRenderSpawnZoneInScene_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> SpawnComponent_GetZoneSize_Ptr;
#pragma warning restore 0649

    public bool IsActive
    {
        get { unsafe { return SpawnComponent_GetIsActive_Ptr != null && SpawnComponent_GetIsActive_Ptr(Entity.ID) != 0; } }
        set { unsafe { if (SpawnComponent_SetIsActive_Ptr != null) SpawnComponent_SetIsActive_Ptr(Entity.ID, value); } }
    }

    public bool IsCheckpoint
    {
        get { unsafe { return SpawnComponent_IsCheckpoint_Ptr != null && SpawnComponent_IsCheckpoint_Ptr(Entity.ID) != 0; } }
        set { unsafe { if (SpawnComponent_SetIsCheckpoint_Ptr != null) SpawnComponent_SetIsCheckpoint_Ptr(Entity.ID, value); } }
    }

    public Vector3 SpawnPoint
    {
        get
        {
            unsafe
            {
                if (SpawnComponent_GetSpawnPoint_Ptr == null) return Vector3.Zero;
                Vector3 point;
                SpawnComponent_GetSpawnPoint_Ptr(Entity.ID, &point);
                return point;
            }
        }
        set
        {
            unsafe
            {
                if (SpawnComponent_SetSpawnPoint_Ptr != null)
                    SpawnComponent_SetSpawnPoint_Ptr(Entity.ID, &value);
            }
        }
    }

    public bool RenderSpawnZoneInScene
    {
        get { unsafe { return SpawnComponent_GetRenderSpawnZoneInScene_Ptr != null && SpawnComponent_GetRenderSpawnZoneInScene_Ptr(Entity.ID) != 0; } }
    }

    public Vector3 ZoneSize
    {
        get
        {
            unsafe
            {
                if (SpawnComponent_GetZoneSize_Ptr == null) return Vector3.One;
                Vector3 size;
                SpawnComponent_GetZoneSize_Ptr(Entity.ID, &size);
                return size;
            }
        }
    }
}

/// <summary>Animation component wrapper.</summary>
public class AnimationComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, int> AnimationComponent_GetCurrentAnimationIndex_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, void> AnimationComponent_SetCurrentAnimationIndex_Ptr;
    internal static unsafe delegate* unmanaged<ulong, uint> AnimationComponent_GetIsPlaying_Ptr;
    internal static unsafe delegate* unmanaged<ulong, uint, void> AnimationComponent_SetIsPlaying_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> AnimationComponent_GetIsLooping_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> AnimationComponent_SetIsLooping_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> AnimationComponent_GetIsFinished_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> AnimationComponent_GetDuration_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> AnimationComponent_GetNormalizedTime_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> AnimationComponent_GetBlendDuration_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> AnimationComponent_SetBlendDuration_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, float, void> AnimationComponent_CrossFade_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, float, void> AnimationComponent_SetFloat_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, bool, void> AnimationComponent_SetBool_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, float> AnimationComponent_GetFloat_Ptr;
#pragma warning restore 0649

    public int CurrentAnimationIndex
    {
        get { unsafe { return AnimationComponent_GetCurrentAnimationIndex_Ptr != null ? AnimationComponent_GetCurrentAnimationIndex_Ptr(Entity.ID) : 0; } }
        set { unsafe { if (AnimationComponent_SetCurrentAnimationIndex_Ptr != null) AnimationComponent_SetCurrentAnimationIndex_Ptr(Entity.ID, value); } }
    }

    public bool IsPlaying
    {
        get { unsafe { return AnimationComponent_GetIsPlaying_Ptr != null && AnimationComponent_GetIsPlaying_Ptr(Entity.ID) != 0; } }
        set { unsafe { if (AnimationComponent_SetIsPlaying_Ptr != null) AnimationComponent_SetIsPlaying_Ptr(Entity.ID, value ? 1u : 0u); } }
    }

    public bool IsLooping
    {
        get { unsafe { return AnimationComponent_GetIsLooping_Ptr != null && AnimationComponent_GetIsLooping_Ptr(Entity.ID) != 0; } }
        set { unsafe { if (AnimationComponent_SetIsLooping_Ptr != null) AnimationComponent_SetIsLooping_Ptr(Entity.ID, value); } }
    }

    public bool IsFinished
    {
        get { unsafe { return AnimationComponent_GetIsFinished_Ptr != null && AnimationComponent_GetIsFinished_Ptr(Entity.ID) != 0; } }
    }

    public float Duration
    {
        get { unsafe { return AnimationComponent_GetDuration_Ptr != null ? AnimationComponent_GetDuration_Ptr(Entity.ID) : 0.0f; } }
    }

    public float NormalizedTime
    {
        get { unsafe { return AnimationComponent_GetNormalizedTime_Ptr != null ? AnimationComponent_GetNormalizedTime_Ptr(Entity.ID) : 0.0f; } }
    }

    public float BlendDuration
    {
        get { unsafe { return AnimationComponent_GetBlendDuration_Ptr != null ? AnimationComponent_GetBlendDuration_Ptr(Entity.ID) : 0.25f; } }
        set { unsafe { if (AnimationComponent_SetBlendDuration_Ptr != null) AnimationComponent_SetBlendDuration_Ptr(Entity.ID, value); } }
    }

    public void Play() => IsPlaying = true;
    public void Pause() => IsPlaying = false;

    public void CrossFade(int targetIndex, float duration = 0.25f)
    {
        unsafe
        {
            if (AnimationComponent_CrossFade_Ptr != null)
                AnimationComponent_CrossFade_Ptr(Entity.ID, targetIndex, duration);
        }
    }

    public unsafe void SetFloat(string name, float value)
    {
        if (AnimationComponent_SetFloat_Ptr != null)
            fixed (char* ptr = name)
                AnimationComponent_SetFloat_Ptr(Entity.ID, ptr, value);
    }

    public unsafe void SetBool(string name, bool value)
    {
        if (AnimationComponent_SetBool_Ptr != null)
            fixed (char* ptr = name)
                AnimationComponent_SetBool_Ptr(Entity.ID, ptr, value);
    }

    public unsafe float GetFloat(string name)
    {
        if (AnimationComponent_GetFloat_Ptr != null)
            fixed (char* ptr = name)
                return AnimationComponent_GetFloat_Ptr(Entity.ID, ptr);
        return 0.0f;
    }
}

/// <summary>Deprecated: use AnimationComponent instead. Kept for backward compatibility.</summary>
public class AnimationGraphComponent : AnimationComponent
{
}

/// <summary>Primitive/geometry component with material properties.</summary>
public class PrimitiveComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, Vector4*, void> PrimitiveComponent_GetAlbedoColor_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector4, void> PrimitiveComponent_SetAlbedoColor_Ptr;
#pragma warning restore 0649

    public Vector4 AlbedoColor
    {
        get
        {
            unsafe
            {
                if (PrimitiveComponent_GetAlbedoColor_Ptr == null) return new Vector4(1, 1, 1, 1);
                Vector4 color;
                PrimitiveComponent_GetAlbedoColor_Ptr(Entity.ID, &color);
                return color;
            }
        }
        set
        {
            unsafe { if (PrimitiveComponent_SetAlbedoColor_Ptr != null) PrimitiveComponent_SetAlbedoColor_Ptr(Entity.ID, value); }
        }
    }
}

/// <summary>Network identity component — identifies a networked entity.</summary>
public class NetworkIdentityComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, ulong> NetworkIdentityComponent_GetNetworkID_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> NetworkIdentityComponent_GetIsOwner_Ptr;
#pragma warning restore 0649

    public ulong NetworkID
    {
        get { unsafe { return NetworkIdentityComponent_GetNetworkID_Ptr == null ? 0 : NetworkIdentityComponent_GetNetworkID_Ptr(Entity.ID); } }
    }

    public bool IsOwner
    {
        get { unsafe { return NetworkIdentityComponent_GetIsOwner_Ptr != null && NetworkIdentityComponent_GetIsOwner_Ptr(Entity.ID) != 0; } }
    }
}

}
