using System;
using Coral.Managed.Interop;

namespace Chained
{

/// <summary>Transform wrapper.</summary>
public class TransformComponent : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_GetTranslation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_SetTranslation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_GetRotation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_SetRotation_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_GetScale_Ptr;
    internal static unsafe delegate* unmanaged<ulong, Vector3*, void> Transform_SetScale_Ptr;
#pragma warning restore 0649

    public Vector3 Translation
    {
        get { unsafe { Vector3 translation; Transform_GetTranslation_Ptr(Entity.ID, &translation); return translation; } }
        set { unsafe { Transform_SetTranslation_Ptr(Entity.ID, &value); } }
    }

    public Vector3 Rotation
    {
        get { unsafe { Vector3 rotation; Transform_GetRotation_Ptr(Entity.ID, &rotation); return rotation; } }
        set { unsafe { Transform_SetRotation_Ptr(Entity.ID, &value); } }
    }

    public Vector3 Scale
    {
        get { unsafe { Vector3 scale; Transform_GetScale_Ptr(Entity.ID, &scale); return scale; } }
        set { unsafe { Transform_SetScale_Ptr(Entity.ID, &value); } }
    }
}

}
