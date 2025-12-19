#include "MenuSettingsController.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <raylib.h>
#include <set>
#include <string>


MenuSettingsController::MenuSettingsController()
{
    // Initialize default options
    m_displayModeOptions = MenuConstants::DISPLAY_MODE_OPTIONS;
    m_vsyncOptions = MenuConstants::VSYNC_OPTIONS;

    // Format FPS options
    m_fpsOptions.clear();
    for (const auto &fps : MenuConstants::FPS_OPTIONS)
    {
        if (fps == "Unlimited")
            m_fpsOptions.push_back(fps);
        else
            m_fpsOptions.push_back(fps + " FPS");
    }
}

void MenuSettingsController::Initialize(SettingsManager *settingsManager,
                                        ICameraSensitivityController *cameraController)
{
    m_settingsManager = settingsManager;
    m_cameraController = cameraController;

    // Load current settings from SettingsManager
    if (m_settingsManager)
    {
        // 1. Fetch available monitor resolutions dynamically
        m_resolutionOptions.clear();
        std::set<std::pair<int, int>> resolutions;

        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (monitor)
        {
            int count = 0;
            const GLFWvidmode *modes = glfwGetVideoModes(monitor, &count);
            for (int i = 0; i < count; i++)
            {
                // Only include resolutions >= 800x600 to avoid tiny unplayable windows
                // And filter out duplicate resolutions (set handles this)
                if (modes[i].width >= 800 && modes[i].height >= 600)
                {
                    resolutions.insert({modes[i].width, modes[i].height});
                }
            }
        }

        // Add fallback if empty
        if (resolutions.empty())
        {
            resolutions.insert({1280, 720});
            resolutions.insert({1920, 1080});
        }

        // Convert to string options
        for (const auto &[w, h] : resolutions)
        {
            m_resolutionOptions.push_back(std::to_string(w) + "x" + std::to_string(h));
        }

        // 2. Find current resolution index
        int currentW, currentH;
        m_settingsManager->GetResolution(currentW, currentH);

        m_videoSettings.resolutionIndex = 0;
        std::string currentResStr = std::to_string(currentW) + "x" + std::to_string(currentH);

        // Try to match current resolution
        bool found = false;
        for (size_t i = 0; i < m_resolutionOptions.size(); ++i)
        {
            if (m_resolutionOptions[i] == currentResStr)
            {
                m_videoSettings.resolutionIndex = static_cast<int>(i);
                found = true;
                break;
            }
        }

        // If current resolution is not in the list (custom?), add it?
        // For now, let's stick to the list logic. If not found, it defaults to 0 (lowest/first).

        m_videoSettings.displayModeIndex = m_settingsManager->GetDisplayModeIndex();
        m_videoSettings.vsyncIndex = m_settingsManager->GetVSyncIndex();
        m_videoSettings.fpsIndex = m_settingsManager->GetFpsIndex();

        m_audioSettings.masterVolume = m_settingsManager->GetMasterVolume();
        m_audioSettings.musicVolume = m_settingsManager->GetMusicVolume();
        m_audioSettings.sfxVolume = m_settingsManager->GetSfxVolume();
        m_audioSettings.muted = m_settingsManager->IsMuted();

        m_controlSettings.mouseSensitivity = m_settingsManager->GetMouseSensitivity();
        m_controlSettings.invertYAxis = m_settingsManager->GetInvertYAxis();
        m_controlSettings.controllerSupport = m_settingsManager->GetControllerSupport();
    }
}

void MenuSettingsController::SetCameraController(ICameraSensitivityController *controller)
{
    m_cameraController = controller;
}

void MenuSettingsController::SetBackCallback(BackCallback callback)
{
    m_backCallback = std::move(callback);
}

