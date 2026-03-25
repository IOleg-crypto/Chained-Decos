using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach this script to a UI Button entity to load a target scene when clicked.
    /// Helpful for "Play", "Return to Menu", etc.
    /// </summary>
    public class LoadSceneButton : Script
    {
        public string TargetScene = "scenes/main_menu.chscene";

        public override void OnUpdate(float deltaTime)
        {
            if (Entity.HasComponent<ButtonControl>())
            {
                ButtonControl? btn = Entity.GetComponent<ButtonControl>();
                if (btn != null && btn.IsPressed)
                {
                    Log.Info($"Button pressed! Loading target scene: {TargetScene}");
                    Scene.LoadScene(TargetScene);
                }
            }
        }
    }
}
