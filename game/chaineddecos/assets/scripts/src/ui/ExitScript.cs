using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class ExitScript : Script
    {
        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn != null && btn.IsClicked)
            {
                Log.Info("Exit button clicked");
                Application.Close();
            }
        }
    }
}
