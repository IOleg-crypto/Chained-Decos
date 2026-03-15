using Coral.Managed.Interop;

namespace CHEngine
{

public static class Log
{
#pragma warning disable 0649
    internal static unsafe delegate*<NativeString, void> Log_Info_Ptr;
    internal static unsafe delegate*<NativeString, void> Log_Warn_Ptr;
    internal static unsafe delegate*<NativeString, void> Log_Error_Ptr;
#pragma warning restore 0649

    public static unsafe void Info(string message)  => Log_Info_Ptr(message);
    public static unsafe void Warn(string message)  => Log_Warn_Ptr(message);
    public static unsafe void Error(string message) => Log_Error_Ptr(message);
}

}
 // namespace CHEngine
