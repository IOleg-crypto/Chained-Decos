using System;

namespace Chained
{
    /// <summary>2D Sprite wrapper.</summary>
    [NativeProperty("TexturePath", "string", "SpriteComponent_GetTexturePath", "SpriteComponent_SetTexturePath")]
    [NativeProperty("Tint", "Vector4", "SpriteComponent_GetTint", "SpriteComponent_SetTint")]
    [NativeProperty("FlipX", "bool", "SpriteComponent_GetFlipX", "SpriteComponent_SetFlipX")]
    [NativeProperty("FlipY", "bool", "SpriteComponent_GetFlipY", "SpriteComponent_SetFlipY")]
    [NativeProperty("ZOrder", "int", "SpriteComponent_GetZOrder", "SpriteComponent_SetZOrder")]
    public partial class SpriteComponent : Component
    {
    }
}
