using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class MultiplayerMenuScript : Script
    {
        private string m_SelectedMap = "scenes/test_rpg.chscene";
        private string m_IpAddress = "127.0.0.1";
        private int m_Port = 7777;
        private string m_StatusMessage = "";

        private readonly string[] m_Maps = {
            "scenes/test_rpg.chscene",
            "scenes/test_platform_scene.chscene",
            "scenes/rpg_strategy_scene.chscene",
            "scenes/test_first_scene.chscene"
        };
        private readonly string[] m_MapNames = {
            "RPG World",
            "Platformer",
            "RPG Strategy",
            "First Scene"
        };
        private int m_SelectedMapIndex = 0;

        public override void OnCreate()
        {
            Log.Info("MultiplayerMenuScript: Initialized");
        }

        public override void OnUpdate(float deltaTime)
        {
        }

        public override void OnGUI()
        {
            UI.Text("=== MULTIPLAYER LOBBY ===");
            UI.Text("");

            if (Network.IsConnected)
            {
                // --- Connected: show status ---
                string role = Network.IsHost ? "Host" : "Client";
                UI.Text($"Connected as: {role}");
                UI.Text($"Clients: {Network.ClientCount}");
                UI.Text($"Map: {m_SelectedMap}");
                UI.Text("");

                if (UI.Button("Start Game"))
                {
                    if (Network.IsHost)
                    {
                        Scene.LoadScene(m_SelectedMap);
                    }
                }

                if (UI.Button("Disconnect"))
                {
                    Network.Disconnect();
                    m_StatusMessage = "Disconnected";
                }
            }
            else
            {
                // --- Map selection ---
                UI.Text("Select Map:");
                for (int i = 0; i < m_MapNames.Length; i++)
                {
                    string prefix = (i == m_SelectedMapIndex) ? "> " : "  ";
                    if (UI.Button($"{prefix}{m_MapNames[i]}"))
                    {
                        m_SelectedMapIndex = i;
                        m_SelectedMap = m_Maps[i];
                    }
                }
                UI.Text("");

                // --- Host ---
                UI.Text("--- Host Game ---");
                if (UI.Button("Create Server"))
                {
                    Network.HostGame((ushort)m_Port);
                    m_StatusMessage = "Server started!";
                    Scene.LoadScene(m_SelectedMap);
                }

                UI.Text("");
                UI.Text("--- Join Game ---");
                UI.Text($"IP: {m_IpAddress}  Port: {m_Port}");
                if (UI.Button("Connect to Server"))
                {
                    Network.ConnectTo(m_IpAddress, (ushort)m_Port);
                    m_StatusMessage = "Connecting...";
                    Scene.LoadScene(m_SelectedMap);
                }
            }

            UI.Text("");
            if (!string.IsNullOrEmpty(m_StatusMessage))
            {
                UI.Text(m_StatusMessage);
            }

            if (UI.Button("Back"))
            {
                Scene.LoadScene("scenes/game_mode_selection.chscene");
            }
        }
    }
}
