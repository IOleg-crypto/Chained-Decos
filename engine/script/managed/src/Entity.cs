using Coral.Managed.Interop;
using System.Collections.Generic;

namespace CHEngine
{

// ─────────────────────────────────────────────────────────────────────────────
//  Component — base class for all component wrappers
// ─────────────────────────────────────────────────────────────────────────────
public abstract class Component
{
    public Entity Entity { get; internal set; } = null!;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Entity — wraps a native entity ID and provides component access
// ─────────────────────────────────────────────────────────────────────────────
public class Entity
{
    public ulong ID { get; private set; }

    // Cache: avoids allocating a new stub on every GetComponent<T>() call
    private readonly Dictionary<System.Type, Component> _cache = new();

#pragma warning disable 0649
    internal static unsafe delegate*<ulong, NativeString, bool> Entity_HasComponent_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_GetTranslation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_SetTranslation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_GetRotation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_SetRotation_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_GetScale_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_SetScale_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_GetVelocity_Ptr;
    internal static unsafe delegate*<ulong, Vector3*, void> Entity_SetVelocity_Ptr;
    internal static unsafe delegate*<ulong, bool> Entity_IsGrounded_Ptr;
    internal static unsafe delegate*<NativeString, NativeArray<ulong>> Entity_FindAllWithComponent_Ptr;
#pragma warning restore 0649

    public Entity(ulong id) { ID = id; }

    public bool IsValid => ID != 0;

    // ── Component access ──────────────────────────────────────────────────

    private static unsafe bool HasComponent_Native(ulong entityID, string componentName)
        => Entity_HasComponent_Ptr(entityID, componentName);

    public bool HasComponent<T>() where T : Component, new()
        => IsValid && HasComponent_Native(ID, typeof(T).Name);

    public T? GetComponent<T>() where T : Component, new()
    {
        if (!IsValid) return null;

        System.Type type = typeof(T);
        if (_cache.TryGetValue(type, out Component? cached))
            return (T)cached;

        if (!HasComponent<T>())
            return null;

        T comp = new T() { Entity = this };
        _cache[type] = comp;
        return comp;
    }

    public static ulong[] FindAllWithComponent<T>() where T : Component, new()
    {
        unsafe { return Entity_FindAllWithComponent_Ptr(typeof(T).Name).ToArray(); }
    }

    /// <summary>Clear component cache (call after hot-reload or scene change).</summary>
    public void InvalidateComponentCache() => _cache.Clear();

    public override string ToString() => $"Entity({ID})";

    // ── Internal Calls (Native) ───────────────────────────────────────────

    public static void GetTranslation(ulong entityID, out Vector3 outTranslation)
    {
        unsafe { fixed (Vector3* p = &outTranslation) Entity_GetTranslation_Ptr(entityID, p); }
    }

    public static void SetTranslation(ulong entityID, ref Vector3 inTranslation)
    {
        unsafe { fixed (Vector3* p = &inTranslation) Entity_SetTranslation_Ptr(entityID, p); }
    }

    public static void GetRotation(ulong entityID, out Vector3 outRotation)
    {
        unsafe { fixed (Vector3* p = &outRotation) Entity_GetRotation_Ptr(entityID, p); }
    }

    public static void SetRotation(ulong entityID, ref Vector3 inRotation)
    {
        unsafe { fixed (Vector3* p = &inRotation) Entity_SetRotation_Ptr(entityID, p); }
    }

    public static void GetScale(ulong entityID, out Vector3 outScale)
    {
        unsafe { fixed (Vector3* p = &outScale) Entity_GetScale_Ptr(entityID, p); }
    }

    public static void SetScale(ulong entityID, ref Vector3 inScale)
    {
        unsafe { fixed (Vector3* p = &inScale) Entity_SetScale_Ptr(entityID, p); }
    }

    public static void GetVelocity(ulong entityID, out Vector3 outVelocity)
    {
        unsafe { fixed (Vector3* p = &outVelocity) Entity_GetVelocity_Ptr(entityID, p); }
    }

    public static void SetVelocity(ulong entityID, ref Vector3 inVelocity)
    {
        unsafe { fixed (Vector3* p = &inVelocity) Entity_SetVelocity_Ptr(entityID, p); }
    }

