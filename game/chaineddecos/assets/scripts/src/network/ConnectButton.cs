using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to the "Connect to Server" button entity.
    /// Reads IP from input_ip and port from input_port, then calls Network.ConnectTo.
    /// </summary>
    public class ConnectButton : Script
    {
        public string IpInputTag   = "input_ip";
        public string PortInputTag = "input_port";
        public string NickInputTag = "input_nick";
        public string LobbyScene = "scenes/lobby.chscene";
        public string DefaultIp    = "127.0.0.1";
        public ushort DefaultPort  = 7777;

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null || !btn.IsClicked)
                return;

            string ip = DefaultIp;
            ushort port = DefaultPort;

            Entity? ipEntity = Scene.FindEntityByTag(IpInputTag);
            if (ipEntity != null && ipEntity.HasComponent<InputTextControl>())
            {
                InputTextControl? input = ipEntity.GetComponent<InputTextControl>();
                if (input != null && !string.IsNullOrWhiteSpace(input.Text))
                    ip = input.Text.Trim();
            }

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

            Log.Info($"[ConnectButton] Connecting to {ip}:{port}");
            Network.SetLocalPlayerInfo(PlayerSettings.Nickname, (byte)LobbyManager.SelectedSkinIndex);
            Network.ConnectTo(ip, port);
            Scene.LoadScene(LobbyScene);
        }
    }
}
