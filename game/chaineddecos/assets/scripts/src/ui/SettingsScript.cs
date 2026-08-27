using System;
using System.Collections.Generic;
using System.IO;
using Chained;

namespace ChainedDecos.Scripts
{
    public class SettingsScript : Script
    {
        private bool m_WasApplied = false;
        private string m_ConfigPath = "";

        public override void OnStart()
        {
            m_ConfigPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "settings.cfg");
            LoadSettings();
            PopulateResolutions();
            PopulateAntiAliasing();
            InitFullscreen();
            InitVSync();
            InitAudioSliders();
        }

        // ── SettingsStorage ────────────────────────────────────────────────

        private static class SettingsStorage
        {
            public static Dictionary<string, string> Load(string path)
            {
                var dict = new Dictionary<string, string>();
                if (!File.Exists(path)) return dict;

                foreach (string line in File.ReadAllLines(path))
                {
                    string trimmed = line.Trim();
                    if (string.IsNullOrEmpty(trimmed) || trimmed.StartsWith("#")) continue;

                    int eq = trimmed.IndexOf('=');
                    if (eq > 0)
                    {
                        string key = trimmed.Substring(0, eq).Trim();
                        string value = trimmed.Substring(eq + 1).Trim();
                        dict[key] = value;
                    }
                }
                return dict;
            }

            public static void Save(string path, Dictionary<string, string> settings)
            {
                using (var writer = new StreamWriter(path))
                {
                    foreach (var kv in settings)
                        writer.WriteLine($"{kv.Key}={kv.Value}");
                }
            }
        }

        // ── Load ───────────────────────────────────────────────────────────

        private void LoadSettings()
        {
            var cfg = SettingsStorage.Load(m_ConfigPath);
            if (cfg.Count == 0) return;

            if (cfg.TryGetValue("AntiAliasingSamples", out string aaStr) && int.TryParse(aaStr, out int aa))
                AppWindow.SetAntiAliasingSamples(aa);

            if (cfg.TryGetValue("Fullscreen", out string fsStr) && bool.TryParse(fsStr, out bool fs))
                AppWindow.SetFullscreen(fs);

            if (cfg.TryGetValue("VSync", out string vsStr) && bool.TryParse(vsStr, out bool vs))
                AppWindow.SetVSync(vs);

            if (cfg.TryGetValue("Resolution", out string resStr))
            {
                string[] parts = resStr.Split('x');
                if (parts.Length == 2 && int.TryParse(parts[0], out int w) && int.TryParse(parts[1], out int h))
                    AppWindow.SetSize(w, h);
            }

            if (cfg.TryGetValue("MasterVolume", out string mvStr) && float.TryParse(mvStr, System.Globalization.CultureInfo.InvariantCulture, out float mv))
                Audio.SetMasterVolume(mv);

            if (cfg.TryGetValue("MusicVolume", out string musStr) && float.TryParse(musStr, System.Globalization.CultureInfo.InvariantCulture, out float mus))
                Audio.SetMusicVolume(mus);

            if (cfg.TryGetValue("SFXVolume", out string sfxStr) && float.TryParse(sfxStr, System.Globalization.CultureInfo.InvariantCulture, out float sfx))
                Audio.SetSFXVolume(sfx);
        }

        // ── Save ───────────────────────────────────────────────────────────

        private void SaveSettings()
        {
            var cfg = new Dictionary<string, string>();

            Entity? resEnt = Scene.FindEntityByTag("resolution");
            ComboBoxControl? resCombo = resEnt?.GetComponent<ComboBoxControl>();
            if (resCombo != null && resCombo.ItemCount > 0)
            {
                string? resStr = resCombo.GetItem(resCombo.SelectedIndex);
                if (!string.IsNullOrEmpty(resStr))
                    cfg["Resolution"] = resStr;
            }

            cfg["Fullscreen"] = GetCheckboxState("option_fullscreen").ToString();
            cfg["VSync"] = GetCheckboxState("option_vsync").ToString();

            Entity? aaEnt = Scene.FindEntityByTag("anti_aliasing_combobox");
            ComboBoxControl? aaCombo = aaEnt?.GetComponent<ComboBoxControl>();
            if (aaCombo != null)
            {
                int[] aaValues = { 0, 2, 4, 8, 16 };
                int idx = aaCombo.SelectedIndex;
                if (idx >= 0 && idx < aaValues.Length)
                    cfg["AntiAliasingSamples"] = aaValues[idx].ToString();
            }

            cfg["MasterVolume"] = GetSliderValue("volume_master").ToString("F1", System.Globalization.CultureInfo.InvariantCulture);
            cfg["MusicVolume"] = GetSliderValue("volume_music").ToString("F1", System.Globalization.CultureInfo.InvariantCulture);
            cfg["SFXVolume"] = GetSliderValue("volume_sfx").ToString("F1", System.Globalization.CultureInfo.InvariantCulture);

            SettingsStorage.Save(m_ConfigPath, cfg);
            Log.Info("Settings saved to " + m_ConfigPath);
        }

