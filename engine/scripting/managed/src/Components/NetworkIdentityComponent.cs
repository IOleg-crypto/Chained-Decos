using System;

namespace Chained
{
    /// <summary>Network identity component — identifies a networked entity.</summary>
    [NativeProperty("NetworkID", "ulong", "NetworkIdentityComponent_GetNetworkID")]
    [NativeProperty("IsOwner", "bool", "NetworkIdentityComponent_GetIsOwner")]
    public partial class NetworkIdentityComponent : Component
    {
    }
}
