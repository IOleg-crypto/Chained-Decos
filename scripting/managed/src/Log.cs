using System.Collections.Generic;
using Coral.Managed.Interop;

namespace Chained
{

/// <summary>Logging helpers.</summary>
public static class Log
{
    private static List<string> s_History = new List<string>();
    public static IReadOnlyList<string> History => s_History;

#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<NativeString, void> Log_Info_Ptr;
    internal static unsafe delegate* unmanaged<NativeString, void> Log_Warn_Ptr;
    internal static unsafe delegate* unmanaged<NativeString, void> Log_Error_Ptr;
#pragma warning restore 0649

    /// <summary>Logs an info message.</summary>
    public static unsafe void Info(string message) 
    {
        AddToHistory("[INFO] " + message);
        Log_Info_Ptr(message);
    }
    
    /// <summary>Logs a warning.</summary>
    public static unsafe void Warn(string message) 
    {
        AddToHistory("[WARN] " + message);
        Log_Warn_Ptr(message);
    }
    
    /// <summary>Logs an error.</summary>
    public static unsafe void Error(string message) 
    {
        AddToHistory("[ERROR] " + message);
        Log_Error_Ptr(message);
    }

    private static void AddToHistory(string formatted)
    {
        s_History.Add(formatted);
        if (s_History.Count > 15) s_History.RemoveAt(0);
    }

    public static void ClearHistory() => s_History.Clear();
}

}
 // namespace Chained
