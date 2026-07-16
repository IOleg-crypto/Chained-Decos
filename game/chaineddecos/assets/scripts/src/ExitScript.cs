using System;
using Chained;

namespace ChainedDecos.Scripts
{
public class ExitScript : Script
{
    public override void OnUpdate(float deltaTime)
    {
        ButtonControl? btn = Entity.GetComponent<ButtonControl>();
        if (btn != null)
        {
            if (btn.IsClicked)
            {
                Log.Info($"[ExitScript] btn.IsDown is TRUE on frame update (deltaTime: {deltaTime}). Calling Application.Close().");
                Application.Close();
            }
        }
        else
        {
            Log.Warn("[ExitScript] ButtonControl is null!");
        }
    }
}
}
