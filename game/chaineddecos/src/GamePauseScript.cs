using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
    public class GamePauseScript : Script
    {
        public string MenuScene = "scenes/start_menu.chscene";

        public override void OnUpdate(float deltaTime)
        {
            if (Input.IsKeyPressed(Key.Escape))
            {
                Log.Info("Returning to main menu...");
                Scene.LoadScene(MenuScene);
            }
        }
    }
}
