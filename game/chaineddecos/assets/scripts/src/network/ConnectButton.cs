using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to the "Connect to Server" button entity.
    /// Reads IP from input_ip and port from input_port, then calls Network.ConnectTo.
    /// Polls for connection success/failure before loading lobby.
    /// </summary>
    public class ConnectButton : Script
    {
        public string IpInputTag   = "input_ip";
        public string PortInputTag = "input_port";
        public string NickInputTag = "input_nick";
        public string LobbyScene = "scenes/lobby.chscene";
        public string DefaultIp    = "127.0.0.1";
        public ushort DefaultPort  = 7777;
        public float  ConnectTimeout = 5f;

        private bool   m_IsConnecting = false;
        private float  m_ConnectTimer = 0f;
        private string m_PendingIp    = "";
        private ushort m_PendingPort  = 0;

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();

            // Poll connection state while connecting
            if (m_IsConnecting)
            {
                m_ConnectTimer += deltaTime;

                if (Network.IsFullyConnected)
                {
                    m_IsConnecting = false;
                    if (btn != null) btn.Label = "Connected!";
                    Log.Info($"[ConnectButton] Connected to {m_PendingIp}:{m_PendingPort}, loading lobby");
                    Scene.LoadScene(LobbyScene);
                    return;
                }

                if (m_ConnectTimer >= ConnectTimeout)
                {
                    m_IsConnecting = false;
                    if (btn != null) btn.Label = "Connect to Server";
                    Log.Warn($"[ConnectButton] Connection to {m_PendingIp}:{m_PendingPort} timed out");
                    Network.Disconnect();
                    ShowError($"Could not connect to {m_PendingIp}:{m_PendingPort} (Timed out)");
                    return;
                }

                return; // wait, don't process button clicks
            }

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

            // Smart check: If user pasted "IP:PORT" into the IP field, parse both
            if (ip.Contains(":"))
            {
                int colonIndex = ip.LastIndexOf(':');
                string ipPart = ip.Substring(0, colonIndex).Trim();
                string portPart = ip.Substring(colonIndex + 1).Trim();
                if (ushort.TryParse(portPart, out ushort extractedPort) && extractedPort > 0)
                {
                    port = extractedPort;
                    ip = ipPart;
                }
            }

            Entity? nickEntity = Scene.FindEntityByTag(NickInputTag);
            if (nickEntity != null && nickEntity.HasComponent<InputTextControl>())
            {
                InputTextControl? input = nickEntity.GetComponent<InputTextControl>();
                if (input != null && !string.IsNullOrWhiteSpace(input.Text))
                    PlayerSettings.Nickname = input.Text.Trim();
            }

            Log.Info($"[ConnectButton] Connecting to {ip}:{port}");
            ShowError(""); // clear previous error
            if (btn != null) btn.Label = "Connecting...";

            Network.SetLocalPlayerInfo(PlayerSettings.Nickname, (byte)LobbyManager.SelectedSkinIndex);
            Network.ConnectTo(ip, port);

            m_PendingIp = ip;
            m_PendingPort = port;
            m_ConnectTimer = 0f;
            m_IsConnecting = true;
        }

        private void ShowError(string message)
        {
            Entity? errorLabel = Scene.FindEntityByTag("label_error");
            if (errorLabel != null && errorLabel.IsValid)
            {
                LabelControl? lc = errorLabel.GetComponent<LabelControl>();
                if (lc != null)
                {
                    lc.Text = message;
                }
            }
        }
    }
}
