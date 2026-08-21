using System;

namespace Chained
{
    /// <summary>Player component wrapper.</summary>
    [NativeProperty("MovementSpeed", "float", "PlayerComponent_GetMovementSpeed", "PlayerComponent_SetMovementSpeed")]
    [NativeProperty("JumpForce", "float", "PlayerComponent_GetJumpForce", "PlayerComponent_SetJumpForce")]
    [NativeProperty("LookSensitivity", "float", "PlayerComponent_GetLookSensitivity", "PlayerComponent_SetLookSensitivity")]
    public partial class PlayerComponent : Component
    {
    }
}
