using System;
using Chained;

namespace ChainedDecos.Scripts
{
    public class SettingsScript : Script
    {
        private bool m_WasApplied = false;

        public override void OnStart()
        {
            PopulateResolutions();
            Log.Info("Resolutions populated");
            PopulateAntiAliasing();
            Log.Info("Anti-aliasing populated");
        }

        private void PopulateResolutions()
        {
            Entity? resEnt = Scene.FindEntityByTag("resolution");
            ComboBoxControl? combo = resEnt?.GetComponent<ComboBoxControl>();
            if (combo == null) return;

            combo.ClearItems();
            string resolutions = AppWindow.GetSupportedResolutions();
            if (string.IsNullOrEmpty(resolutions)) return;

            foreach (string res in resolutions.Split(';'))
            {
                if (!string.IsNullOrEmpty(res))
                    combo.AddItem(res);
            }
        }

        private void PopulateAntiAliasing()
        {
            Entity? aaEnt = Scene.FindEntityByTag("anti_aliasing_combobox");
            ComboBoxControl? combo = aaEnt?.GetComponent<ComboBoxControl>();
            if (combo == null) return;

            combo.ClearItems();
            combo.AddItem("None");
            combo.AddItem("2x MSAA");
            combo.AddItem("4x MSAA");
            combo.AddItem("8x MSAA");
            combo.SelectedIndex = 2;
        }

        public override void OnUpdate(float deltaTime)
        {
            ButtonControl? btn = Entity.GetComponent<ButtonControl>();
            if (btn == null) return;

            bool isPressed = btn.IsClicked;

            if (isPressed && !m_WasApplied)
            {
                m_WasApplied = true;
                ApplySettings();
                Log.Info("Settings applied");
            }
            else if (!btn.IsDown)
            {
                m_WasApplied = false;
            }
        }

        private void ApplySettings()
        {
            // 1. Resolution (ComboBox)
            Entity? resEnt = Scene.FindEntityByTag("resolution");
            if (resEnt != null)
            {
                ComboBoxControl? combo = resEnt.GetComponent<ComboBoxControl>();
                if (combo != null && combo.ItemCount > 0)
                {
                    string? resStr = combo.GetItem(combo.SelectedIndex);
                    if (!string.IsNullOrEmpty(resStr))
                    {
                        string[] parts = resStr.Split('x');
                        if (parts.Length == 2 && int.TryParse(parts[0], out int w) && int.TryParse(parts[1], out int h))
                        {
                            AppWindow.SetSize(w, h);
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
                    AppWindow.SetFullscreen(check.IsChecked);
            }

            // 3. VSync (Checkbox)
            Entity? vsyncEnt = Scene.FindEntityByTag("option_vsync");
            if (vsyncEnt != null)
            {
                CheckboxControl? check = vsyncEnt.GetComponent<CheckboxControl>();
                if (check != null)
                    AppWindow.SetVSync(check.IsChecked);
            }

            // 4. Anti-aliasing (ComboBox)
            Entity? aaEnt = Scene.FindEntityByTag("anti_aliasing_combobox");
            if (aaEnt != null)
            {
                ComboBoxControl? combo = aaEnt.GetComponent<ComboBoxControl>();
                if (combo != null && combo.ItemCount > 0)
                {
                    int[] aaValues = { 0, 2, 4, 8 };
                    int idx = combo.SelectedIndex;
                    if (idx >= 0 && idx < aaValues.Length)
                        AppWindow.SetAntiAliasingSamples(aaValues[idx]);
                }
            }
        }
    }
}