    public static unsafe bool IsGrounded(ulong entityID) => Entity_IsGrounded_Ptr(entityID);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Component implementations
// ─────────────────────────────────────────────────────────────────────────────

public class TransformComponent : Component
{
    public Vector3 Translation
    {
        get { Entity.GetTranslation(Entity.ID, out Vector3 v); return v; }
        set { Entity.SetTranslation(Entity.ID, ref value); }
    }

    public Vector3 Rotation
    {
        get { Entity.GetRotation(Entity.ID, out Vector3 v); return v; }
        set { Entity.SetRotation(Entity.ID, ref value); }
    }

    public Vector3 Scale
    {
        get { Entity.GetScale(Entity.ID, out Vector3 v); return v; }
        set { Entity.SetScale(Entity.ID, ref value); }
    }
}

public class RigidBodyComponent : Component
{
    public Vector3 Velocity
    {
        get { Entity.GetVelocity(Entity.ID, out Vector3 v); return v; }
        set { Entity.SetVelocity(Entity.ID, ref value); }
    }

    public bool IsGrounded => Entity.IsGrounded(Entity.ID);
}

public class TagComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, NativeString> TagComponent_GetTag_Ptr;
#pragma warning restore 0649

    private static unsafe string GetTag_Native(ulong entityID)
    {
        return TagComponent_GetTag_Ptr(entityID);
    }

    public string Tag => GetTag_Native(Entity.ID) ?? string.Empty;
}

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
        unsafe { fixed (float* Py = &yaw, Pp = &pitch, Pd = &distance) Camera_GetOrbit_Ptr(entityID, Py, Pp, Pd); }
    }

    private static void SetOrbit(ulong entityID, float yaw, float pitch, float distance)
    {
        unsafe { Camera_SetOrbit_Ptr(entityID, yaw, pitch, distance); }
    }

    public Vector3 Forward
    {
        get { GetForward(Entity.ID, out Vector3 v); return v; }
    }

    public Vector3 Right
    {
        get { GetRight(Entity.ID, out Vector3 v); return v; }
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
        get { unsafe { return Camera_GetTargetTag_Ptr(Entity.ID); } }
        set { unsafe { Camera_SetTargetTag_Ptr(Entity.ID, value); } }
    }
}

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
    private static unsafe string GetSoundPath(ulong entityID)
    {
        return AudioComponent_GetSoundPath_Ptr(entityID);
    }

    public float Volume { set => SetVolume(Entity.ID, value); }
    public bool  Loop   { set => SetLoop(Entity.ID, value); }
    public bool  IsPlaying  => IsPlaying_Native(Entity.ID);
    public string SoundPath => GetSoundPath(Entity.ID) ?? string.Empty;
}

// ── UI Controls ───────────────────────────────────────────────────────────────

public class ButtonControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, bool> ButtonControl_IsPressed_Ptr;
#pragma warning restore 0649

    private static unsafe bool IsPressed_Native(ulong entityID) => ButtonControl_IsPressed_Ptr(entityID);
    public bool IsPressed => IsPressed_Native(Entity.ID);
}

public class CheckboxControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, bool> CheckboxControl_GetChecked_Ptr;
#pragma warning restore 0649

    private static unsafe bool GetChecked(ulong entityID) => CheckboxControl_GetChecked_Ptr(entityID);
    public bool IsChecked => GetChecked(Entity.ID);
}

public class ComboBoxControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, int> ComboBoxControl_GetSelectedIndex_Ptr;
    internal static unsafe delegate*<ulong, int, NativeString> ComboBoxControl_GetItem_Ptr;
#pragma warning restore 0649

    private static unsafe int GetSelectedIndex(ulong entityID) => ComboBoxControl_GetSelectedIndex_Ptr(entityID);
    private static unsafe string GetItem(ulong entityID, int idx)
    {
        return ComboBoxControl_GetItem_Ptr(entityID, idx);
    }

    public int    SelectedIndex     => GetSelectedIndex(Entity.ID);
    public string? GetItem(int idx) => GetItem(Entity.ID, idx);
}

// ── Gameplay Components ────────────────────────────────────────────────────────

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
        get { GetSpawnPoint(Entity.ID, out Vector3 p); return p; }
    }
}

public class SceneTransitionComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate*<ulong, NativeString> SceneTransitionComponent_GetTargetScene_Ptr;
#pragma warning restore 0649

    private static unsafe string GetTargetScene(ulong entityID)
    {
        return SceneTransitionComponent_GetTargetScene_Ptr(entityID);
    }
    public string? TargetScene => GetTargetScene(Entity.ID);
}

} // namespace CHEngine
