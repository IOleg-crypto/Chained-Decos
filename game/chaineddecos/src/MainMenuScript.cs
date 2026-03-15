using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public class MainMenuScript : Script
    {
        public string TargetScene = "scenes/untitled100.chscene";

        public override void OnUpdate(float deltaTime)
        {
            if (Entity.HasComponent<ButtonControl>())
            {
                ButtonControl btn = Entity?.GetComponent<ButtonControl>();
                if (btn.IsPressed)
                {
                    Log.Info("Starting game, loading scene: " + TargetScene);
                    Scene.LoadScene(TargetScene);
                }
            }
        }
    }
}
