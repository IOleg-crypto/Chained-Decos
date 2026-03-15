using System;
using System.Runtime.InteropServices;

namespace CHEngine
{

// ─────────────────────────────────────────────────────────────────────────────
//  Script — base class for all user-defined C# scripts
// ─────────────────────────────────────────────────────────────────────────────
public abstract class Script
{
    /// <summary>The entity this script is attached to.</summary>
    public Entity Entity { get; private set; } = null!;

#pragma warning disable 0649
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

    // Called by C++ immediately after CreateInstance(), before OnCreate().
    internal void __Init(ulong entityID)
    {
        Entity = new Entity(entityID);
        
        var type = this.GetType();
        _onCreate  = (OnCreateDelegate)Delegate.CreateDelegate(typeof(OnCreateDelegate), this, type.GetMethod("OnCreate")!);
        _onStart   = (OnStartDelegate)Delegate.CreateDelegate(typeof(OnStartDelegate), this, type.GetMethod("OnStart")!);
        _onUpdate  = (OnUpdateDelegate)Delegate.CreateDelegate(typeof(OnUpdateDelegate), this, type.GetMethod("OnUpdate")!);
        _onDestroy = (OnDestroyDelegate)Delegate.CreateDelegate(typeof(OnDestroyDelegate), this, type.GetMethod("OnDestroy")!);
        _onGUI     = (OnGUIDelegate)Delegate.CreateDelegate(typeof(OnGUIDelegate), this, type.GetMethod("OnGUI")!);
        _onCollisionEnter = (OnCollisionEnterDelegate)Delegate.CreateDelegate(typeof(OnCollisionEnterDelegate), this, type.GetMethod("OnCollisionEnter")!);

        Log.Info($"[C# ScriptBase] __Init called for Entity ID: {entityID}, Type: {this.GetType().Name}");

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
    public virtual void OnCollisionEnter(ulong otherID) {}

    /// <summary>Called when the script or scene is destroyed / hot-reloaded.</summary>
    public virtual void OnDestroy() {}
}

}
