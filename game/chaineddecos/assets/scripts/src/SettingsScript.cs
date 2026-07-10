using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class SettingsScript : Script
    {
        public override void OnCreate()
        {
            Log.Info("SettingsScript: Initialized");
        }

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn?.IsPressed ?? false)
            {
                Log.Info("SettingsScript: Apply button pressed, applying settings...");
                ApplySettings();
            }
        }

        private void ApplySettings()
        {
            // 1. Resolution (ComboBox)
            Entity? resEnt = Scene.FindEntityByTag("resolution");
            if (resEnt != null)
            {
                ComboBoxControl? combo = resEnt.GetComponent<ComboBoxControl>();
                if (combo != null)
                {
                    string? resStr = combo.GetItem(combo.SelectedIndex);
                    if (!string.IsNullOrEmpty(resStr))
                    {
                        string[] parts = resStr.Split('x');
                        if (parts.Length == 2)
                        {
                            int w = int.Parse(parts[0]);
                            int h = int.Parse(parts[1]);
                            AppWindow.SetSize(w, h);
                            Log.Info($"SettingsScript: Set Resolution to {w}x{h}");
                        }
                    }
                }
            }

            // 2. Fullscreen (Checkbox)
            Entity? fullEnt = Scene.FindEntityByTag("option_fullscreen");
            if (fullEnt != null)
            {
                CheckboxControl? check = fullEnt.GetComponent<CheckboxControl>();
                if (check != null)
                {
                    AppWindow.SetFullscreen(check.IsChecked);
                    Log.Info($"SettingsScript: Set Fullscreen to {check.IsChecked}");
                }
            }

            // 3. VSync (Checkbox)
            Entity? vsyncEnt = Scene.FindEntityByTag("option_vsync");
            if (vsyncEnt != null)
            {
                CheckboxControl? check = vsyncEnt.GetComponent<CheckboxControl>();
                if (check != null)
                {
                    AppWindow.SetVSync(check.IsChecked);
                    Log.Info($"SettingsScript: Set VSync to {check.IsChecked}");
                }
            }

            // 4. Anti-aliasing (Checkbox)
            Entity? aaEnt = Scene.FindEntityByTag("option_aa");
            if (aaEnt != null)
            {
                CheckboxControl? check = aaEnt.GetComponent<CheckboxControl>();
                if (check != null)
                {
                    AppWindow.SetAntialiasing(check.IsChecked);
                    Log.Info($"SettingsScript: Set Anti-aliasing to {check.IsChecked}");
                }
            }

            Log.Info("SettingsScript: Settings Applied Successfully!");
        }
    }
}
