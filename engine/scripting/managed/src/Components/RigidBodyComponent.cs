using System;
using Coral.Managed.Interop;

namespace Chained
{
    /// <summary>Rigid-body wrapper.</summary>
    [NativeProperty("Velocity", "Vector3", "RigidBody_GetVelocity", "RigidBody_SetVelocity")]
    [NativeProperty("IsGrounded", "bool", "RigidBody_IsGrounded")]
    [NativeProperty("IsKinematic", "bool", "RigidBody_IsKinematic", "RigidBody_SetKinematic")]
    [NativeCall("Chained.RigidBodyComponent", "RigidBody_ForceSetVelocity", "void", "ulong", "Vector3*")]
    public partial class RigidBodyComponent : Component
    {
        /// <summary>
        /// Sets velocity unconditionally, bypassing the Y-velocity override
        /// that the normal Velocity setter applies for Dynamic bodies.
        /// Use this to truly zero velocity (e.g. after a respawn teleport).
        /// </summary>
        public void ForceSetVelocity(Vector3 velocity)
        {
            unsafe { if (RigidBody_ForceSetVelocity_Ptr != null) RigidBody_ForceSetVelocity_Ptr(Entity.ID, &velocity); }
        }
    }
}