void MenuSettingsController::RenderVideoSettings()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float labelWidth = 180.0f;
    const float comboWidth = 300.0f;
    const float startX = centerX - (labelWidth + comboWidth + 30.0f) / 2.0f;
    const float buttonSpacing = 140.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("VIDEO SETTINGS");
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);

    // Unsaved changes indicator
    bool hasUnsavedChanges = HasUnsavedVideoChanges();
    if (hasUnsavedChanges)
    {
        ImGui::SetCursorPos(ImVec2(centerX - 100.0f, MenuConstants::TOP_MARGIN - 20));
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "* Unsaved changes");
    }

    // Settings
    const float settingsStartY = MenuConstants::TOP_MARGIN + 80.0f;
    ImGui::SetCursorPos(ImVec2(startX, settingsStartY));

    // Resolution
    RenderVideoSettingCombo("Resolution", "##resolution", m_resolutionOptions,
                            m_videoSettings.resolutionIndex, labelWidth, comboWidth, startX);

    // Display Mode
    RenderVideoSettingCombo("Display Mode", "##displaymode", m_displayModeOptions,
                            m_videoSettings.displayModeIndex, labelWidth, comboWidth, startX);

    // VSync
    RenderVideoSettingCombo("VSync", "##vsync", m_vsyncOptions, m_videoSettings.vsyncIndex,
                            labelWidth, comboWidth, startX);

    // FPS Limit
    RenderVideoSettingCombo("FPS Limit", "##fps", m_fpsOptions, m_videoSettings.fpsIndex,
                            labelWidth, comboWidth, startX);

    // Buttons
    const float buttonY = windowSize.y - 80.0f;
    const float buttonGroupWidth = 120.0f + buttonSpacing + 120.0f;
    const float buttonStartX = centerX - buttonGroupWidth / 2.0f;
    ImGui::SetCursorPos(ImVec2(buttonStartX, buttonY));

    // Apply button
    ImGui::BeginDisabled(!hasUnsavedChanges);
    if (ImGui::Button("Apply", ImVec2(120, 40)))
    {
        SyncVideoSettingsToConfig();
        if (m_settingsManager)
        {
            m_settingsManager->SaveSettings();
            m_settingsManager->ApplyVideoSettings();
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine(buttonStartX + buttonSpacing);
    if (ImGui::Button("Back", ImVec2(120, 40)))
    {
        if (m_backCallback)
            m_backCallback();
    }
}

void MenuSettingsController::RenderAudioSettings()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float labelWidth = 180.0f;
    const float sliderWidth = 300.0f;
    const float startX = centerX - (labelWidth + sliderWidth + 30.0f) / 2.0f;
    const float spacing = 30.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, MenuConstants::TOP_MARGIN - 50));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "AUDIO SETTINGS");
    ImGui::SetWindowFontScale(1.0f);

    // Settings
    const float settingsStartY = MenuConstants::TOP_MARGIN + 60.0f;
    ImGui::SetCursorPos(ImVec2(startX, settingsStartY));

    // Master Volume
    ImGui::SetCursorPosX(startX);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Master Volume");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::SetNextItemWidth(sliderWidth);
    if (ImGui::SliderFloat("##master_volume", &m_audioSettings.masterVolume, 0.0f, 1.0f, "%.0f%%"))
    {
        m_audioSettings.masterVolume = m_audioSettings.masterVolume;
    }

    // Music Volume
    ImGui::SetCursorPosX(startX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Music Volume");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::SetNextItemWidth(sliderWidth);
    ImGui::SliderFloat("##music_volume", &m_audioSettings.musicVolume, 0.0f, 1.0f, "%.0f%%");

    // SFX Volume
    ImGui::SetCursorPosX(startX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "SFX Volume");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::SetNextItemWidth(sliderWidth);
    ImGui::SliderFloat("##sfx_volume", &m_audioSettings.sfxVolume, 0.0f, 1.0f, "%.0f%%");

    // Mute checkbox
    ImGui::SetCursorPosX(startX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Mute All");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::Checkbox("##mute", &m_audioSettings.muted);

    // Buttons
    const float buttonY = windowSize.y - 80.0f;
    const float buttonGroupWidth = 120.0f + 140.0f + 120.0f;
    const float buttonStartX = centerX - buttonGroupWidth / 2.0f;
    ImGui::SetCursorPos(ImVec2(buttonStartX, buttonY));

    if (ImGui::Button("Apply", ImVec2(120, 40)))
    {
        SyncAudioSettingsToConfig();
        if (m_settingsManager)
        {
            m_settingsManager->SaveSettings();
            m_settingsManager->ApplyAudioSettings();
        }
    }

    ImGui::SameLine(buttonStartX + 140.0f);
    if (ImGui::Button("Back", ImVec2(120, 40)))
    {
        if (m_backCallback)
            m_backCallback();
    }
}

void MenuSettingsController::RenderControlSettings()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float labelWidth = 180.0f;
    const float sliderWidth = 300.0f;
    const float startX = centerX - (labelWidth + sliderWidth + 30.0f) / 2.0f;
    const float spacing = 30.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, MenuConstants::TOP_MARGIN - 50));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "CONTROL SETTINGS");
    ImGui::SetWindowFontScale(1.0f);

    // Settings
    const float settingsStartY = MenuConstants::TOP_MARGIN + 60.0f;
    ImGui::SetCursorPos(ImVec2(startX, settingsStartY));

    // Mouse Sensitivity
    ImGui::SetCursorPosX(startX);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Mouse Sensitivity");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::SetNextItemWidth(sliderWidth);
    ImGui::SliderFloat("##mouse_sensitivity", &m_controlSettings.mouseSensitivity, 0.1f, 5.0f,
                       "%.2f");

    // Invert Y-Axis
    ImGui::SetCursorPosX(startX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Invert Y-Axis");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::Checkbox("##invert_y", &m_controlSettings.invertYAxis);

    // Controller Support
    ImGui::SetCursorPosX(startX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "Controller Support");
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::Checkbox("##controller_support", &m_controlSettings.controllerSupport);

    // Buttons
    const float buttonY = windowSize.y - 80.0f;
    const float buttonGroupWidth = 120.0f + 140.0f + 120.0f;
    const float buttonStartX = centerX - buttonGroupWidth / 2.0f;
    ImGui::SetCursorPos(ImVec2(buttonStartX, buttonY));

    if (ImGui::Button("Apply", ImVec2(120, 40)))
    {
        SyncControlSettingsToConfig();
        if (m_settingsManager)
            m_settingsManager->SaveSettings();

        // Apply sensitivity to camera
        ApplyCameraSensitivity(m_controlSettings.mouseSensitivity);
    }

    ImGui::SameLine(buttonStartX + 140.0f);
    if (ImGui::Button("Back", ImVec2(120, 40)))
    {
        if (m_backCallback)
            m_backCallback();
    }
}

