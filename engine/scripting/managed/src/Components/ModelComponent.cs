using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Model/Mesh wrapper.</summary>
    public class ModelComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, char*> Model_GetModelPath_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, void> Model_SetModelPath_Ptr;
#pragma warning restore 0649

        public string ModelPath
        {
            get { unsafe { string? result = Marshal.PtrToStringUni(new IntPtr(Model_GetModelPath_Ptr(Entity.ID))); return result ?? string.Empty; } }
            set { unsafe { if (Model_SetModelPath_Ptr != null) fixed (char* ptr = value) Model_SetModelPath_Ptr(Entity.ID, ptr); } }
        }
    }
}
