using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class MainMenuScript : Script
    {
        public string TargetScene = "scenes/untitled100.chscene";

        public override void OnUpdate(float deltaTime)
        {
            if (Entity.HasComponent<ButtonControl>())
            {
                ButtonControl? btn = Entity.GetComponent<ButtonControl>();
                if (btn?.IsPressed ?? false)
                {
                    Log.Info("Starting game, loading scene: " + TargetScene);
                    Scene.LoadScene(TargetScene);
                }
            }
        }
    }
}
