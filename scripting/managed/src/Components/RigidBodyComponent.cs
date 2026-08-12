using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Rigid-body wrapper.</summary>
    public class RigidBodyComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> RigidBody_GetVelocity_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> RigidBody_SetVelocity_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector3*, void> RigidBody_ForceSetVelocity_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> RigidBody_IsGrounded_Ptr;
        internal static unsafe delegate* unmanaged<ulong, uint> RigidBody_IsKinematic_Ptr;
        internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody_SetKinematic_Ptr;
#pragma warning restore 0649

        public Vector3 Velocity
        {
            get { unsafe { Vector3 velocity; RigidBody_GetVelocity_Ptr(Entity.ID, &velocity); return velocity; } }
            set { unsafe { RigidBody_SetVelocity_Ptr(Entity.ID, &value); } }
        }

        /// <summary>
        /// Sets velocity unconditionally, bypassing the Y-velocity override
        /// that the normal Velocity setter applies for Dynamic bodies.
        /// Use this to truly zero velocity (e.g. after a respawn teleport).
        /// </summary>
        public void ForceSetVelocity(Vector3 velocity)
        {
            unsafe { RigidBody_ForceSetVelocity_Ptr(Entity.ID, &velocity); }
        }

        public bool IsGrounded { get { unsafe { return RigidBody_IsGrounded_Ptr(Entity.ID) != 0; } } }

        public bool IsKinematic
        {
            get { unsafe { return RigidBody_IsKinematic_Ptr(Entity.ID) != 0; } }
            set { unsafe { RigidBody_SetKinematic_Ptr(Entity.ID, value); } }
        }
    }
}
