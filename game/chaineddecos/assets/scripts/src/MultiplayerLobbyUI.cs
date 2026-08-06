using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class MultiplayerLobbyUI : Script
    {
        private string m_SelectedMap = "scenes/test_rpg.chscene";
        private ushort m_Port = 7777;
        private string m_IpAddress = "127.0.0.1";

        public override void OnCreate()
        {
            Log.Info("MultiplayerLobbyUI: Initialized");
        }

        public override void OnUpdate(float deltaTime)
        {
            // Map selection buttons
            CheckButton("btn_map_rpg",       () => SetSelectedMap("scenes/test_rpg.chscene"));
            CheckButton("btn_map_platformer", () => SetSelectedMap("scenes/test_platform_scene.chscene"));
            CheckButton("btn_map_strategy",  () => SetSelectedMap("scenes/rpg_strategy_scene.chscene"));
            CheckButton("btn_map_first",     () => SetSelectedMap("scenes/test_first_scene.chscene"));

            // Read Port from InputText (for hosting)
            ReadInputText("input_port", value =>
            {
                if (ushort.TryParse(value, out ushort port) && port > 0)
                    m_Port = port;
            });

            // Read IP from InputText (for connecting)
            ReadInputText("input_ip", value =>
            {
                if (!string.IsNullOrWhiteSpace(value))
                    m_IpAddress = value;
            });

            // Action buttons
            CheckButton("btn_host",    HostGame);
            CheckButton("btn_connect", ConnectGame);
        }

        private void SetSelectedMap(string mapPath)
        {
            m_SelectedMap = mapPath;
            Log.Info($"Selected Map: {m_SelectedMap}");
        }

        private void HostGame()
        {
            Log.Info($"Hosting game on port {m_Port}, loading {m_SelectedMap}");
            Network.HostGame(m_Port);

            // Attempt UPnP port forwarding (automatic if available)
            if (Network.IsPortForwardAvailable)
            {
                Log.Info($"Port forwarding via UPnP for port {m_Port}");
                Network.TryPortForward(m_Port);
            }
            else
            {
                Log.Info("UPnP not available — manual port forwarding required");
            }

            Scene.LoadScene(m_SelectedMap);
        }

        private void ConnectGame()
        {
            Log.Info($"Connecting to {m_IpAddress}:{m_Port}, loading {m_SelectedMap}");
            Network.ConnectTo(m_IpAddress, m_Port);
            Scene.LoadScene(m_SelectedMap);
        }

        private void CheckButton(string tag, Action onClick)
        {
            Entity? entity = Scene.FindEntityByTag(tag);
            if (entity != null && entity.HasComponent<ButtonControl>())
            {
                ButtonControl? btn = entity.GetComponent<ButtonControl>();
                if (btn != null && btn.IsClicked)
                {
                    onClick?.Invoke();
                }
            }
        }

        // Reads text from an InputTextControl by tag and calls callback when value changed.
        private void ReadInputText(string tag, Action<string> onChanged)
        {
            Entity? entity = Scene.FindEntityByTag(tag);
            if (entity != null && entity.HasComponent<InputTextControl>())
            {
                InputTextControl? input = entity.GetComponent<InputTextControl>();
                if (input != null && input.HasChanged)
                {
                    onChanged(input.Text);
                    input.ClearChanged();
                }
            }
        }
    }
}
