using Coral.Managed.Interop;
using System.Collections.Generic;

namespace CHEngine
{

// ─────────────────────────────────────────────────────────────────────────────
//  Component — base class for all component wrappers
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Base class for managed component wrappers.</summary>
public abstract class Component
{
    public Entity Entity { get; internal set; } = null!;
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

    internal static unsafe delegate*<ulong, NativeString, bool> Entity_HasComponent_Ptr;
    internal static unsafe delegate*<NativeString, NativeArray<ulong>> Entity_FindAllWithComponent_Ptr;
    internal static unsafe delegate*<ulong, NativeString, void> Entity_AddComponent_Ptr;
#pragma warning restore 0649

    /// <summary>Wraps a native entity ID.</summary>
    public Entity(ulong id) { ID = id; }

    /// <summary>True when the entity handle is valid.</summary>
    public bool IsValid => ID != 0;

    // ── Component access ──────────────────────────────────────────────────

    private static unsafe bool HasComponent_Native(ulong entityID, string componentName)
        => Entity_HasComponent_Ptr(entityID, componentName);

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
        
        unsafe { Entity_AddComponent_Ptr(ID, typeof(T).Name); }
        
        T component = new T() { Entity = this };
        _cache[typeof(T)] = component;
        return component;
    }

    /// <summary>Returns all entity IDs with the component.</summary>
    public static ulong[] FindAllWithComponent<T>() where T : Component, new()
    {
        unsafe { return Entity_FindAllWithComponent_Ptr(typeof(T).Name).ToArray(); }
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
    internal static unsafe delegate*<ulong, Vector3*, void> Transform_GetTranslation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Transform_SetTranslation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Transform_GetRotation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Transform_SetRotation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Transform_GetScale_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Transform_SetScale_Ptr;
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
    internal static unsafe delegate*<ulong, NativeString> Model_GetModelPath_Ptr;
    internal static unsafe delegate*<ulong, NativeString, void> Model_SetModelPath_Ptr;
#pragma warning restore 0649

    public string ModelPath
    {
        get { unsafe { string? result = Model_GetModelPath_Ptr(Entity.ID); return result ?? string.Empty; } }
        set { unsafe { Model_SetModelPath_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Rigid-body wrapper.</summary>
public class RigidBodyComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, Vector3*, void> RigidBody_GetVelocity_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> RigidBody_SetVelocity_Ptr;
    internal static unsafe delegate*<ulong, bool> RigidBody_IsGrounded_Ptr;
    internal static unsafe delegate*<ulong, bool> RigidBody_IsKinematic_Ptr;
    internal static unsafe delegate*<ulong, bool, void> RigidBody_SetKinematic_Ptr;
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
    internal static unsafe delegate*<ulong, NativeString> TagComponent_GetTag_Ptr;
#pragma warning restore 0649

    private static unsafe string? GetTag_Native(ulong entityID)
    {
        return TagComponent_GetTag_Ptr(entityID);
    }

    public string Tag => GetTag_Native(Entity.ID) ?? string.Empty;
}

/// <summary>Camera wrapper.</summary>
public class CameraComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, Vector3*, void> Camera_GetForward_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Camera_GetRight_Ptr;
    internal static unsafe delegate*<ulong, float*, float*, float*, void> Camera_GetOrbit_Ptr;
    internal static unsafe delegate*<ulong, float, float, float, void> Camera_SetOrbit_Ptr;
    internal static unsafe delegate*<ulong, bool> Camera_GetPrimary_Ptr;
    internal static unsafe delegate*<ulong, bool, void> Camera_SetPrimary_Ptr;
    internal static unsafe delegate*<ulong, bool> Camera_GetIsOrbit_Ptr;
    internal static unsafe delegate*<ulong, bool, void> Camera_SetIsOrbit_Ptr;
    internal static unsafe delegate*<ulong, NativeString> Camera_GetTargetTag_Ptr;
    internal static unsafe delegate*<ulong, NativeString, void> Camera_SetTargetTag_Ptr;
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
        get { unsafe { string? result = Camera_GetTargetTag_Ptr(Entity.ID); return result ?? string.Empty; } }
        set { unsafe { Camera_SetTargetTag_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Player settings wrapper — reads directly from the C++ PlayerComponent.</summary>
public class PlayerComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, float> PlayerComponent_GetMovementSpeed_Ptr;
    internal static unsafe delegate*<ulong, float, void> PlayerComponent_SetMovementSpeed_Ptr;
    internal static unsafe delegate*<ulong, float> PlayerComponent_GetJumpForce_Ptr;
    internal static unsafe delegate*<ulong, float, void> PlayerComponent_SetJumpForce_Ptr;
    internal static unsafe delegate*<ulong, float> PlayerComponent_GetLookSensitivity_Ptr;
    internal static unsafe delegate*<ulong, float, void> PlayerComponent_SetLookSensitivity_Ptr;
#pragma warning restore 0649

    public float MovementSpeed
    {
        get { unsafe { return PlayerComponent_GetMovementSpeed_Ptr(Entity.ID); } }
        set { unsafe { PlayerComponent_SetMovementSpeed_Ptr(Entity.ID, value); } }
    }

    public float JumpForce
    {
        get { unsafe { return PlayerComponent_GetJumpForce_Ptr(Entity.ID); } }
        set { unsafe { PlayerComponent_SetJumpForce_Ptr(Entity.ID, value); } }
    }

    public float LookSensitivity
    {
        get { unsafe { return PlayerComponent_GetLookSensitivity_Ptr(Entity.ID); } }
        set { unsafe { PlayerComponent_SetLookSensitivity_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Audio wrapper.</summary>
public class AudioComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, float, void> AudioComponent_SetVolume_Ptr;
    internal static unsafe delegate*<ulong, bool, void> AudioComponent_SetLoop_Ptr;
    internal static unsafe delegate*<ulong, bool> AudioComponent_IsPlaying_Ptr;
    internal static unsafe delegate*<ulong, NativeString> AudioComponent_GetSoundPath_Ptr;
    internal static unsafe delegate*<ulong, void> AudioComponent_Play_Ptr;
    internal static unsafe delegate*<ulong, void> AudioComponent_Stop_Ptr;
#pragma warning restore 0649

    private static unsafe void SetVolume(ulong entityID, float volume) => AudioComponent_SetVolume_Ptr(entityID, volume);
    private static unsafe void SetLoop(ulong entityID, bool loop) => AudioComponent_SetLoop_Ptr(entityID, loop);
    private static unsafe bool IsPlaying_Native(ulong entityID) => AudioComponent_IsPlaying_Ptr(entityID);
    private static unsafe string? GetSoundPath(ulong entityID)
    {
        return AudioComponent_GetSoundPath_Ptr(entityID);
    }

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
    internal static unsafe delegate*<ulong, NativeString> SpriteComponent_GetTexturePath_Ptr;
    internal static unsafe delegate*<ulong, NativeString, void> SpriteComponent_SetTexturePath_Ptr;
    internal static unsafe delegate*<ulong, Vector4*, void> SpriteComponent_GetTint_Ptr;
    internal static unsafe delegate*<ulong, Vector4, void> SpriteComponent_SetTint_Ptr;
    internal static unsafe delegate*<ulong, bool> SpriteComponent_GetFlipX_Ptr;
    internal static unsafe delegate*<ulong, bool, void> SpriteComponent_SetFlipX_Ptr;
    internal static unsafe delegate*<ulong, bool> SpriteComponent_GetFlipY_Ptr;
    internal static unsafe delegate*<ulong, bool, void> SpriteComponent_SetFlipY_Ptr;
    internal static unsafe delegate*<ulong, int> SpriteComponent_GetZOrder_Ptr;
    internal static unsafe delegate*<ulong, int, void> SpriteComponent_SetZOrder_Ptr;
#pragma warning restore 0649

    public string TexturePath
    {
        get { unsafe { return SpriteComponent_GetTexturePath_Ptr(Entity.ID); } }
        set { unsafe { SpriteComponent_SetTexturePath_Ptr(Entity.ID, value); } }
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
    internal static unsafe delegate*<ulong, bool> ButtonControl_IsPressed_Ptr;
#pragma warning restore 0649

    private static unsafe bool IsPressed_Native(ulong entityID) => ButtonControl_IsPressed_Ptr(entityID);
    public bool IsPressed => IsPressed_Native(Entity.ID);
}

/// <summary>Checkbox wrapper.</summary>
public class CheckboxControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, bool> CheckboxControl_GetChecked_Ptr;
#pragma warning restore 0649

    private static unsafe bool GetChecked(ulong entityID) => CheckboxControl_GetChecked_Ptr(entityID);
    public bool IsChecked => GetChecked(Entity.ID);
}

/// <summary>Combo box wrapper.</summary>
public class ComboBoxControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, int> ComboBoxControl_GetSelectedIndex_Ptr;
    internal static unsafe delegate*<ulong, int, NativeString> ComboBoxControl_GetItem_Ptr;
#pragma warning restore 0649

    private static unsafe int GetSelectedIndex(ulong entityID) => ComboBoxControl_GetSelectedIndex_Ptr(entityID);
    private static unsafe string? GetItem(ulong entityID, int index)
    {
        return ComboBoxControl_GetItem_Ptr(entityID, index);
    }

    public int    SelectedIndex     => GetSelectedIndex(Entity.ID);
    public string? GetItem(int index) => GetItem(Entity.ID, index);
}

// ── Gameplay Components ────────────────────────────────────────────────────────

/// <summary>Spawn point wrapper.</summary>
public class SpawnComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, bool> SpawnComponent_IsActive_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> SpawnComponent_GetSpawnPoint_Ptr;
#pragma warning restore 0649

    private static unsafe bool IsActive_Native(ulong entityID) => SpawnComponent_IsActive_Ptr(entityID);
    private static void GetSpawnPoint(ulong entityID, out Vector3 point)
    {
        unsafe { fixed (Vector3* p = &point) SpawnComponent_GetSpawnPoint_Ptr(entityID, p); }
    }

    public bool IsActive => IsActive_Native(Entity.ID);
    public Vector3 SpawnPoint
    {
        get { GetSpawnPoint(Entity.ID, out Vector3 spawnPoint); return spawnPoint; }
    }
}

/// <summary>Scene transition wrapper.</summary>
public class SceneTransitionComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, NativeString> SceneTransitionComponent_GetTargetScene_Ptr;
#pragma warning restore 0649

    private static unsafe string? GetTargetScene(ulong entityID)
    {
        return SceneTransitionComponent_GetTargetScene_Ptr(entityID);
    }
    public string? TargetScene => GetTargetScene(Entity.ID);
}

/// <summary>RPG stats wrapper.</summary>
public class RPGStatsComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, int> RPGStatsComponent_GetLevel_Ptr;
    internal static unsafe delegate*<ulong, int, void> RPGStatsComponent_SetLevel_Ptr;
    internal static unsafe delegate*<ulong, float> RPGStatsComponent_GetHealth_Ptr;
    internal static unsafe delegate*<ulong, float, void> RPGStatsComponent_SetHealth_Ptr;
    internal static unsafe delegate*<ulong, int> RPGStatsComponent_GetGold_Ptr;
    internal static unsafe delegate*<ulong, int, void> RPGStatsComponent_SetGold_Ptr;
#pragma warning restore 0649

    public int Level { get { unsafe { return RPGStatsComponent_GetLevel_Ptr(Entity.ID); } } set { unsafe { RPGStatsComponent_SetLevel_Ptr(Entity.ID, value); } } }
    public float Health { get { unsafe { return RPGStatsComponent_GetHealth_Ptr(Entity.ID); } } set { unsafe { RPGStatsComponent_SetHealth_Ptr(Entity.ID, value); } } }
    public int Gold { get { unsafe { return RPGStatsComponent_GetGold_Ptr(Entity.ID); } } set { unsafe { RPGStatsComponent_SetGold_Ptr(Entity.ID, value); } } }
}

/// <summary>Skill wrapper.</summary>
public class SkillComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, bool> SkillComponent_IsUnlocked_Ptr;
    internal static unsafe delegate*<ulong, bool, void> SkillComponent_SetUnlocked_Ptr;
#pragma warning restore 0649

    public bool IsUnlocked { get { unsafe { return SkillComponent_IsUnlocked_Ptr(Entity.ID); } } set { unsafe { SkillComponent_SetUnlocked_Ptr(Entity.ID, value); } } }
}

/// <summary>Inventory wrapper.</summary>
public class InventoryComponent : Component
{
}

/// <summary>Shader control wrapper.</summary>
public class ShaderComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, NativeString, float, void> Shader_SetFloat_Ptr;
    internal static unsafe delegate*<ulong, NativeString, Vector3*, void> Shader_SetVec3_Ptr;
    internal static unsafe delegate*<ulong, bool> Shader_GetEnabled_Ptr;
    internal static unsafe delegate*<ulong, bool, void> Shader_SetEnabled_Ptr;
#pragma warning restore 0649

    public bool Enabled
    {
        get { unsafe { return Shader_GetEnabled_Ptr(Entity.ID); } }
        set { unsafe { Shader_SetEnabled_Ptr(Entity.ID, value); } }
    }

    public void SetFloat(string name, float value)
    {
        unsafe { Shader_SetFloat_Ptr(Entity.ID, name, value); }
    }

    public void SetVector3(string name, Vector3 value)
    {
        unsafe { Shader_SetVec3_Ptr(Entity.ID, name, &value); }
    }
}

} // namespace CHEngine
