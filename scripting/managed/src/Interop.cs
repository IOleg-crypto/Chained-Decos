using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Chained
{
    /// <summary>
    /// Minimal interop: Coral fills delegate* fields via AddInternalCall automatically.
    /// C# -> C++ lifecycle callbacks are registered once through RegisterCallbacks().
    /// </summary>
    public static unsafe class Interop
    {
        // ── ManagedCallbacks struct (matches C++ ManagedCallbacks) ────
        [StructLayout(LayoutKind.Sequential)]
        public struct ManagedCallbacks
        {
            public delegate* unmanaged<float, void> OnUpdate;
            public delegate* unmanaged<int, void> OnEvent;
            public delegate* unmanaged<void> OnRenderUI;
            public delegate* unmanaged<ulong, ulong, void> OnCollisionEnter;
            public delegate* unmanaged<void> ClearAll;
            public delegate* unmanaged<ulong, char*, void> InstantiateScript;
            public delegate* unmanaged<ulong, char*, void> DestroyScript;
        }

        // Coral fills this via AddInternalCall (points to C++ RegisterManagedCallbacks)
        // Must be internal (non-public) - Coral uses BindingFlags.NonPublic to find delegate* fields.
        internal static delegate* unmanaged<ManagedCallbacks*, void> RegisterManagedCallbacks_Ptr;

        /// <summary>
        /// Called once from C++ after assemblies are loaded.
        /// NOT [UnmanagedCallersOnly] — invoked via managed InvokeStaticMethod.
        /// Creates a ManagedCallbacks struct with our lifecycle functions and passes it to C++.
        /// </summary>
        public static void RegisterCallbacks()
        {
            if (RegisterManagedCallbacks_Ptr == null)
            {
                Console.WriteLine("[C# Interop] ERROR: RegisterManagedCallbacks_Ptr is null - Coral did not fill it.");
                return;
            }

            var cb = new ManagedCallbacks();
            cb.OnUpdate           = &ScriptEngine.OnUpdate;
            cb.OnEvent            = &ScriptEngine.OnEvent;
            cb.OnRenderUI         = &ScriptEngine.OnRenderUI;
            cb.OnCollisionEnter   = &ScriptEngine.OnCollisionEnter;
            cb.ClearAll           = &ScriptEngine.ClearAll;
            cb.InstantiateScript  = &ScriptEngine.InstantiateScript;
            cb.DestroyScript      = &ScriptEngine.DestroyScript;

            RegisterManagedCallbacks_Ptr(&cb);
            Console.WriteLine("[C# Interop] ManagedCallbacks registered successfully.");
            VerifyInternalCalls();
        }

        /// <summary>
        /// Catches ABI/signature drift: any delegate* field suffixed with _Ptr that Coral
        /// failed to bind (name mismatch, signature mismatch) stays null. A null here means
        /// a C++ AddInternalCall has no matching C# field (or vice versa) — a silent crash
        /// waiting to happen. We surface it loudly at load time instead.
        /// </summary>
        private static void VerifyInternalCalls()
        {
            var unbound = new System.Collections.Generic.List<string>();
            var asm = typeof(Interop).Assembly;
            foreach (var type in asm.GetTypes())
            {
                foreach (var field in type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static))
                {
                    if (field.Name.EndsWith("_Ptr") && field.FieldType.IsFunctionPointer && field.GetValue(null) == null)
                        unbound.Add($"{type.Name}.{field.Name}");
                }
            }

            if (unbound.Count > 0)
                Console.WriteLine($"[C# Interop] ERROR: {unbound.Count} internal call(s) NOT bound by Coral: {string.Join(", ", unbound)}");
            else
                Console.WriteLine("[C# Interop] All internal calls bound successfully.");
        }
    }
}
