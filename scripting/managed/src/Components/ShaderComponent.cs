using System;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>Shader control wrapper.</summary>
    public class ShaderComponent : Component
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<ulong, char*, float, void> Shader_SetFloat_Ptr;
        internal static unsafe delegate* unmanaged<ulong, char*, Vector3*, void> Shader_SetVec3_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte> Shader_GetEnabled_Ptr;
        internal static unsafe delegate* unmanaged<ulong, byte, void> Shader_SetEnabled_Ptr;
#pragma warning restore 0649

        public bool Enabled
        {
            get { unsafe { return Shader_GetEnabled_Ptr(Entity.ID) != 0; } }
            set { unsafe { Shader_SetEnabled_Ptr(Entity.ID, (byte)(value ? 1 : 0)); } }
        }

        public unsafe void SetFloat(string name, float value)
        {
            if (Shader_SetFloat_Ptr == null) return;
            fixed (char* ptr = name) Shader_SetFloat_Ptr(Entity.ID, ptr, value);
        }

        public unsafe void SetVector3(string vname, Vector3 value)
        {
            if (Shader_SetVec3_Ptr == null) return;
            Vector3 v = value;
            fixed (char* ptr = vname) Shader_SetVec3_Ptr(Entity.ID, ptr, &v);
        }
    }
}
