using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class ExitScript : Script
{
    public override void OnUpdate(float deltaTime)
    {
        ButtonControl btn = Entity.GetComponent<ButtonControl>();
        if (btn != null)
        {
            if (btn.IsPressed)
            {
                Application.Close();
            }
        }
    }
}
}
