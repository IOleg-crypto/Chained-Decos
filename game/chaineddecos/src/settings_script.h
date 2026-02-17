#ifndef CH_SETTINGS_SCRIPT_H
#define CH_SETTINGS_SCRIPT_H

#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include <string>
#include <vector>

namespace CHEngine
{
    CH_SCRIPT(SettingsScript)
    {
    public:
        CH_START()
        {
            CH_CORE_INFO("SettingsScript: Initialized");
        }

        CH_UPDATE(dt)
        {
            if (HasComponent<ButtonControl>())
            {
                auto& button = GetComponent<ButtonControl>();
                if (button.PressedThisFrame)
                {
                    CH_CORE_INFO("SettingsScript: Button '{}' pressed (Entity Tag: '{}')", button.Label, GetEntity().GetName());
                    ApplySettings();
                }
            }
        }

    private:
        void ApplySettings()
        {
            CH_CORE_INFO("SettingsScript: Applying Settings...");
            
            Entity resolutionEnt = FindEntityByTag("resolution");
            Entity fullscreenEnt = FindEntityByTag("option_fullscreen");
            Entity vsyncEnt = FindEntityByTag("option_vsync");

            auto& window = Application::Get().GetWindow();

            // 1. Resolution
            if (resolutionEnt && resolutionEnt.HasComponent<ComboBoxControl>())
            {
                auto& combo = resolutionEnt.GetComponent<ComboBoxControl>();
                if (combo.SelectedIndex >= 0 && combo.SelectedIndex < (int)combo.Items.size())
                {
                    std::string resStr = combo.Items[combo.SelectedIndex];
                    size_t xPos = resStr.find('x');
                    if (xPos != std::string::npos)
                    {
                        int w = std::stoi(resStr.substr(0, xPos));
                        int h = std::stoi(resStr.substr(xPos + 1));
                        window.SetSize(w, h);
                        CH_CORE_INFO("SettingsScript: Set Resolution to {}x{}", w, h);
                    }
                }
            }

            // 2. Fullscreen
            if (fullscreenEnt && fullscreenEnt.HasComponent<CheckboxControl>())
            {
                bool enabled = fullscreenEnt.GetComponent<CheckboxControl>().Checked;
                window.SetFullscreen(enabled);
                CH_CORE_INFO("SettingsScript: Set Fullscreen to {}", enabled);
            }

            // 3. VSync
            if (vsyncEnt && vsyncEnt.HasComponent<CheckboxControl>())
            {
                bool enabled = vsyncEnt.GetComponent<CheckboxControl>().Checked;
                window.SetVSync(enabled);
                CH_CORE_INFO("SettingsScript: Set VSync to {}", enabled);
            }
            
            CH_CORE_INFO("SettingsScript: Settings Applied Successfully!");
        }

        Entity FindEntityByTag(const std::string& tag)
        {
            auto view = GetScene()->GetRegistry().view<TagComponent>();
            for (auto entity : view)
            {
                if (view.get<TagComponent>(entity).Tag == tag)
                    return {entity, &GetScene()->GetRegistry()};
            }
            return {};
        }
    };
}

#endif // CH_SETTINGS_SCRIPT_H
