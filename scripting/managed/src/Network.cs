using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>
    /// Static API for multiplayer networking.
    /// Provides HostGame, ConnectTo, Disconnect and status queries.
    /// </summary>
    public static class Network
    {
#pragma warning disable 0649

        internal static unsafe delegate* unmanaged<ushort, int, void> Network_HostGame_Ptr;
        internal static unsafe delegate* unmanaged<char*, ushort, void> Network_ConnectTo_Ptr;
        internal static unsafe delegate* unmanaged<void> Network_Disconnect_Ptr;
        internal static unsafe delegate* unmanaged<byte> Network_IsHost_Ptr;
        internal static unsafe delegate* unmanaged<byte> Network_IsClient_Ptr;
        internal static unsafe delegate* unmanaged<byte> Network_IsConnected_Ptr;
        internal static unsafe delegate* unmanaged<int> Network_GetClientCount_Ptr;
        internal static unsafe delegate* unmanaged<int> Network_GetRole_Ptr;

#pragma warning restore 0649

        /// <summary>Starts a listen server on the given port.</summary>
        public static unsafe void HostGame(ushort port = 7777, int maxClients = 4)
        {
            if (Network_HostGame_Ptr == null) return;
            Network_HostGame_Ptr(port, maxClients);
        }

        /// <summary>Connects to a remote server.</summary>
        public static unsafe void ConnectTo(string ip, ushort port = 7777)
        {
            if (Network_ConnectTo_Ptr == null || string.IsNullOrEmpty(ip)) return;
            fixed (char* ptr = ip) Network_ConnectTo_Ptr(ptr, port);
        }

        /// <summary>Disconnects from the current session.</summary>
        public static unsafe void Disconnect()
        {
            if (Network_Disconnect_Ptr == null) return;
            Network_Disconnect_Ptr();
        }

        /// <summary>True when this instance is the host (listen server).</summary>
        public static unsafe bool IsHost => Network_IsHost_Ptr != null && Network_IsHost_Ptr() != 0;

        /// <summary>True when this instance is a connected client.</summary>
        public static unsafe bool IsClient => Network_IsClient_Ptr != null && Network_IsClient_Ptr() != 0;

        /// <summary>True when connected (host or client).</summary>
        public static unsafe bool IsConnected => Network_IsConnected_Ptr != null && Network_IsConnected_Ptr() != 0;

        /// <summary>Number of connected clients (host-side only).</summary>
        public static unsafe int ClientCount => Network_GetClientCount_Ptr != null ? Network_GetClientCount_Ptr() : 0;

        /// <summary>Network role: 0=Offline, 1=Host, 2=Client.</summary>
        public static unsafe int GetRole() => Network_GetRole_Ptr != null ? Network_GetRole_Ptr() : 0;
    }
}
