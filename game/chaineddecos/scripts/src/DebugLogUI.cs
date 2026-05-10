using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public class DebugLogUI : Script
    {
        public bool ShowLogs = true;

        public override void OnUpdate(float deltaTime)
        {
            if (!ShowLogs) return;

            var history = Log.History;
            if (history.Count == 0) return;

            // Simple on-screen log using UI.Text
            // We draw from bottom to top or just a list
            for (int i = 0; i < history.Count; i++)
            {
                UI.Text(history[i]);
            }
        }
    }
}
