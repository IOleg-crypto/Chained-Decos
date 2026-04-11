using Coral.Managed.Interop;

namespace CHEngine
{

/// <summary>Logging helpers.</summary>
public static class Log
{
#pragma warning disable 0649
    internal static unsafe delegate*<NativeString, void> Log_Info_Ptr;
    internal static unsafe delegate*<NativeString, void> Log_Warn_Ptr;
    internal static unsafe delegate*<NativeString, void> Log_Error_Ptr;
#pragma warning restore 0649

    /// <summary>Logs an info message.</summary>
    public static unsafe void Info(string message) => Log_Info_Ptr(message);
    /// <summary>Logs a warning.</summary>
    public static unsafe void Warn(string message) => Log_Warn_Ptr(message);
    /// <summary>Logs an error.</summary>
    public static unsafe void Error(string message) => Log_Error_Ptr(message);
}

}
 // namespace CHEngine
