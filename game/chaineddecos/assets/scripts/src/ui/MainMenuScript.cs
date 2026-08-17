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
            if (btn != null && btn.IsClicked)
            {
                if (!string.IsNullOrEmpty(TargetScene))
                {
                    Scene.LoadScene(TargetScene);
                }
            }
        }
    }
}
