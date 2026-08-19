using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to a map-selection Button entity.
    /// When clicked, stores the SelectedMap for LobbyManager and optionally loads the next scene.
    /// </summary>
    public class SelectMapButton : Script
    {
        public string MapScene = "scenes/rpg_strategy_scene_mp.chscene";

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null || !btn.IsClicked)
                return;

            LobbyManager.SelectedMap = MapScene;
            Log.Info($"[SelectMapButton] Selected map: {MapScene}");
        }
    }
}
