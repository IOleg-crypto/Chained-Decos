using Chained;

namespace ChainedDecos.Scripts
{
    // Applied to any entity in the start scene. Loads and applies settings.cfg
    // on game startup so that resolution, fullscreen, VSync, AA, and audio
    // volumes are correct before the first frame renders.
    public class GameStartup : Script
    {
        public override void OnStart()
        {
            SettingsConfig.ApplyAll();
        }
    }
}
