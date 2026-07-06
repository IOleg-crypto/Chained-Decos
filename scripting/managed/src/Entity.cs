using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Coral.Managed.Interop;

namespace Chained
{

// ─────────────────────────────────────────────────────────────────────────────
//  Component — base class for all component wrappers
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Base class for managed component wrappers.</summary>
public abstract class Component
{
    /// <summary>The entity this component belongs to.</summary>
    public Entity Entity { get; internal set; } = null!;
    
    /// <summary>Short-cut to the entity's transform.</summary>
    public TransformComponent Transform => Entity.Transform;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Entity — wraps a native entity ID and provides component access
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Entity wrapper with a small component cache.</summary>
public class Entity
{
    /// <summary>Native entity handle.</summary>
    public ulong ID { get; private set; }

    // Cache: avoids allocating a new stub on every GetComponent<T>() call
    private readonly Dictionary<System.Type, Component> _cache = new();

#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*, bool> Entity_HasComponent_Ptr;
    internal static unsafe delegate* unmanaged<char*, ulong*, int, int> Entity_FindAllWithComponent_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> Entity_AddComponent_Ptr;
#pragma warning restore 0649

    /// <summary>Wraps a native entity ID.</summary>
    public Entity(ulong id) { ID = id; }

    /// <summary>True when the entity handle is valid.</summary>
    public bool IsValid => ID != 0;

    /// <summary>Quick access to the TransformComponent.</summary>
    public TransformComponent Transform => GetComponent<TransformComponent>()!;

    // ── Component access ──────────────────────────────────────────────────

    private static unsafe bool HasComponent_Native(ulong entityID, string componentName)
    {
        if (Entity_HasComponent_Ptr == null) return false;
        fixed (char* ptr = componentName)
            return Entity_HasComponent_Ptr(entityID, ptr);
    }

    /// <summary>True when the component exists.</summary>
    public bool HasComponent<T>() where T : Component, new()
        => IsValid && HasComponent_Native(ID, typeof(T).Name);

    /// <summary>Returns the component wrapper, or null.</summary>
    public T? GetComponent<T>() where T : Component, new()
    {
        if (!IsValid) return null;

        System.Type componentType = typeof(T);
        if (_cache.TryGetValue(componentType, out Component? cachedComponent))
            return (T)cachedComponent;

        if (!HasComponent<T>())
            return null;

        T component = new T() { Entity = this };
        _cache[componentType] = component;
        return component;
    }

    /// <summary>Adds a component and returns its wrapper.</summary>
    public T AddComponent<T>() where T : Component, new()
    {
        if (!IsValid) throw new System.Exception("Cannot add component to invalid entity.");
        string name = typeof(T).Name;
        unsafe { fixed (char* ptr = name) Entity_AddComponent_Ptr(ID, ptr); }
        T component = new T() { Entity = this };
        _cache[typeof(T)] = component;
        return component;
    }

    /// <summary>Returns all entity IDs with the component.</summary>
    public static unsafe ulong[] FindAllWithComponent<T>() where T : Component, new()
    {
        if (Entity_FindAllWithComponent_Ptr == null) return Array.Empty<ulong>();
        string name = typeof(T).Name;
        {
            fixed (char* ptr = name)
            {
                ulong* buf = stackalloc ulong[512];
                int count = Entity_FindAllWithComponent_Ptr(ptr, buf, 512);
                ulong[] result = new ulong[count];
                for (int i = 0; i < count; i++) result[i] = buf[i];
                return result;
            }
        }
    }

    /// <summary>Clears the component cache.</summary>
    public void InvalidateComponentCache() => _cache.Clear();

    public override string ToString() => $"Entity({ID})";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Component implementations
// ─────────────────────────────────────────────────────────────────────────────

/// <summary>Transform wrapper.</summary>
public class TransformComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_GetTranslation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_SetTranslation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_GetRotation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_SetRotation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_GetScale_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_SetScale_Ptr;
#pragma warning restore 0649

    public Vector3 Translation
    {
        get { unsafe { Vector3 translation; Transform_GetTranslation_Ptr(Entity.ID, &translation); return translation; } }
        set { unsafe { Transform_SetTranslation_Ptr(Entity.ID, &value); } }
    }

    public Vector3 Rotation
    {
        get { unsafe { Vector3 rotation; Transform_GetRotation_Ptr(Entity.ID, &rotation); return rotation; } }
        set { unsafe { Transform_SetRotation_Ptr(Entity.ID, &value); } }
    }

    public Vector3 Scale
    {
        get { unsafe { Vector3 scale; Transform_GetScale_Ptr(Entity.ID, &scale); return scale; } }
        set { unsafe { Transform_SetScale_Ptr(Entity.ID, &value); } }
    }
}

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
    internal static unsafe delegate* unmanaged<ulong, bool> RigidBody_IsGrounded_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool> RigidBody_IsKinematic_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody_SetKinematic_Ptr;
#pragma warning restore 0649

    public Vector3 Velocity
    {
        get { unsafe { Vector3 velocity; RigidBody_GetVelocity_Ptr(Entity.ID, &velocity); return velocity; } }
        set { unsafe { RigidBody_SetVelocity_Ptr(Entity.ID, &value); } }
    }

    public bool IsGrounded { get { unsafe { return RigidBody_IsGrounded_Ptr(Entity.ID); } } }

