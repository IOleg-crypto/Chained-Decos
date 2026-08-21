using System;

namespace Chained
{
    /// <summary>
    /// Declares a C++ internal call bound through Coral. The NativeCall source generator
    /// turns each attribute into an `internal static unsafe delegate* unmanaged<...> <Method>_Ptr`
    /// field (so the field is no longer hand-written) and registers it in the native-call
    /// manifest consumed by the load-time ABI self-check (Interop.VerifyInternalCalls).
    ///
    /// Signature is given as strings: [returnType, param1, param2, ...].
    /// e.g. [NativeCall("Chained.AnimationComponent", "AnimationComponent_GetIsLooping", "byte", "ulong")].
    /// Pointer types are written literally ("Vector3*", "char*").
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Property | AttributeTargets.Method | AttributeTargets.Field, AllowMultiple = true)]
    public sealed class NativeCallAttribute : Attribute
    {
        public string ClassName { get; }
        public string MethodName { get; }
        public string[] Signature { get; }

        public NativeCallAttribute(string className, string methodName, params string[] signature)
        {
            ClassName = className;
            MethodName = methodName;
            Signature = signature;
        }
    }

    /// <summary>
    /// Generates both the C++ internal call function pointer (_Ptr field) AND the
    /// C# property getter/setter implementation on a component.
    ///
    /// Usage:
    ///   [NativeProperty("MovementSpeed", "float", "PlayerComponent_GetMovementSpeed", "PlayerComponent_SetMovementSpeed")]
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public sealed class NativePropertyAttribute : Attribute
    {
        public string PropertyName { get; }
        public string PropertyType { get; }
        public string? GetterMethod { get; }
        public string? SetterMethod { get; }

        public NativePropertyAttribute(string propertyName, string propertyType, string? getterMethod = null, string? setterMethod = null)
        {
            PropertyName = propertyName;
            PropertyType = propertyType;
            GetterMethod = getterMethod;
            SetterMethod = setterMethod;
        }
    }
}

