using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach this script to a UI Button entity to load a target scene when clicked.
    /// Helpful for "Play", "Return to Menu", etc.
    /// </summary>
    public class LoadSceneButton : Script
    {
        public string TargetScene = "scenes/start_menu.chscene";

        public override void OnUpdate(float deltaTime)
        {
            if (Entity.HasComponent<ButtonControl>())
            {
                ButtonControl? btn = Entity.GetComponent<ButtonControl>();
                if (btn != null && btn.IsClicked)
                {
                    Scene.LoadScene(TargetScene);
                }
            }
        }
    }
}
