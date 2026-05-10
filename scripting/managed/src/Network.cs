using System;
using System.Runtime.InteropServices;
using Coral.Managed.Interop;

namespace CHEngine
{
    public static class Network
    {
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ushort, bool> Network_Host_Ptr;
    internal static unsafe delegate* unmanaged<NativeString, bool> Network_Connect_Ptr;
    internal static unsafe delegate* unmanaged<void> Network_Disconnect_Ptr;
    internal static unsafe delegate* unmanaged<bool> Network_IsActive_Ptr;
    internal static unsafe delegate* unmanaged<bool> Network_IsServer_Ptr;
    internal static unsafe delegate* unmanaged<byte*, uint, bool, bool> Network_SendData_Ptr;
    internal static unsafe delegate* unmanaged<bool> Network_HasMessages_Ptr;
    internal static unsafe delegate* unmanaged<byte[]> Network_GetNextMessage_Ptr;
#pragma warning restore 0649

        public static unsafe bool Host(ushort port) => Network_Host_Ptr(port);
        public static unsafe bool Connect(string address) => Network_Connect_Ptr(address);
        public static unsafe void Disconnect() => Network_Disconnect_Ptr();
        public static unsafe bool IsActive() => Network_IsActive_Ptr();
        public static unsafe bool IsServer() => Network_IsServer_Ptr();
        
        public static unsafe bool SendData(byte[] data, bool reliable = true)
        {
            if (data == null || data.Length == 0) return false;
            fixed (byte* p = data)
                return Network_SendData_Ptr(p, (uint)data.Length, reliable);
        }

        public static unsafe bool HasMessages() => Network_HasMessages_Ptr();
        public static unsafe byte[] GetNextMessage() => Network_GetNextMessage_Ptr();
    }
}