        // ── UI Init ────────────────────────────────────────────────────────

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

            string currentRes = $"{AppWindow.GetWidth()}x{AppWindow.GetHeight()}";
            for (int i = 0; i < combo.ItemCount; i++)
            {
                if (combo.GetItem(i) == currentRes)
                {
                    combo.SelectedIndex = i;
                    return;
                }
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
            combo.AddItem("16x MSAA");

            int current = AppWindow.GetAntiAliasingSamples();
            int[] aaValues = { 0, 2, 4, 8, 16 };
            for (int i = 0; i < aaValues.Length; i++)
            {
                if (aaValues[i] == current)
                {
                    combo.SelectedIndex = i;
                    return;
                }
            }
            combo.SelectedIndex = 2;
        }

        private void InitFullscreen()
        {
            Entity? fullEnt = Scene.FindEntityByTag("option_fullscreen");
            CheckboxControl? check = fullEnt?.GetComponent<CheckboxControl>();
            if (check != null)
                check.IsChecked = AppWindow.GetFullscreen();
        }

        private void InitVSync()
        {
            Entity? vsyncEnt = Scene.FindEntityByTag("option_vsync");
            CheckboxControl? check = vsyncEnt?.GetComponent<CheckboxControl>();
            if (check != null)
                check.IsChecked = AppWindow.GetVSync();
        }

        private void InitAudioSliders()
        {
            SetSliderValue("volume_master", Audio.GetMasterVolume());
            SetSliderValue("volume_music", Audio.GetMusicVolume());
            SetSliderValue("volume_sfx", Audio.GetSFXVolume());
        }

        // ── Helpers ────────────────────────────────────────────────────────

        private bool GetCheckboxState(string tag)
        {
            Entity? ent = Scene.FindEntityByTag(tag);
            CheckboxControl? check = ent?.GetComponent<CheckboxControl>();
            return check != null && check.IsChecked;
        }

        private float GetSliderValue(string tag)
        {
            Entity? ent = Scene.FindEntityByTag(tag);
            SliderControl? slider = ent?.GetComponent<SliderControl>();
            return slider != null ? slider.Value : 1.0f;
        }

        private void SetSliderValue(string tag, float normalized)
        {
            Entity? ent = Scene.FindEntityByTag(tag);
            SliderControl? slider = ent?.GetComponent<SliderControl>();
            if (slider != null)
                slider.Value = normalized * 100f;
        }

        // ── Apply ──────────────────────────────────────────────────────────

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
                            AppWindow.SetSize(w, h);
                    }
                }
            }

            // 2. Fullscreen (Checkbox)
            AppWindow.SetFullscreen(GetCheckboxState("option_fullscreen"));

            // 3. VSync (Checkbox)
            AppWindow.SetVSync(GetCheckboxState("option_vsync"));

            // 4. Anti-aliasing (ComboBox)
            Entity? aaEnt = Scene.FindEntityByTag("anti_aliasing_combobox");
            if (aaEnt != null)
            {
                ComboBoxControl? combo = aaEnt.GetComponent<ComboBoxControl>();
                if (combo != null && combo.ItemCount > 0)
                {
                    int[] aaValues = { 0, 2, 4, 8, 16 };
                    int idx = combo.SelectedIndex;
                    if (idx >= 0 && idx < aaValues.Length)
                        AppWindow.SetAntiAliasingSamples(aaValues[idx]);
                }
            }

            // 5. Audio volumes (Sliders: 0–100 → 0.0–1.0)
            Audio.SetMasterVolume(GetSliderValue("volume_master") / 100f);
            Audio.SetMusicVolume(GetSliderValue("volume_music") / 100f);
            Audio.SetSFXVolume(GetSliderValue("volume_sfx") / 100f);

            // 6. Save to config file
            SaveSettings();
        }
    }
}
