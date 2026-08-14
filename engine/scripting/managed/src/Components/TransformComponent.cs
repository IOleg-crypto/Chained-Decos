using System;
using Coral.Managed.Interop;

namespace Chained
{
    /// <summary>Transform wrapper.</summary>
    [NativeProperty("Translation", "Vector3", "Transform_GetTranslation", "Transform_SetTranslation")]
    [NativeProperty("Rotation", "Vector3", "Transform_GetRotation", "Transform_SetRotation")]
    [NativeProperty("Scale", "Vector3", "Transform_GetScale", "Transform_SetScale")]
    public partial class TransformComponent : Component
    {
    }
}
