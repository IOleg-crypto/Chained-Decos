using System;

namespace Chained
{
    /// <summary>Shader control wrapper.</summary>
    [NativeProperty("Enabled", "bool", "Shader_GetEnabled", "Shader_SetEnabled")]
    [NativeCall("Chained.ShaderComponent", "Shader_SetFloat", "void", "ulong", "char*", "float")]
    [NativeCall("Chained.ShaderComponent", "Shader_SetVec3", "void", "ulong", "char*", "Vector3*")]
    public partial class ShaderComponent : Component
    {
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
