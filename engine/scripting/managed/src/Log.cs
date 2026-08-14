using System;
using System.Collections.Generic;

namespace Chained
{
    public static class Log
    {
        private static List<string> s_History = new List<string>();
        public static IReadOnlyList<string> History => s_History;

        internal static unsafe delegate* unmanaged<char*, void> Log_Info_Ptr;
        internal static unsafe delegate* unmanaged<char*, void> Log_Warn_Ptr;
        internal static unsafe delegate* unmanaged<char*, void> Log_Error_Ptr;

        public static unsafe void Info(string message) 
        {
            if (message == null) return;
            AddToHistory("[INFO] " + message);

            if (Log_Info_Ptr != null)
            {
                
                
                fixed (char* ptr = message)
                {
                    Log_Info_Ptr(ptr);
                }
            }
            else
            {
                Console.WriteLine("[INFO] " + message);
            }
        }

        public static unsafe void Warn(string message) 
        {
            if (message == null) return;
            AddToHistory("[WARN] " + message);

            if (Log_Warn_Ptr != null)
            {
                fixed (char* ptr = message) Log_Warn_Ptr(ptr);
            }
            else Console.WriteLine("[WARN] " + message);
        }

        public static unsafe void Error(string message) 
        {
            if (message == null) return;
            AddToHistory("[ERROR] " + message);

            if (Log_Error_Ptr != null)
            {
                fixed (char* ptr = message) Log_Error_Ptr(ptr);
            }
            else Console.WriteLine("[ERROR] " + message);
        }

        private static void AddToHistory(string formatted)
        {
            if (s_History == null) s_History = new List<string>();
            s_History.Add(formatted);
            if (s_History.Count > 15) s_History.RemoveAt(0);
        }

        public static void ClearHistory() => s_History?.Clear();
    }
}