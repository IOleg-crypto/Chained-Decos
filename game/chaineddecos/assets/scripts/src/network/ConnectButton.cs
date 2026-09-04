using System;
using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach to the "Connect to Server" button entity.
    /// Reads IP/port from input fields, then calls Network.ConnectTo.
    /// For WAN targets without UPnP — also starts hole punch in parallel.
    /// </summary>
    public class ConnectButton : Script
    {
        public string IpInputTag   = "input_ip";
        public string PortInputTag = "input_port";
        public string NickInputTag = "input_nick";
        public string LobbyScene   = "scenes/lobby.chscene";
        public string DefaultIp    = "127.0.0.1";
        public ushort DefaultPort  = 7777;
        public float  ConnectTimeout = 15f;

        private bool   m_IsConnecting = false;
        private float  m_ConnectTimer = 0f;
        private string m_PendingIp    = "";
        private ushort m_PendingPort  = 0;

        public override void OnCreate()
        {
            Network.Disconnect();
            m_IsConnecting = false;
            m_ConnectTimer = 0f;
        }

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();

            // ── Poll connection state ─────────────────────────────────────
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

                // Allow cancel
                if (btn != null && btn.IsClicked)
                {
                    m_IsConnecting = false;
                    if (btn != null) btn.Label = "Connect to Server";
                    Network.Disconnect();
                    return;
                }

                return; // wait
            }

            if (btn == null || !btn.IsClicked)
                return;

            // ── Read inputs ───────────────────────────────────────────────
            string ip   = ReadText(IpInputTag, DefaultIp);
            ushort port = DefaultPort;

            string portText = ReadText(PortInputTag, "");
            bool explicitPortGiven = false;
            if (!string.IsNullOrWhiteSpace(portText))
            {
                if (ushort.TryParse(portText, out ushort parsedPort) && parsedPort > 0)
                {
                    port = parsedPort;
                    explicitPortGiven = true;
                }
                else
                {
                    ShowError($"Invalid port: '{portText}'");
                    return;
                }
            }

            // Only parse "IP:PORT" from IP field if port field wasn't explicitly given
            if (!explicitPortGiven && ip.Contains(":"))
            {
                (ip, port) = ParseAddressField(ip, port);
            }

            // Read nickname
            string nick = ReadText(NickInputTag, "");
            if (!string.IsNullOrWhiteSpace(nick))
                PlayerSettings.Nickname = nick.Trim();

            ShowError("");
            Network.SetLocalPlayerInfo(PlayerSettings.Nickname, (byte)LobbyManager.SelectedSkinIndex);

            Log.Info($"[ConnectButton] Connecting to {ip}:{port}");

            bool isLocal = ip.StartsWith("127.") || ip.StartsWith("192.168.")
                        || ip.StartsWith("10.")  || ip.StartsWith("172.")
                        || ip == "::1" || ip == "localhost";

            // For WAN without UPnP — start hole punch in parallel with ENet connect.
            // The hole punch socket opens the NAT pinhole; ENet then connects through it.
            if (!isLocal && !Network.IsUpnpAvailable)
            {
                Log.Info($"[ConnectButton] Starting hole punch to {ip}:{port} (parallel)");
                Network.StartHolePunch(ip, port);
            }

            LobbyManager.SelectedPort = port;
            Network.ConnectTo(ip, port);
            m_PendingIp   = ip;
            m_PendingPort = port;
            m_IsConnecting = true;
            m_ConnectTimer = 0f;
            if (btn != null) btn.Label = "Connecting...";
        }

        // ── Helpers ───────────────────────────────────────────────────────

        private string ReadText(string tag, string fallback)
        {
            Entity? e = Scene.FindEntityByTag(tag);
            if (e == null) return fallback;
            InputTextControl? input = e.GetComponent<InputTextControl>();
            return (input != null && !string.IsNullOrWhiteSpace(input.Text))
                ? input.Text.Trim()
                : fallback;
        }

        private static (string ip, ushort port) ParseAddressField(string raw, ushort fallbackPort)
        {
            if (raw.Contains(":"))
            {
                int colon = raw.LastIndexOf(':');
                string ipPart   = raw.Substring(0, colon).Trim();
                string portPart = raw.Substring(colon + 1).Trim();
                if (ushort.TryParse(portPart, out ushort p) && p > 0)
                    return (ipPart, p);
            }
            return (raw, fallbackPort);
        }

        private void ShowError(string message)
        {
            Entity? e = Scene.FindEntityByTag("label_error");
            if (e == null || !e.IsValid) return;
            LabelControl? lc = e.GetComponent<LabelControl>();
            if (lc != null) lc.Text = message;
        }
    }
}

