using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Network identity component — identifies a networked entity.</summary>
    public class NetworkIdentityComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, ulong> NetworkIdentityComponent_GetNetworkID_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> NetworkIdentityComponent_GetIsOwner_Ptr;
#pragma warning restore 0649

        public ulong NetworkID
        {
            get { unsafe { return NetworkIdentityComponent_GetNetworkID_Ptr == null ? 0 : NetworkIdentityComponent_GetNetworkID_Ptr(Entity.ID); } }
        }

        public bool IsOwner
        {
            get { unsafe { return NetworkIdentityComponent_GetIsOwner_Ptr != null && NetworkIdentityComponent_GetIsOwner_Ptr(Entity.ID) != 0; } }
        }
    }
}
