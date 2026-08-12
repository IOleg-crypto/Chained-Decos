using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Player component wrapper.</summary>
    public class PlayerComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, float> PlayerComponent_GetMovementSpeed_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float, void> PlayerComponent_SetMovementSpeed_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float> PlayerComponent_GetJumpForce_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float, void> PlayerComponent_SetJumpForce_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float> PlayerComponent_GetLookSensitivity_Ptr;
        internal static unsafe delegate* unmanaged<ulong, float, void> PlayerComponent_SetLookSensitivity_Ptr;
#pragma warning restore 0649

        public float MovementSpeed
        {
            get { unsafe { return PlayerComponent_GetMovementSpeed_Ptr == null ? 0 : PlayerComponent_GetMovementSpeed_Ptr(Entity.ID); } }
            set { unsafe { if (PlayerComponent_SetMovementSpeed_Ptr != null) PlayerComponent_SetMovementSpeed_Ptr(Entity.ID, value); } }
        }

        public float JumpForce
        {
            get { unsafe { return PlayerComponent_GetJumpForce_Ptr == null ? 0 : PlayerComponent_GetJumpForce_Ptr(Entity.ID); } }
            set { unsafe { if (PlayerComponent_SetJumpForce_Ptr != null) PlayerComponent_SetJumpForce_Ptr(Entity.ID, value); } }
        }

        public float LookSensitivity
        {
            get { unsafe { return PlayerComponent_GetLookSensitivity_Ptr == null ? 0 : PlayerComponent_GetLookSensitivity_Ptr(Entity.ID); } }
            set { unsafe { if (PlayerComponent_SetLookSensitivity_Ptr != null) PlayerComponent_SetLookSensitivity_Ptr(Entity.ID, value); } }
        }
    }
}
