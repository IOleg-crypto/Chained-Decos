using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Tag wrapper.</summary>
    public class TagComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, char*> TagComponent_GetTag_Ptr;
#pragma warning restore 0649

        private static unsafe string GetTag_Native(ulong entityID) => Marshal.PtrToStringUni(new IntPtr(TagComponent_GetTag_Ptr(entityID))) ?? string.Empty;
        public string Tag => GetTag_Native(Entity.ID);
    }
}
