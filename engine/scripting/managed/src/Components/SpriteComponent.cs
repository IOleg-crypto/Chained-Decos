using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>2D Sprite wrapper.</summary>
    public class SpriteComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, char*> SpriteComponent_GetTexturePath_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, void> SpriteComponent_SetTexturePath_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector4*, void> SpriteComponent_GetTint_Ptr;
        internal static unsafe delegate* unmanaged<ulong, Vector4, void> SpriteComponent_SetTint_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> SpriteComponent_GetFlipX_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte, void> SpriteComponent_SetFlipX_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> SpriteComponent_GetFlipY_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte, void> SpriteComponent_SetFlipY_Ptr;
        internal static unsafe delegate* unmanaged<ulong, int> SpriteComponent_GetZOrder_Ptr;
        internal static unsafe delegate* unmanaged<ulong, int, void> SpriteComponent_SetZOrder_Ptr;
#pragma warning restore 0649

        public string TexturePath
        {
            get { unsafe { return Marshal.PtrToStringUni(new IntPtr(SpriteComponent_GetTexturePath_Ptr(Entity.ID))) ?? string.Empty; } }
            set { unsafe { if (SpriteComponent_SetTexturePath_Ptr != null) fixed (char* ptr = value) SpriteComponent_SetTexturePath_Ptr(Entity.ID, ptr); } }
        }

        public Vector4 Tint
        {
            get { unsafe { Vector4 v; SpriteComponent_GetTint_Ptr(Entity.ID, &v); return v; } }
            set { unsafe { SpriteComponent_SetTint_Ptr(Entity.ID, value); } }
        }

        public bool FlipX
        {
            get { unsafe { return SpriteComponent_GetFlipX_Ptr(Entity.ID) != 0; } }
            set { unsafe { SpriteComponent_SetFlipX_Ptr(Entity.ID, (byte)(value ? 1 : 0)); } }
        }

        public bool FlipY
        {
            get { unsafe { return SpriteComponent_GetFlipY_Ptr(Entity.ID) != 0; } }
            set { unsafe { SpriteComponent_SetFlipY_Ptr(Entity.ID, (byte)(value ? 1 : 0)); } }
        }

        public int ZOrder
        {
            get { unsafe { return SpriteComponent_GetZOrder_Ptr(Entity.ID); } }
            set { unsafe { SpriteComponent_SetZOrder_Ptr(Entity.ID, value); } }
        }
    }
}
