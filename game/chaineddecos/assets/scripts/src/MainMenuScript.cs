using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class MainMenuScript : Script
    {
        public string TargetScene = "";

        public override void OnCreate()
        {
            Log.Info($"MainMenuScript: Initialized, TargetScene={TargetScene}");
        }

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn != null && btn.IsPressed)
            {
                if (!string.IsNullOrEmpty(TargetScene))
                {
                    Log.Info($"MainMenuScript: Loading scene: {TargetScene}");
                    Scene.LoadScene(TargetScene);
                }
                else
                {
                    Log.Warn("MainMenuScript: TargetScene is empty!");
                }
            }
        }
    }
}
