using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to a Button entity. Reads the port from input_port
    /// and a selected map tag, then calls Network.HostGame and loads the lobby scene.
    /// </summary>
    public class StartServerButton : Script
    {
        public string PortInputTag = "input_port";
        public string NickInputTag = "input_nick";
        public string LobbyScene = "scenes/lobby.chscene";
        public string PlayerPrefabPath = "prefab/player.chprefab";
        public ushort DefaultPort = 7777;

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null || !btn.IsClicked)
                return;

            ushort port = DefaultPort;

            Entity? portEntity = Scene.FindEntityByTag(PortInputTag);
            if (portEntity != null && portEntity.HasComponent<InputTextControl>())
            {
                InputTextControl? input = portEntity.GetComponent<InputTextControl>();
                if (input != null && ushort.TryParse(input.Text, out ushort parsed) && parsed > 0)
                    port = parsed;
            }

            Entity? nickEntity = Scene.FindEntityByTag(NickInputTag);
            if (nickEntity != null && nickEntity.HasComponent<InputTextControl>())
            {
                InputTextControl? input = nickEntity.GetComponent<InputTextControl>();
                if (input != null && !string.IsNullOrWhiteSpace(input.Text))
                    PlayerSettings.Nickname = input.Text.Trim();
            }

            Log.Info($"[StartServerButton] Hosting on port {port}");
            Network.SetLocalPlayerInfo(PlayerSettings.Nickname, (byte)LobbyManager.SelectedSkinIndex);
            Network.HostGame(port);

            LobbyManager.SelectedPort = port;

            Scene.LoadScene(LobbyScene);
        }
    }
}
