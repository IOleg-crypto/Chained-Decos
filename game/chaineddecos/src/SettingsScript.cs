using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class SettingsScript : Script
{
    public override void OnCreate()
    {
        Log.Info("settings_script: Initialized");
    }

    public override void OnUpdate(float deltaTime)
    {
        ButtonControl btn = Entity.GetComponent<ButtonControl>();
        if (btn != null)
        {
            if (btn.IsPressed)
            {
                Log.Info("settings_script: Button pressed, applying settings...");
                ApplySettings();
            }
        }
    }

    private void ApplySettings()
    {
        Log.Info("settings_script: Applying Settings...");

        Entity resEnt = Scene.FindEntityByTag("resolution");
        Entity fullEnt = Scene.FindEntityByTag("option_fullscreen");
        Entity vsyncEnt = Scene.FindEntityByTag("option_vsync");
        Entity aaEnt = Scene.FindEntityByTag("option_aa");

        // 1. Resolution
        if (resEnt != null)
        {
            ComboBoxControl combo = resEnt.GetComponent<ComboBoxControl>();
            if (combo != null)
            {
                int index = combo.SelectedIndex;
                string resStr = combo.GetItem(index);
                if (!string.IsNullOrEmpty(resStr))
                {
                    string[] parts = resStr.Split('x');
                    if (parts.Length == 2)
                    {
                        int w = int.Parse(parts[0]);
                        int h = int.Parse(parts[1]);
                        AppWindow.SetSize(w, h);
                        Log.Info($"settings_script: Set Resolution to {w}x{h}");
                    }
                }
            }
        }

        // 2. Fullscreen
        if (fullEnt != null)
        {
            CheckboxControl check = fullEnt.GetComponent<CheckboxControl>();
            if (check != null)
            {
                bool enabled = check.IsChecked;
                AppWindow.SetFullscreen(enabled);
                Log.Info($"settings_script: Set Fullscreen to {enabled}");
            }
        }

        // 3. VSync
        if (vsyncEnt != null)
        {
            CheckboxControl check = vsyncEnt.GetComponent<CheckboxControl>();
            if (check != null)
            {
                bool enabled = check.IsChecked;
                AppWindow.SetVSync(enabled);
                Log.Info($"settings_script: Set VSync to {enabled}");
            }
        }

        // 4. Anti-aliasing
        if (aaEnt != null)
        {
            CheckboxControl check = aaEnt.GetComponent<CheckboxControl>();
            if (check != null)
            {
                bool enabled = check.IsChecked;
                AppWindow.SetAntialiasing(enabled);
                Log.Info($"settings_script: Set Anti-aliasing to {enabled}");
            }
        }

        Log.Info("settings_script: Settings Applied Successfully!");
    }
}
}
