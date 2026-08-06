using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Host waiting-room overlay: keeps the "player_count" label in sync.
    /// The Start Game button carries its own <see cref="StartGameButton"/> script;
    /// the selected map lives in <see cref="LobbyManager.SelectedMap"/>.
    /// </summary>
    public class HostWaitingUI : Script
    {
        private float m_RefreshTimer = 0.0f;

        public override void OnCreate()
        {
            Log.Info("HostWaitingUI: Initialized");
        }

        public override void OnUpdate(float deltaTime)
        {
            // Update player count display
            m_RefreshTimer += deltaTime;
            if (m_RefreshTimer >= 0.5f)
            {
                m_RefreshTimer = 0.0f;
                UpdatePlayerCount();
            }
        }

        private void UpdatePlayerCount()
        {
            Entity? label = Scene.FindEntityByTag("player_count");
            if (label != null && label.HasComponent<LabelControl>())
            {
                LabelControl? lbl = label.GetComponent<LabelControl>();
                if (lbl != null)
                {
                    int count = Network.ClientCount;
                    lbl.Text = count == 0
                        ? "Waiting for players..."
                        : $"{count} player(s) connected";
                }
            }
        }
    }
}
