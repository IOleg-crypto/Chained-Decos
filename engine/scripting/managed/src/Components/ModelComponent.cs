using System;

namespace Chained
{
    /// <summary>Model/Mesh wrapper.</summary>
    [NativeProperty("ModelPath", "string", "Model_GetModelPath", "Model_SetModelPath")]
    public partial class ModelComponent : Component
    {
    }
}
