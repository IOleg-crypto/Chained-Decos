using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to "Start Game" button entity.
    /// Broadcasts scene change to all clients and loads the selected map on the host.
    /// </summary>
    public class StartGameButton : Script
    {
        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null || !btn.IsClicked)
                return;

            string map = LobbyManager.SelectedMap;
            int playerCount = Network.ClientCount;
            Log.Info($"[StartGameButton] Starting game with {playerCount} client(s), map: {map}");

            Network.BroadcastSceneChange(map);
            Scene.LoadScene(map);
        }
    }
}
