using System;
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
    
    /// <summary>Short-cut to the entity's transform, or null if absent.</summary>
    public TransformComponent? Transform => Entity.Transform;
}

}
