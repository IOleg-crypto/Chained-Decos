using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Primitive geometry component. Shape parameters are exposed here;
    /// material properties are edited via the Material Editor (ModelComponent + .chmat).</summary>
    public class PrimitiveComponent : Component
    {
        // No scripting-accessible material properties — use ModelComponent for that.
        // Geometry parameters (Type, Radius, etc.) are not yet exposed to scripting;
        // add NativeProperty bindings here when needed.
    }
}
