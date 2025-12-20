#include "SettingsScreens.h"
#include "../Menu.h"
#include "../settings/MenuSettingsController.h"
#include <imgui.h>

void VideoSettingsScreen::Render()
{
    if (!GetMenu())
        return;
    auto settings = GetMenu()->GetSettingsController();
    if (settings)
    {
        settings->RenderVideoSettings();
    }
}

void AudioSettingsScreen::Render()
{
    if (!GetMenu())
        return;
    auto settings = GetMenu()->GetSettingsController();
    if (settings)
    {
        settings->RenderAudioSettings();
    }
}

void ControlSettingsScreen::Render()
{
    if (!GetMenu())
        return;
    auto settings = GetMenu()->GetSettingsController();
    if (settings)
    {
        settings->RenderControlSettings();
    }
}

