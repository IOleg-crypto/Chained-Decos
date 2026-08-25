using System;
using System.Runtime.InteropServices;

namespace Chained
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

    /// <summary>Instantiates the native C++ entity handle.</summary>
    internal void __Init(ulong entityID)
    {
        Entity = new Entity(entityID);
        // Do not call Log.Info here, as the pointer struct may not be fully initialized or marshaled yet.
    }

    /// <summary>Called once after the script is instantiated and Entity is set.</summary>
    public virtual void OnCreate() {}

    /// <summary>Called once on the very first Update frame (after OnCreate).</summary>
    public virtual void OnStart() {}

    /// <summary>Called every frame while the scene is running.</summary>
    public virtual void OnUpdate(float deltaTime) {}

    /// <summary>Called for UI rendering (e.g. HUD, Debug tools).</summary>
    public virtual void OnGUI() {}

    /// <summary>Called when a physics collision starts.</summary>
    public virtual void OnCollisionEnter(ulong otherEntityId) {}

    /// <summary>Called for generic engine events (KeyPress, Mouse, etc).</summary>
    public virtual void OnEvent(int eventType) {}

    /// <summary>Called when the script or scene is destroyed / hot-reloaded.</summary>
    public virtual void OnDestroy() {}
}

}
