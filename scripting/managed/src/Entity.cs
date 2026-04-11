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

#pragma warning disable 0649
    internal static unsafe delegate*<ulong, NativeString, bool> Entity_HasComponent_Ptr;
    internal static unsafe delegate*<NativeString, NativeArray<ulong>> Entity_FindAllWithComponent_Ptr;
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

/// <summary>Movement-speed wrapper.</summary>
public class PlayerComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, float> PlayerComponent_GetMovementSpeed_Ptr;
    internal static unsafe delegate*<ulong, float, void> PlayerComponent_SetMovementSpeed_Ptr;
#pragma warning restore 0649

    private static unsafe float GetMovementSpeed(ulong entityID) => PlayerComponent_GetMovementSpeed_Ptr(entityID);
    private static unsafe void SetMovementSpeed(ulong entityID, float speed) => PlayerComponent_SetMovementSpeed_Ptr(entityID, speed);

    public float MovementSpeed
    {
        get => GetMovementSpeed(Entity.ID);
        set => SetMovementSpeed(Entity.ID, value);
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

} // namespace CHEngine
