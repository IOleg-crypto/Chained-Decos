using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Primitive/geometry component with material properties.</summary>
    public class PrimitiveComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, Vector4*, void> PrimitiveComponent_GetAlbedoColor_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector4, void> PrimitiveComponent_SetAlbedoColor_Ptr;
#pragma warning restore 0649

        public Vector4 AlbedoColor
        {
            get
            {
                unsafe
                {
                    if (PrimitiveComponent_GetAlbedoColor_Ptr == null) return new Vector4(1, 1, 1, 1);
                    Vector4 color;
                    PrimitiveComponent_GetAlbedoColor_Ptr(Entity.ID, &color);
                    return color;
                }
            }
            set
            {
                unsafe { if (PrimitiveComponent_SetAlbedoColor_Ptr != null) PrimitiveComponent_SetAlbedoColor_Ptr(Entity.ID, value); }
            }
        }
    }
}
