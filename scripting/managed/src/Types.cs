using System;
using System.Runtime.InteropServices;
using System.Collections;
using System.Collections.Generic;

namespace Chained
{
    [StructLayout(LayoutKind.Explicit, Size = 4)]
    public struct Bool32
    {
        [FieldOffset(0)] public uint Value;
        public static implicit operator Bool32(bool InValue) => new() { Value = InValue ? 1u : 0u };
        public static implicit operator bool(Bool32 InBool32) => InBool32.Value > 0;
    }

    [StructLayout(LayoutKind.Explicit, Size = 16)]
    public struct NativeString : IDisposable
    {
        [FieldOffset(0)] internal IntPtr m_NativeString;
        [FieldOffset(8)] private Bool32 m_IsDisposed;

        public void Dispose()
        {
            if (!m_IsDisposed)
            {
                if (m_NativeString != IntPtr.Zero)
                {
                    Marshal.FreeCoTaskMem(m_NativeString);
                    m_NativeString = IntPtr.Zero;
                }
                m_IsDisposed = true;
            }
            GC.SuppressFinalize(this);
        }

        public override string? ToString() => Marshal.PtrToStringAuto(m_NativeString);
        public static NativeString Null() => new NativeString() { m_NativeString = IntPtr.Zero };
        public static implicit operator NativeString(string? InString) => new() { m_NativeString = Marshal.StringToCoTaskMemAuto(InString) };
        public static implicit operator string?(NativeString InString) => Marshal.PtrToStringAuto(InString.m_NativeString);
    }

    [StructLayout(LayoutKind.Sequential, Size = 32, Pack = 8)]
    public struct NativeArray<T> : IDisposable where T : unmanaged
    {
        private IntPtr m_NativeArray;
        private IntPtr m_ArrayHandle;
        private int m_NativeLength;
        private Bool32 m_IsDisposed;

        public int Length => m_NativeLength;

        public void Dispose()
        {
            if (!m_IsDisposed && m_ArrayHandle == IntPtr.Zero)
            {
                Marshal.FreeHGlobal(m_NativeArray);
                m_IsDisposed = true;
            }
            GC.SuppressFinalize(this);
        }

        public unsafe T[] ToArray()
        {
            if (m_NativeArray == IntPtr.Zero || m_NativeLength <= 0) return Array.Empty<T>();
            var arr = new T[m_NativeLength];
            fixed (T* p = arr)
            {
                Buffer.MemoryCopy((void*)m_NativeArray, p, m_NativeLength * sizeof(T), m_NativeLength * sizeof(T));
            }
            return arr;
        }

        public unsafe T this[int index]
        {
            get => ((T*)m_NativeArray)[index];
            set => ((T*)m_NativeArray)[index] = value;
        }
    }
}
