using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace ChainedEngine
{
    public class Transform
    {
        private readonly uint _entityID;

        internal Transform(uint entityID)
        {
             _entityID = entityID;
        }

        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetPosition(_entityID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetPosition(_entityID, ref value);
        }
    }

    public abstract class Entity
    {
        public readonly uint ID;
        public readonly Transform Transform;

        protected Entity(uint id)
        {
            ID = id;
            Transform = new Transform(id);
        }

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float deltaTime) { }

        // Internally called by C++ to create instances
        [UnmanagedCallersOnly]
        public static IntPtr CreateInstance(uint entityID, IntPtr classNamePtr)
        {
            string? className = Marshal.PtrToStringUni(classNamePtr);
            if (className == null) return IntPtr.Zero;

            Type? type = Type.GetType(className);
            if (type == null) return IntPtr.Zero;

            Entity? instance = Activator.CreateInstance(type, entityID) as Entity;
            if (instance == null) return IntPtr.Zero;

            return GCHandle.ToIntPtr(GCHandle.Alloc(instance));
        }

        [UnmanagedCallersOnly]
        public static void CallOnCreate(IntPtr handle)
        {
            Entity instance = (Entity)GCHandle.FromIntPtr(handle).Target!;
            instance.OnCreate();
        }

        [UnmanagedCallersOnly]
        public static void CallOnUpdate(IntPtr handle, float deltaTime)
        {
            Entity instance = (Entity)GCHandle.FromIntPtr(handle).Target!;
            instance.OnUpdate(deltaTime);
        }

        [UnmanagedCallersOnly]
        public static void DestroyInstance(IntPtr handle)
        {
            GCHandle gcHandle = GCHandle.FromIntPtr(handle);
            gcHandle.Free();
        }
    }

    public static class Log
    {
        public static void Info(string message) => InternalCalls.LogInfo?.Invoke(Marshal.StringToCoTaskMemUni(message));
        public static void Warning(string message) => InternalCalls.LogWarning?.Invoke(Marshal.StringToCoTaskMemUni(message));
        public static void Error(string message) => InternalCalls.LogError?.Invoke(Marshal.StringToCoTaskMemUni(message));
    }
}