    public bool IsKinematic
    {
        get { unsafe { return RigidBody_IsKinematic_Ptr(Entity.ID); } }
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
    internal static unsafe delegate* unmanaged<ulong, bool> Camera_GetPrimary_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> Camera_SetPrimary_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool> Camera_GetIsOrbit_Ptr;
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
        get { unsafe { return Camera_GetPrimary_Ptr(Entity.ID); } }
        set { unsafe { Camera_SetPrimary_Ptr(Entity.ID, value); } }
    }

    public bool IsOrbitCamera
    {
        get { unsafe { return Camera_GetIsOrbit_Ptr(Entity.ID); } }
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
    internal static unsafe delegate* unmanaged<ulong, bool> AudioComponent_IsPlaying_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*> AudioComponent_GetSoundPath_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Play_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Stop_Ptr;
#pragma warning restore 0649

    private static unsafe void SetVolume(ulong entityID, float volume) => AudioComponent_SetVolume_Ptr(entityID, volume);
    private static unsafe void SetLoop(ulong entityID, bool loop) => AudioComponent_SetLoop_Ptr(entityID, loop);
    private static unsafe bool IsPlaying_Native(ulong entityID) => AudioComponent_IsPlaying_Ptr(entityID);
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
    internal static unsafe delegate* unmanaged<ulong, bool> SpriteComponent_GetFlipX_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> SpriteComponent_SetFlipX_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool> SpriteComponent_GetFlipY_Ptr;
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
        get { unsafe { return SpriteComponent_GetFlipX_Ptr(Entity.ID); } }
        set { unsafe { SpriteComponent_SetFlipX_Ptr(Entity.ID, value); } }
    }

    public bool FlipY
    {
        get { unsafe { return SpriteComponent_GetFlipY_Ptr(Entity.ID); } }
        set { unsafe { SpriteComponent_SetFlipY_Ptr(Entity.ID, value); } }
    }

    public int ZOrder
    {
        get { unsafe { return SpriteComponent_GetZOrder_Ptr(Entity.ID); } }
        set { unsafe { SpriteComponent_SetZOrder_Ptr(Entity.ID, value); } }
    }
}

// ── UI Controls ───────────────────────────────────────────────────────────────

/// <summary>Button control wrapper.</summary>
public class ButtonControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, bool> ButtonControl_IsClicked_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool> ButtonControl_IsDown_Ptr;
#pragma warning restore 0649

    public bool IsClicked { get { unsafe { return ButtonControl_IsClicked_Ptr(Entity.ID); } } }
    public bool IsDown    { get { unsafe { return ButtonControl_IsDown_Ptr(Entity.ID); } } }
    public bool IsPressed => IsClicked; // Alias for backward compatibility
}

/// <summary>Checkbox wrapper.</summary>
public class CheckboxControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, bool> CheckboxControl_GetChecked_Ptr;
#pragma warning restore 0649

    private static unsafe bool GetChecked(ulong entityID) => CheckboxControl_GetChecked_Ptr(entityID);
    public bool IsChecked => GetChecked(Entity.ID);
}

/// <summary>Combo box wrapper.</summary>
public class ComboBoxControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, int> ComboBoxControl_GetSelectedIndex_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, void> ComboBoxControl_SetSelectedIndex_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> ComboBoxControl_AddItem_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> ComboBoxControl_ClearItems_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int> ComboBoxControl_GetItemCount_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, char*> ComboBoxControl_GetItem_Ptr;
#pragma warning restore 0649

    private static unsafe int GetSelectedIndex(ulong entityID) => ComboBoxControl_GetSelectedIndex_Ptr(entityID);
    private static unsafe void SetSelectedIndex(ulong entityID, int index) => ComboBoxControl_SetSelectedIndex_Ptr(entityID, index);
    private static unsafe void AddItem(ulong entityID, string item)
    { if (ComboBoxControl_AddItem_Ptr != null) fixed (char* ptr = item) ComboBoxControl_AddItem_Ptr(entityID, ptr); }
    private static unsafe void ClearItems(ulong entityID) => ComboBoxControl_ClearItems_Ptr(entityID);
    private static unsafe int GetItemCount(ulong entityID) => ComboBoxControl_GetItemCount_Ptr(entityID);
    private static unsafe string GetItem(ulong entityID, int index) => Marshal.PtrToStringUni(new IntPtr(ComboBoxControl_GetItem_Ptr(entityID, index))) ?? string.Empty;

    public int    SelectedIndex     { get => GetSelectedIndex(Entity.ID); set => SetSelectedIndex(Entity.ID, value); }
    public int    ItemCount         => GetItemCount(Entity.ID);
    public string? GetItem(int index) => GetItem(Entity.ID, index);
    public void   AddItem(string item) => AddItem(Entity.ID, item);
    public void   ClearItems()         => ClearItems(Entity.ID);
}



/// <summary>Shader control wrapper.</summary>
public class ShaderComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*, float, void> Shader_SetFloat_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, Vector3*, void> Shader_SetVec3_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool> Shader_GetEnabled_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> Shader_SetEnabled_Ptr;
#pragma warning restore 0649

    public bool Enabled
    {
        get { unsafe { return Shader_GetEnabled_Ptr(Entity.ID); } }
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

/// <summary>Network identity for tracking entities across clients.</summary>
public class NetworkIdentity : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, ulong> NetworkIdentity_GetNetworkID_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool> NetworkIdentity_IsOwned_Ptr;
#pragma warning restore 0649

    public ulong NetworkID { get { unsafe { return NetworkIdentity_GetNetworkID_Ptr(Entity.ID); } } }
    public bool IsOwned { get { unsafe { return NetworkIdentity_IsOwned_Ptr(Entity.ID); } } }
}

} // namespace Chained
