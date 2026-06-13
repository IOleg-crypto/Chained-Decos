using Coral.Managed.Interop;
using System.Collections.Generic;

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

    protected unsafe TField GetField<TComponent, TField>(string fieldName) where TComponent : Component
    {
        return Entity.GetField<TComponent, TField>(fieldName);
    }

    protected unsafe void SetField<TComponent, TField>(string fieldName, TField value) where TComponent : Component
    {
        Entity.SetField<TComponent, TField>(fieldName, value);
    }
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

    internal static unsafe delegate* unmanaged<ulong, NativeString, bool> Entity_HasComponent_Ptr;
    internal static unsafe delegate* unmanaged<NativeString, NativeArray<ulong>> Entity_FindAllWithComponent_Ptr;
    internal static unsafe delegate* unmanaged<ulong, NativeString, void> Entity_AddComponent_Ptr;
#pragma warning restore 0649

    /// <summary>Wraps a native entity ID.</summary>
    public Entity(ulong id) { ID = id; }

    /// <summary>True when the entity handle is valid.</summary>
    public bool IsValid => ID != 0;

    /// <summary>Quick access to the TransformComponent.</summary>
    public TransformComponent Transform => GetComponent<TransformComponent>()!;

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

    // ── Universal Reflection Property Access ────────────────────────────────

    internal static unsafe delegate* unmanaged<ulong, NativeString, NativeString, void*, bool> Entity_GetComponentField_Ptr;
    internal static unsafe delegate* unmanaged<ulong, NativeString, NativeString, void*, bool> Entity_SetComponentField_Ptr;
    internal static unsafe delegate* unmanaged<ulong, NativeString, NativeString, NativeString> Entity_GetComponentFieldString_Ptr;
    internal static unsafe delegate* unmanaged<ulong, NativeString, NativeString, NativeString, bool> Entity_SetComponentFieldString_Ptr;

    /// <summary>Reads a field dynamically using reflection-cpp from the engine.</summary>
    public unsafe TField GetField<TComponent, TField>(string fieldName) where TComponent : Component
    {
        if (!IsValid) return default(TField)!;

        // String needs special marshalling across Coral due to non-blittable memory boundaries
        if (typeof(TField) == typeof(string))
        {
            string? s = Entity_GetComponentFieldString_Ptr(ID, typeof(TComponent).Name, fieldName);
            return (TField)(object)(s ?? string.Empty);
        }

        TField result = default(TField)!;
        void* ptr = System.Runtime.CompilerServices.Unsafe.AsPointer(ref result);
        bool success = Entity_GetComponentField_Ptr(ID, typeof(TComponent).Name, fieldName, ptr);
        return result;
    }

    /// <summary>Writes a field dynamically using reflection-cpp from the engine.</summary>
    public unsafe void SetField<TComponent, TField>(string fieldName, TField value) where TComponent : Component
    {
        if (!IsValid) return;

        if (typeof(TField) == typeof(string))
        {
            // Explicit type casting logic for Strings
            string s = (string)(object)value!;
            Entity_SetComponentFieldString_Ptr(ID, typeof(TComponent).Name, fieldName, s);
            return;
        }

        void* ptr = System.Runtime.CompilerServices.Unsafe.AsPointer(ref value);
        Entity_SetComponentField_Ptr(ID, typeof(TComponent).Name, fieldName, ptr);
    }

    public override string ToString() => $"Entity({ID})";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Component implementations
// ─────────────────────────────────────────────────────────────────────────────

/// <summary>Transform wrapper.</summary>
public class TransformComponent : Component
{
    public Vector3 Translation
    {
        get => GetField<TransformComponent, Vector3>("Translation");
        set => SetField<TransformComponent, Vector3>("Translation", value);
    }

    public Vector3 Rotation
    {
        get => GetField<TransformComponent, Vector3>("Rotation");
        set => SetField<TransformComponent, Vector3>("Rotation", value);
    }

    public Vector3 Scale
    {
        get => GetField<TransformComponent, Vector3>("Scale");
        set => SetField<TransformComponent, Vector3>("Scale", value);
    }
}

/// <summary>Model/Mesh wrapper.</summary>
public class ModelComponent : Component
{
    public string ModelPath
    {
        get => GetField<ModelComponent, string>("ModelPath");
        set => SetField<ModelComponent, string>("ModelPath", value);
    }
}

