using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class SceneScript : Script
{
    public override void OnUpdate(float deltaTime)
    {
        ButtonControl btn = Entity.GetComponent<ButtonControl>();
        if (btn != null)
        {
            if (btn.IsPressed)
            {
                SceneTransitionComponent trans = Entity.GetComponent<SceneTransitionComponent>();
                string targetScene = trans?.TargetScene;
                if (string.IsNullOrEmpty(targetScene))
                {
                    targetScene = "scenes/main_menu.chscene";
                    Log.Warn("scenescript: No TargetScenePath found, using fallback: " + targetScene);
                }

                Log.Info("scenescript: Loading scene: " + targetScene);
                Scene.LoadScene(targetScene);
            }
        }
    }
}
}
