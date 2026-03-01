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

        // Shortcut hint (R to Reset)
        if (Input.IsKeyPressed((Key)82)) // KEY_R
        {
            m_Timer = 0.0f;
        }
    }

    // Note: CH_GUI in C++ is usually implemented via a specific callback.
    // If our Script class doesn't have OnGUI yet, we'll need to add it
    // and register it in ScriptEngine/SceneScripting.

    /*
    public override void OnGUI()
    {
        float altitude = Entity.Translation.Y;
        int hours = (int)(m_Timer / 3600.0f);
        int minutes = (int)((m_Timer - hours * 3600.0f) / 60.0f);
        int seconds = (int)(m_Timer) % 60;

        // ImGui calls here...
    }
    */
}
}