/// <summary>Rigid-body wrapper.</summary>
public class RigidBodyComponent : Component
{
    public Vector3 Velocity
    {
        get => GetField<RigidBodyComponent, Vector3>("Velocity");
        set => SetField<RigidBodyComponent, Vector3>("Velocity", value);
    }

    public bool IsGrounded => GetField<RigidBodyComponent, bool>("IsGrounded");

    public bool IsKinematic
    {
        get => GetField<RigidBodyComponent, bool>("IsKinematic");
        set => SetField<RigidBodyComponent, bool>("IsKinematic", value);
    }
}

/// <summary>Player settings wrapper — reads directly from the C++ PlayerComponent.</summary>
public class PlayerComponent : Component
{
    public float MovementSpeed
    {
        get => GetField<PlayerComponent, float>("MovementSpeed");
        set => SetField<PlayerComponent, float>("MovementSpeed", value);
    }

    public float JumpForce
    {
        get => GetField<PlayerComponent, float>("JumpForce");
        set => SetField<PlayerComponent, float>("JumpForce", value);
    }

    public float LookSensitivity
    {
        get => GetField<PlayerComponent, float>("LookSensitivity");
        set => SetField<PlayerComponent, float>("LookSensitivity", value);
    }
}

/// <summary>Audio wrapper.</summary>
public class AudioComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Play_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> AudioComponent_Stop_Ptr;
#pragma warning restore 0649

    public float Volume { get => GetField<AudioComponent, float>("Volume"); set => SetField<AudioComponent, float>("Volume", value); }
    public bool  Loop   { get => GetField<AudioComponent, bool>("Loop"); set => SetField<AudioComponent, bool>("Loop", value); }
    public bool  IsPlaying  => GetField<AudioComponent, bool>("IsPlaying");
    public string SoundPath { get => GetField<AudioComponent, string>("SoundPath"); set => SetField<AudioComponent, string>("SoundPath", value); }

    public void Play() 
    {
        unsafe { AudioComponent_Play_Ptr(Entity.ID); }
    }

    public void Stop()
    {
        unsafe { AudioComponent_Stop_Ptr(Entity.ID); }
    }
}

/// <summary>Camera wrapper.</summary>
public class CameraComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Camera_GetForward_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Camera_GetRight_Ptr;
#pragma warning restore 0649

    private static void GetForward(ulong entityID, out Vector3 outForward)
    {
        unsafe { fixed (Vector3* p = &outForward) Camera_GetForward_Ptr(entityID, p); }
    }

    private static void GetRight(ulong entityID, out Vector3 outRight)
    {
        unsafe { fixed (Vector3* p = &outRight) Camera_GetRight_Ptr(entityID, p); }
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
    {
        yaw = GetField<CameraComponent, float>("OrbitYaw");
        pitch = GetField<CameraComponent, float>("OrbitPitch");
        distance = GetField<CameraComponent, float>("OrbitDistance");
    }

    public void SetOrbit(float yaw, float pitch, float distance)
    {
        SetField<CameraComponent, float>("OrbitYaw", yaw);
        SetField<CameraComponent, float>("OrbitPitch", pitch);
        SetField<CameraComponent, float>("OrbitDistance", distance);
    }

    public bool Primary
    {
        get => GetField<CameraComponent, bool>("Primary");
        set => SetField<CameraComponent, bool>("Primary", value);
    }

    public bool IsOrbitCamera
    {
        get => GetField<CameraComponent, bool>("IsOrbitCamera");
        set => SetField<CameraComponent, bool>("IsOrbitCamera", value);
    }

    public string TargetEntityTag
    {
        get => GetField<CameraComponent, string>("TargetEntityTag");
        set => SetField<CameraComponent, string>("TargetEntityTag", value);
    }
}



/// <summary>2D Sprite wrapper.</summary>
public class SpriteComponent : Component
{
    public string TexturePath
    {
        get => GetField<SpriteComponent, string>("TexturePath");
        set => SetField<SpriteComponent, string>("TexturePath", value);
    }

    public Vector4 Tint
    {
        get => GetField<SpriteComponent, Vector4>("Tint");
        set => SetField<SpriteComponent, Vector4>("Tint", value);
    }

