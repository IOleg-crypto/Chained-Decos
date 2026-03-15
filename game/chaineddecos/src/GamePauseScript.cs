using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public class GamePauseScript : Script
    {
        public string MenuScene = "scenes/main_menu.chscene";

        public override void OnUpdate(float deltaTime)
        {
            if (Input.IsKeyDown(Key.Escape))
            {
                Log.Info("Returning to main menu...");
                Scene.LoadScene(MenuScene);
            }
        }
    }
}
