using Chained;

namespace ChainedDecos.Scripts
{
    /// <summary>
    /// Attach directly to a skin swatch button in the lobby. Set <see cref="SkinIndex"/>
    /// per button in the scene; clicking it selects that skin and tells the host.
    /// </summary>
    public class SelectSkinButton : Script
    {
        public int SkinIndex = 0;

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null || !btn.IsClicked)
                return;

            LobbyManager.SelectedSkinIndex = SkinIndex;
            // SetLocalPlayerInfo updates the host's own list entry and re-broadcasts;
            // SendPlayerInfo is the client-side path and a no-op on the host.
            Network.SetLocalPlayerInfo(PlayerSettings.Nickname, (byte)SkinIndex);
            Network.SendPlayerInfo(PlayerSettings.Nickname, (byte)SkinIndex);
            Log.Info($"[SelectSkinButton] Selected skin {SkinIndex} ({SkinDatabase.GetName(SkinIndex)})");
        }
    }
}