void MenuSettingsController::ApplyPendingSettings()
{
    SyncVideoSettingsToConfig();
    SyncAudioSettingsToConfig();
    SyncControlSettingsToConfig();

    if (m_settingsManager)
    {
        m_settingsManager->SaveSettings();
        m_settingsManager->ApplyAudioSettings();
        m_settingsManager->ApplyVideoSettings();
    }

    ApplyCameraSensitivity(m_controlSettings.mouseSensitivity);
}

bool MenuSettingsController::HasUnsavedVideoChanges() const
{
    if (!m_settingsManager)
        return false;

    // Check resolution
    int currentW, currentH;
    m_settingsManager->GetResolution(currentW, currentH);
    std::string currentResStr = std::to_string(currentW) + "x" + std::to_string(currentH);
    std::string selectedResStr = "";

    if (m_videoSettings.resolutionIndex >= 0 &&
        m_videoSettings.resolutionIndex < static_cast<int>(m_resolutionOptions.size()))
    {
        selectedResStr = m_resolutionOptions[m_videoSettings.resolutionIndex];
    }

    if (currentResStr != selectedResStr)
        return true;

    return m_videoSettings.displayModeIndex != m_settingsManager->GetDisplayModeIndex() ||
           m_videoSettings.vsyncIndex != m_settingsManager->GetVSyncIndex() ||
           m_videoSettings.fpsIndex != m_settingsManager->GetFpsIndex();
}

void MenuSettingsController::SyncVideoSettingsToConfig() const
{
    if (!m_settingsManager)
        return;

    if (m_videoSettings.resolutionIndex >= 0 &&
        m_videoSettings.resolutionIndex < static_cast<int>(m_resolutionOptions.size()))
    {
        std::string resStr = m_resolutionOptions[m_videoSettings.resolutionIndex];
        size_t xPos = resStr.find('x');
        if (xPos != std::string::npos)
        {
            int w = std::stoi(resStr.substr(0, xPos));
            int h = std::stoi(resStr.substr(xPos + 1));
            m_settingsManager->SetResolution(w, h);
        }
    }

    m_settingsManager->SetDisplayModeIndex(m_videoSettings.displayModeIndex);
    m_settingsManager->SetVSyncIndex(m_videoSettings.vsyncIndex);
    m_settingsManager->SetFpsIndex(m_videoSettings.fpsIndex);
}

void MenuSettingsController::SyncAudioSettingsToConfig() const
{
    if (!m_settingsManager)
        return;

    m_settingsManager->SetMasterVolume(m_audioSettings.masterVolume);
    m_settingsManager->SetMusicVolume(m_audioSettings.musicVolume);
    m_settingsManager->SetSfxVolume(m_audioSettings.sfxVolume);
    m_settingsManager->SetMuted(m_audioSettings.muted);
}

void MenuSettingsController::SyncControlSettingsToConfig()
{
    if (!m_settingsManager)
        return;

    m_settingsManager->SetMouseSensitivity(m_controlSettings.mouseSensitivity);
    m_settingsManager->SetInvertYAxis(m_controlSettings.invertYAxis);
    m_settingsManager->SetControllerSupport(m_controlSettings.controllerSupport);
}

void MenuSettingsController::ApplyCameraSensitivity(float sensitivity)
{
    if (m_cameraController)
    {
        // Convert menu sensitivity (0.1-5.0) to camera sensitivity
        float cameraSensitivity = sensitivity * 0.1f;
        m_cameraController->SetMouseSensitivity(cameraSensitivity);
    }
}

bool MenuSettingsController::RenderVideoSettingCombo(const char *label, const char *id,
                                                     const std::vector<std::string> &options,
                                                     int &currentIndex, float labelWidth,
                                                     float comboWidth, float startX)
{
    bool changed = false;

    // Label
    ImGui::SetCursorPosX(startX);
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", label);

    // Combo box
    ImGui::SameLine(startX + labelWidth + 20.0f);
    ImGui::SetNextItemWidth(comboWidth);

    // Validate index
    if (currentIndex < 0 || currentIndex >= static_cast<int>(options.size()))
    {
        currentIndex = 0;
    }

    const char *currentOption = options[currentIndex].c_str();
    if (ImGui::BeginCombo(id, currentOption))
    {
        for (int i = 0; i < static_cast<int>(options.size()); ++i)
        {
            const bool isSelected = (currentIndex == i);
            if (ImGui::Selectable(options[i].c_str(), isSelected))
            {
                currentIndex = i;
                changed = true;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}




