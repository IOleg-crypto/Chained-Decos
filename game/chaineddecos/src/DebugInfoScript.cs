using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public class DebugInfoScript : Script
    {
        public Key ToggleKey = Key.F3;
        private bool m_ShowDebug = false;

        public override void OnUpdate(float ts)
        {
            if (Input.IsKeyPressed(ToggleKey))
            {
                m_ShowDebug = !m_ShowDebug;
            }
        }

        public override void OnGUI()
        {
            if (m_ShowDebug)
            {
                UI.Text($"FPS: {Time.FPS}");
                UI.Text($"DeltaTime: {Time.DeltaTime:F4}s");
            }
        }
    }
}