    public bool FlipX
    {
        get => GetField<SpriteComponent, bool>("FlipX");
        set => SetField<SpriteComponent, bool>("FlipX", value);
    }

    public bool FlipY
    {
        get => GetField<SpriteComponent, bool>("FlipY");
        set => SetField<SpriteComponent, bool>("FlipY", value);
    }

    public int ZOrder
    {
        get => GetField<SpriteComponent, int>("ZOrder");
        set => SetField<SpriteComponent, int>("ZOrder", value);
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
    internal static unsafe delegate* unmanaged<ulong, NativeString, void> ComboBoxControl_AddItem_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> ComboBoxControl_ClearItems_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int> ComboBoxControl_GetItemCount_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, NativeString> ComboBoxControl_GetItem_Ptr;
#pragma warning restore 0649

    private static unsafe int GetSelectedIndex(ulong entityID) => ComboBoxControl_GetSelectedIndex_Ptr(entityID);
    private static unsafe void SetSelectedIndex(ulong entityID, int index) => ComboBoxControl_SetSelectedIndex_Ptr(entityID, index);
    private static unsafe void AddItem(ulong entityID, string item) => ComboBoxControl_AddItem_Ptr(entityID, item);
    private static unsafe void ClearItems(ulong entityID) => ComboBoxControl_ClearItems_Ptr(entityID);
    private static unsafe int GetItemCount(ulong entityID) => ComboBoxControl_GetItemCount_Ptr(entityID);
    private static unsafe string? GetItem(ulong entityID, int index)
    {
        return ComboBoxControl_GetItem_Ptr(entityID, index);
    }

    public int    SelectedIndex     { get => GetSelectedIndex(Entity.ID); set => SetSelectedIndex(Entity.ID, value); }
    public int    ItemCount         => GetItemCount(Entity.ID);
    public string? GetItem(int index) => GetItem(Entity.ID, index);
    public void   AddItem(string item) => AddItem(Entity.ID, item);
    public void   ClearItems()         => ClearItems(Entity.ID);
}

// ── Gameplay Components ────────────────────────────────────────────────────────

/// <summary>Spawn point wrapper.</summary>
public class SpawnComponent : Component
{
    public bool IsActive => GetField<SpawnComponent, bool>("IsActive");
    public Vector3 SpawnPoint => GetField<SpawnComponent, Vector3>("SpawnPoint");
}

/// <summary>Scene transition wrapper.</summary>
public class SceneTransitionComponent : Component
{
    public string? TargetScene => GetField<SceneTransitionComponent, string>("TargetScene");
}

/// <summary>RPG stats wrapper.</summary>
public class RPGStatsComponent : Component
{
    public int Level { get => GetField<RPGStatsComponent, int>("Level"); set => SetField<RPGStatsComponent, int>("Level", value); }
    public float Health { get => GetField<RPGStatsComponent, float>("Health"); set => SetField<RPGStatsComponent, float>("Health", value); }
    public int Gold { get => GetField<RPGStatsComponent, int>("Gold"); set => SetField<RPGStatsComponent, int>("Gold", value); }
}

/// <summary>Skill wrapper.</summary>
public class SkillComponent : Component
{
    public bool IsUnlocked { get => GetField<SkillComponent, bool>("IsUnlocked"); set => SetField<SkillComponent, bool>("IsUnlocked", value); }
}

/// <summary>Inventory wrapper.</summary>
public class InventoryComponent : Component
{
}

/// <summary>Shader control wrapper.</summary>
public class ShaderComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, NativeString, float, void> Shader_SetFloat_Ptr;
    internal static unsafe delegate* unmanaged<ulong, NativeString, Vector3*, void> Shader_SetVec3_Ptr;
#pragma warning restore 0649

    public bool Enabled
    {
        get => GetField<ShaderComponent, bool>("Enabled");
        set => SetField<ShaderComponent, bool>("Enabled", value);
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

/// <summary>Network identity for tracking entities across clients.</summary>
public class NetworkIdentity : Component
{
    public ulong NetworkID => GetField<NetworkIdentity, ulong>("NetworkID");
    public bool IsOwned => GetField<NetworkIdentity, bool>("IsOwned");
}

} // namespace Chained
