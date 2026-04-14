using System;
using System.Runtime.InteropServices;

namespace CHEngine
{

// ─────────────────────────────────────────────────────────────────────────────
//  Script — base class for all user-defined C# scripts
// ─────────────────────────────────────────────────────────────────────────────
/// <summary>Base class for managed scripts.</summary>
public abstract class Script
{
    /// <summary>The entity this script is attached to.</summary>
    public Entity Entity { get; private set; } = null!;

    /// <summary>Returns a component from the entity.</summary>
    public T? GetComponent<T>() where T : Component, new() => Entity.GetComponent<T>();
    /// <summary>True when the entity has the component.</summary>
    public bool HasComponent<T>() where T : Component, new() => Entity.HasComponent<T>();

#pragma warning disable 0649
    /// <summary>Native entry point for lifecycle pointers.</summary>
    internal static unsafe delegate*<ulong, IntPtr, IntPtr, IntPtr, IntPtr, IntPtr, IntPtr, void> RegisterLifecyclePointers;
#pragma warning restore 0649

    // Explicit delegates for unmanaged function pointers (Marshal does not support generic Action<T>)
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OnCreateDelegate();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OnStartDelegate();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OnUpdateDelegate(float deltaTime);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OnDestroyDelegate();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OnGUIDelegate();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OnCollisionEnterDelegate(ulong otherID);

    // Keep delegates alive to prevent GC
    private OnCreateDelegate? _onCreate;
    private OnStartDelegate? _onStart;
    private OnUpdateDelegate? _onUpdate;
    private OnDestroyDelegate? _onDestroy;
    private OnGUIDelegate?    _onGUI;
    private OnCollisionEnterDelegate? _onCollisionEnter;

    /// <summary>Cached lifecycle methods for a script type.</summary>
    internal class ScriptMethods
    {
        public System.Reflection.MethodInfo OnCreate;
        public System.Reflection.MethodInfo OnStart;
        public System.Reflection.MethodInfo OnUpdate;
        public System.Reflection.MethodInfo OnDestroy;
        public System.Reflection.MethodInfo OnGUI;
        public System.Reflection.MethodInfo OnCollisionEnter;
    }

    private static readonly System.Collections.Generic.Dictionary<Type, ScriptMethods> s_MethodCache = new();

    /// <summary>Caches lifecycle delegates and passes them to native code.</summary>
    internal void __Init(ulong entityID)
    {
        Entity = new Entity(entityID);
        
        var scriptType = this.GetType();
        
        if (!s_MethodCache.TryGetValue(scriptType, out var scriptMethods))
        {
            scriptMethods = new ScriptMethods
            {
                OnCreate = scriptType.GetMethod("OnCreate", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.NonPublic)!,
                OnStart = scriptType.GetMethod("OnStart", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.NonPublic)!,
                OnUpdate = scriptType.GetMethod("OnUpdate", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.NonPublic)!,
                OnDestroy = scriptType.GetMethod("OnDestroy", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.NonPublic)!,
                OnGUI = scriptType.GetMethod("OnGUI", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.NonPublic)!,
                OnCollisionEnter = scriptType.GetMethod("OnCollisionEnter", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.NonPublic)!
            };
            s_MethodCache[scriptType] = scriptMethods;
        }

        _onCreate  = (OnCreateDelegate)Delegate.CreateDelegate(typeof(OnCreateDelegate), this, scriptMethods.OnCreate);
        _onStart   = (OnStartDelegate)Delegate.CreateDelegate(typeof(OnStartDelegate), this, scriptMethods.OnStart);
        _onUpdate  = (OnUpdateDelegate)Delegate.CreateDelegate(typeof(OnUpdateDelegate), this, scriptMethods.OnUpdate);
        _onDestroy = (OnDestroyDelegate)Delegate.CreateDelegate(typeof(OnDestroyDelegate), this, scriptMethods.OnDestroy);
        _onGUI     = (OnGUIDelegate)Delegate.CreateDelegate(typeof(OnGUIDelegate), this, scriptMethods.OnGUI);
        _onCollisionEnter = (OnCollisionEnterDelegate)Delegate.CreateDelegate(typeof(OnCollisionEnterDelegate), this, scriptMethods.OnCollisionEnter);

        Log.Info($"[C# ScriptBase] __Init cached execution for Entity ID: {entityID}, Type: {scriptType.Name}");

        unsafe
        {
            RegisterLifecyclePointers(entityID, 
                Marshal.GetFunctionPointerForDelegate(_onCreate),
                Marshal.GetFunctionPointerForDelegate(_onStart),
                Marshal.GetFunctionPointerForDelegate(_onUpdate),
                Marshal.GetFunctionPointerForDelegate(_onDestroy),
                Marshal.GetFunctionPointerForDelegate(_onGUI),
                Marshal.GetFunctionPointerForDelegate(_onCollisionEnter));
        }
    }

    /// <summary>Called once after the script is instantiated and Entity is set.</summary>
    public virtual void OnCreate() { Log.Info($"[C# ScriptBase] OnCreate (Default) for Entity: {Entity?.ID}"); }

    /// <summary>Called once on the very first Update frame (after OnCreate).</summary>
    public virtual void OnStart() { Log.Info($"[C# ScriptBase] OnStart (Default) for Entity: {Entity?.ID}"); }

    /// <summary>Called every frame while the scene is running.</summary>
    public virtual void OnUpdate(float deltaTime) {}

    /// <summary>Called for UI rendering (e.g. HUD, Debug tools).</summary>
    public virtual void OnGUI() {}

    /// <summary>Called when a physics collision starts.</summary>
    public virtual void OnCollisionEnter(ulong otherEntityId) {}

    /// <summary>Called when the script or scene is destroyed / hot-reloaded.</summary>
    public virtual void OnDestroy() {}
}

}
