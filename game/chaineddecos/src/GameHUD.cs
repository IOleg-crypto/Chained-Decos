using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class GameHUD : Script
{
    private float m_Timer = 0.0f;

    public override void OnUpdate(float deltaTime)
    {
        m_Timer += deltaTime;

        if (Input.IsKeyPressed(Key.R))
        {
            m_Timer = 0.0f;
            Log.Info("timer reset via R");
        }
    }

    public override void OnGUI()
    {
        TransformComponent? transform = Entity.GetComponent<TransformComponent>();
        float altitude = transform != null ? transform.Translation.Y : 0.0f;
        
        int hours = (int)(m_Timer / 3600.0f);
        int minutes = (int)((m_Timer - hours * 3600.0f) / 60.0f);
        int seconds = (int)(m_Timer) % 60;

        UI.Text($"Altitude: {altitude:F2}");
        UI.Text($"Time: {hours:D2}:{minutes:D2}:{seconds:D2}");
    }
}
}
