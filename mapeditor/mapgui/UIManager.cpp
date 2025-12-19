#include "mapeditor/mapgui/UIManager.h"
#include "mapeditor/EditorTypes.h"
#include "mapeditor/IEditor.h"
#include "mapeditor/mapgui/IUIManager.h"
#include "mapeditor/mapgui/SkyboxBrowser.h"
#include "nfd.h"
#include "scene/resources/map/Core/MapData.h"
#include "scene/resources/map/Core/MapLoader.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include <raymath.h>

namespace fs = std::filesystem;

EditorUIManager::EditorUIManager(const UIManagerConfig &config)
    : m_editor(config.editor), m_displayImGuiInterface(true), m_displayObjectListPanel(true),
      m_displayPropertiesPanel(true), m_pendingObjectCreation(false), m_displaySkyboxPanel(false),
      m_displayParkourMapDialog(false), m_currentlySelectedParkourMapIndex(0),
      m_displayWelcomeScreen(true), m_skyboxBrowser(std::make_unique<SkyboxBrowser>(config.editor))
{
}

EditorUIManager::~EditorUIManager()
{
    // SkyboxBrowser handles its own cleanup
    if (m_iconsLoaded)
    {
        UnloadTexture(m_iconNewProject);
        UnloadTexture(m_iconOpenProject);
    }
}

void EditorUIManager::Render()
{
    // Note: rlImGuiBegin() is now called in Application::Run() for docking support

    if (m_displayWelcomeScreen)
    {
        RenderWelcomeScreen();
        return;
    }

    // Render all ImGui panels in specific order
    RenderImGuiToolbar();

    if (m_displayObjectListPanel)
    {
        RenderImGuiObjectPanel();
    }

    if (m_editor->GetSelectedObject() != nullptr)
    {
        RenderImGuiPropertiesPanel();
    }

    // Render skybox panel (always call to handle cleanup even when closed)
    // Note: SkyboxBrowser handles its own Begin/End and can modify m_displaySkyboxPanel
    if (m_displaySkyboxPanel && m_skyboxBrowser)
    {
        m_skyboxBrowser->RenderPanel(m_displaySkyboxPanel);
    }
}

void EditorUIManager::HandleInput()
{
    // Block input if welcome screen is active
    if (m_displayWelcomeScreen)
        return;

    // Get ImGui IO for input handling
    const ImGuiIO &io = ImGui::GetIO();

    // Handle keyboard input only when ImGui is not capturing
    if (!io.WantCaptureKeyboard)
    {
        HandleKeyboardInput();
    }
}

void EditorUIManager::ShowObjectPanel(bool show)
{
    m_displayObjectListPanel = show;
}

void EditorUIManager::ShowPropertiesPanel(bool show)
{
    m_displayPropertiesPanel = show;
}

Tool EditorUIManager::GetActiveTool() const
{
    // Convert from IToolManager enum to local Tool enum
    return static_cast<Tool>(m_editor->GetActiveTool());
}

void EditorUIManager::SetActiveTool(::Tool tool)
{
    m_editor->SetActiveTool(static_cast<Tool>(tool));
}

ImVec2 EditorUIManager::ClampWindowPosition(const ImVec2 &desiredPos, ImVec2 &windowSize)
{
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    // Clamp window size to fit screen
    if (windowSize.x > static_cast<float>(screenWidth))
        windowSize.x = static_cast<float>(screenWidth);
    if (windowSize.y > static_cast<float>(screenHeight))
        windowSize.y = static_cast<float>(screenHeight);

    float clampedX = desiredPos.x;
    float clampedY = desiredPos.y;

    // Clamp X position
    if (clampedX < 0.0f)
        clampedX = 0.0f;
    else if (clampedX + windowSize.x > static_cast<float>(screenWidth))
        clampedX = static_cast<float>(screenWidth) - windowSize.x;

    // Clamp Y position
    if (clampedY < 0.0f)
        clampedY = 0.0f;
    else if (clampedY + windowSize.y > static_cast<float>(screenHeight))
        clampedY = static_cast<float>(screenHeight) - windowSize.y;

    return ImVec2(clampedX, clampedY);
}

void EditorUIManager::EnsureWindowInBounds()
{
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    bool needsClamp = false;
    ImVec2 clampedPos = pos;
    ImVec2 clampedSize = size;

    // Clamp size
    if (clampedSize.x > static_cast<float>(screenWidth))
    {
        clampedSize.x = static_cast<float>(screenWidth);
        needsClamp = true;
    }
    if (clampedSize.y > static_cast<float>(screenHeight))
    {
        clampedSize.y = static_cast<float>(screenHeight);
        needsClamp = true;
    }

    // Clamp position
    if (clampedPos.x < 0.0f)
    {
        clampedPos.x = 0.0f;
        needsClamp = true;
    }
    else if (clampedPos.x + clampedSize.x > static_cast<float>(screenWidth))
    {
        clampedPos.x = static_cast<float>(screenWidth) - clampedSize.x;
        needsClamp = true;
    }

    if (clampedPos.y < 0.0f)
    {
        clampedPos.y = 0.0f;
        needsClamp = true;
    }
    else if (clampedPos.y + clampedSize.y > static_cast<float>(screenHeight))
    {
        clampedPos.y = static_cast<float>(screenHeight) - clampedSize.y;
        needsClamp = true;
    }

    // Apply clamping only if needed
    if (needsClamp)
    {
        ImGui::SetWindowPos(clampedPos, ImGuiCond_Always);
        ImGui::SetWindowSize(clampedSize, ImGuiCond_Always);
    }
}

void EditorUIManager::RenderImGuiToolbar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Map As..."))
            {
                // Use NFD to show save dialog
                nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_SaveDialogU8(&outPath, filterItem, 1, nullptr, "map.json");
                if (result == NFD_OKAY)
                {
                    m_editor->SaveMap(std::string(outPath));
                    NFD_FreePath(outPath);
                }
            }
            if (ImGui::MenuItem("Load Map..."))
            {
                if (m_editor->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::LOAD_MAP;
                }
                else
                {
                    // Use NFD to show open dialog
                    nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                    nfdchar_t *outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr);
                    if (result == NFD_OKAY)
                    {
                        m_editor->LoadMap(std::string(outPath));
                        NFD_FreePath(outPath);
                    }
                }
            }
            if (ImGui::MenuItem("Quick Save", nullptr, false,
                                !m_editor->GetGameMap().GetMapMetaData().name.empty()))
            {
                m_editor->SaveMap(m_editor->GetGameMap().GetMapMetaData().name + ".json");
            }

            // Back to Welcome Screen
            if (ImGui::MenuItem("Back to Welcome Screen"))
            {
                if (m_editor->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_PROJECT;
                }
                else
                {
                    m_editor->ClearScene();
                    m_displayWelcomeScreen = true;
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                m_shouldExit = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            const char *toolNames[] = {"Select",       "Move",      "Rotate",
                                       "Scale",        "Add Cube",  "Add Sphere",
                                       "Add Cylinder", "Add Model", "Add Spawn Zone"};
            for (int i = 0; i < std::size(toolNames); i++)
            {
                bool isSelected = (GetActiveTool() == static_cast<Tool>(i));
                if (ImGui::MenuItem(toolNames[i], nullptr, isSelected))
                {
                    SetActiveTool(static_cast<Tool>(i));
                    if (GetActiveTool() == ADD_CUBE || GetActiveTool() == ADD_SPHERE ||
                        GetActiveTool() == ADD_CYLINDER || GetActiveTool() == ADD_MODEL ||
                        GetActiveTool() == ADD_SPAWN_ZONE)
                    {
                        m_pendingObjectCreation = true;
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene Hierarchy", nullptr, &m_displayObjectListPanel);
            ImGui::MenuItem("Properties", nullptr, &m_displayPropertiesPanel);
            ImGui::MenuItem("Skybox Settings", nullptr, &m_displaySkyboxPanel);
            ImGui::EndMenu();
        }

        // Status info on the right
        float width = ImGui::GetWindowWidth();
        std::string mapName = m_editor->GetGameMap().GetMapMetaData().name.empty()
                                  ? "Untitled"
                                  : m_editor->GetGameMap().GetMapMetaData().name;
        std::string infoText =
            "Map: " + mapName + " | Grid: " + std::to_string(m_editor->GetGridSize());

        ImGui::SameLine(width - 300);
        ImGui::Text("%s", infoText.c_str());

        ImGui::EndMainMenuBar();

        // Handle Model Selection if needed (as a popup or separate window)
        // For now, if tool is Add Model, show a small window
        if (GetActiveTool() == ADD_MODEL)
        {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 50),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::Begin("Select Model", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

            if (ImGui::BeginCombo("##ModelSelect", m_currentlySelectedModelName.c_str()))
            {
                const auto availableModels = m_editor->GetModelLoader()->GetAvailableModels();
                for (const auto &modelName : availableModels)
                {
                    bool isSelected = (m_currentlySelectedModelName == modelName);
                    if (ImGui::Selectable(modelName.c_str(), isSelected))
                    {
                        m_currentlySelectedModelName = modelName;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::End();
        }

        // Handle Grid resizing via shortcuts or menu
        // (Grid slider logic moved to View menu or kept in a settings panel if needed)
    }
}

void EditorUIManager::RenderImGuiObjectPanel()
{
    const int screenWidth = GetScreenWidth();
    ImVec2 windowSize(240, 400);
    ImVec2 desiredPos(static_cast<float>(screenWidth) - 250, 10);
    ImVec2 clampedPos = ClampWindowPosition(desiredPos, windowSize);

    ImGui::SetNextWindowPos(clampedPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    // Limit maximum window size to screen dimensions
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(100, 100),
        ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));

    // Fixed Left Panel Layout
    ImGui::SetNextWindowPos(ImVec2(0, MENU_BAR_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(PANEL_WIDTH, static_cast<float>(GetScreenHeight()) - MENU_BAR_HEIGHT),
        ImGuiCond_Always);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    bool objectPanelOpen = true;
    if (ImGui::Begin("Scene Hierarchy", &objectPanelOpen, windowFlags))
    {
        // Ensure window stays within screen bounds
        EnsureWindowInBounds();
        if (ImGui::Button("Add Object"))
        {
            m_editor->CreateDefaultObject(MapObjectType::CUBE);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove") && m_editor->GetSelectedObject() != nullptr)
        {
            // Remove selected object
            m_editor->RemoveObject(m_editor->GetSelectedObjectIndex());
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All"))
        {
            m_editor->ClearScene();
        }

        ImGui::Separator();

        // List all objects
        const auto &objects = m_editor->GetGameMap().GetMapObjects();
        for (size_t i = 0; i < objects.size(); i++)
        {
            const auto &obj = objects[i];
            std::string label = "Object " + std::to_string(i);
            if (!obj.modelName.empty())
                label = obj.modelName;

            if (const bool isSelected = (static_cast<int>(i) == m_editor->GetSelectedObjectIndex());
                ImGui::Selectable(label.c_str(), isSelected))
            {
                m_editor->SelectObject(static_cast<int>(i));
            }

            // Show object info on hover
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Position: %.1f, %.1f, %.1f", obj.position.x, obj.position.y,
                            obj.position.z);
                ImGui::Text("Type: %d", static_cast<int>(obj.type));
                ImGui::EndTooltip();
            }
        }
    }

    // If window was closed, don't show it next frame
    if (!objectPanelOpen)
    {
        m_displayObjectListPanel = false;
    }

    ImGui::End();
}

void EditorUIManager::RenderImGuiPropertiesPanel()
{
    // Fixed Right Panel Layout
    ImGui::SetNextWindowPos(
        ImVec2(static_cast<float>(GetScreenWidth()) - PANEL_WIDTH, MENU_BAR_HEIGHT),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(PANEL_WIDTH, static_cast<float>(GetScreenHeight()) - MENU_BAR_HEIGHT),
        ImGuiCond_Always);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    bool propertiesPanelOpen = true;
    if (ImGui::Begin("Properties", &propertiesPanelOpen, windowFlags))
    {
        // Ensure window stays within screen bounds
        EnsureWindowInBounds();

        auto *selectedMapObj = m_editor->GetSelectedObject();

        if (selectedMapObj != nullptr)
        {
            auto &obj = *selectedMapObj;

            // Type
            const char *types[] = {"Cube",    "Sphere", "Cylinder",  "Plane",
                                   "Ellipse", "Model",  "Spawn Zone"};
            int typeIndex = static_cast<int>(obj.type);
            if (ImGui::Combo("Type", &typeIndex, types, IM_ARRAYSIZE(types)))
            {
                obj.type = static_cast<MapObjectType>(typeIndex);
            }

            ImGui::Separator();

            // Transform
            float pos[3] = {obj.position.x, obj.position.y, obj.position.z};
            if (ImGui::DragFloat3("Position", pos, 0.1f))
            {
                obj.position = {pos[0], pos[1], pos[2]};
                m_editor->SetSceneModified(true);
            }

            // Rotation
            float rot[3] = {obj.rotation.x, obj.rotation.y, obj.rotation.z};
            if (ImGui::DragFloat3("Rotation", rot, 0.1f))
            {
                obj.rotation = {rot[0], rot[1], rot[2]};
                m_editor->SetSceneModified(true);
            }

            // Scale
            float scl[3] = {obj.scale.x, obj.scale.y, obj.scale.z};
            if (ImGui::DragFloat3("Scale", scl, 0.1f))
            {
                obj.scale = {scl[0], scl[1], scl[2]};
                m_editor->SetSceneModified(true);
            }

            // Model Specific
            if (obj.type == MapObjectType::MODEL)
            {
                ImGui::Text("Model Asset:");
                if (ImGui::BeginCombo("##ModelSelect", obj.modelName.c_str()))
                {
                    const auto availableModels = m_editor->GetModelLoader()->GetAvailableModels();
                    for (const auto &modelName : availableModels)
                    {
                        bool isSelected = (obj.modelName == modelName);
                        if (ImGui::Selectable(modelName.c_str(), isSelected))
                        {
                            obj.modelName = modelName;
                            m_editor->SetSceneModified(true);
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            // Color
            float c[4] = {obj.color.r / 255.0f, obj.color.g / 255.0f, obj.color.b / 255.0f,
                          obj.color.a / 255.0f};
            if (ImGui::ColorEdit4("Color", c))
            {
                obj.color = {(unsigned char)(c[0] * 255), (unsigned char)(c[1] * 255),
                             (unsigned char)(c[2] * 255), (unsigned char)(c[3] * 255)};
                m_editor->SetSceneModified(true);
            }
        }
        else
        {
            ImGui::TextDisabled("No object selected");
        }
    }
    ImGui::End();

    if (!propertiesPanelOpen)
    {
        m_displayPropertiesPanel = false;
    }
}

void EditorUIManager::HandleKeyboardInput()
{
    // Handle keyboard shortcuts for scene objects
    if (IsKeyPressed(KEY_DELETE) && m_editor->GetSelectedObject() != nullptr)
    {
        // Remove selected object
        m_editor->RemoveObject(m_editor->GetSelectedObjectIndex());
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        m_editor->ClearSelection();
    }

    // Toggle UI panels with different keys
    if (IsKeyPressed(KEY_TWO))
    {
        m_displayObjectListPanel = !m_displayObjectListPanel;
    }

    if (IsKeyPressed(KEY_F))
    {
        m_displayPropertiesPanel = !m_displayPropertiesPanel;
    }
}

void EditorUIManager::ExecutePendingAction()
{
    if (m_pendingAction == PendingAction::NEW_PROJECT)
    {
        m_editor->ClearScene();
        if (m_editor)
            m_editor->SetSkyboxTexture("");
        m_displayWelcomeScreen = true; // Fix: Use true to go back to welcome screen
    }
    else if (m_pendingAction == PendingAction::OPEN_PROJECT ||
             m_pendingAction == PendingAction::LOAD_MAP)
    {
        // For Open/Load, show the dialog
        nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr);
        if (result == NFD_OKAY)
        {
            m_editor->LoadMap(std::string(outPath));
            m_displayWelcomeScreen = false;
            NFD_FreePath(outPath);
        }
    }
    m_pendingAction = PendingAction::NONE;
}

int EditorUIManager::GetGridSize() const
{
    return m_editor ? m_editor->GetGridSize() : 50;
}

void EditorUIManager::RenderWelcomeScreen()
{
    // Lazy load icons with path fallback
    if (!m_iconsLoaded)
    {

        m_iconNewProject = LoadTexture(PROJECT_ROOT_DIR "/resources/map_editor/newproject.jpg");
        m_iconOpenProject = LoadTexture(PROJECT_ROOT_DIR "/resources/map_editor/folder.png");

        // Linear filter for better scaling
        SetTextureFilter(m_iconNewProject, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(m_iconOpenProject, TEXTURE_FILTER_BILINEAR);

        m_iconsLoaded = true;
    }

    // Full screen window with gray background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    // JetBrains Darcula-like background (approx #2B2B2B)
    ImVec4 bgColor = ImVec4(0.169f, 0.169f, 0.169f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);

    // Style adjustments for a cleaner look
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Begin("Welcome Screen", nullptr, flags))
    {
        // Center content area
        ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
        float contentWidth = 700.0f;
        float contentHeight = 500.0f;

        ImGui::SetCursorPos(ImVec2((viewportSize.x - contentWidth) * 0.5f,
                                   (viewportSize.y - contentHeight) * 0.5f));

        if (ImGui::BeginChild("WelcomeContent", ImVec2(contentWidth, contentHeight), false,
                              ImGuiWindowFlags_NoBackground))
        {
            // Title
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::SetWindowFontScale(2.5f);
            std::string title = "Chained Decos Editor";
            float textWidth = ImGui::CalcTextSize(title.c_str()).x;
            ImGui::SetCursorPosX((contentWidth - textWidth) * 0.5f);

            // Nice white title
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            ImGui::Text("%s", title.c_str());
            ImGui::PopStyleColor();

            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopFont();

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            // Two columns for buttons
            ImGui::Columns(2, "StartColumns", false);

            float columnWidth = ImGui::GetColumnWidth();
            float iconSize = 180.0f; // Size of the icon

            // --- NEW PROJECT ---
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - iconSize) * 0.5f);

            // Use Image + IsItemClicked for cleaner look (no button frame unless hovered logic
            // added) But to make it feel responsive, we'll wrap in a group and handle hover
            // manually or use InvisibleButton
            ImGui::PushID("NewProj");

            // Draw Icon
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)(intptr_t)m_iconNewProject.id, ImVec2(iconSize, iconSize));

            // Add a subtle hover effect?
            if (ImGui::IsItemHovered())
            {
                ImGui::GetWindowDrawList()->AddRect(
                    ImVec2(cursorPos.x - 5, cursorPos.y - 5),
                    ImVec2(cursorPos.x + iconSize + 5, cursorPos.y + iconSize + 5),
                    IM_COL32(100, 149, 237, 100), // Cornflower blue hint
                    5.0f, 0, 3.0f);
            }

            if (ImGui::IsItemClicked())
            {
                if (m_editor->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_PROJECT;
                }
                else
                {
                    m_editor->ClearScene();
                    if (m_editor)
                        m_editor->SetSkyboxTexture("");
                    m_displayWelcomeScreen = false;
                }
            }
            ImGui::PopID();

            // Label
            ImGui::Spacing();
            std::string labelNew = "Create New Project";
            float labelNewWidth = ImGui::CalcTextSize(labelNew.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - labelNewWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", labelNew.c_str());

            ImGui::NextColumn();

            // --- OPEN PROJECT ---
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - iconSize) * 0.5f);

            ImGui::PushID("OpenProj");
            cursorPos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)(intptr_t)m_iconOpenProject.id, ImVec2(iconSize, iconSize));

            if (ImGui::IsItemHovered())
            {
                ImGui::GetWindowDrawList()->AddRect(
                    ImVec2(cursorPos.x - 5, cursorPos.y - 5),
                    ImVec2(cursorPos.x + iconSize + 5, cursorPos.y + iconSize + 5),
                    IM_COL32(100, 149, 237, 100), 5.0f, 0, 3.0f);
            }

            if (ImGui::IsItemClicked())
            {
                if (m_editor->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::OPEN_PROJECT;
                }
                else
                {
                    nfdu8filteritem_t filterItem[1] = {{"JSON", "json"}};
                    nfdu8char_t *outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr);
                    if (result == NFD_OKAY)
                    {
                        m_editor->LoadMap(std::string(outPath));
                        m_displayWelcomeScreen = false;
                        NFD_FreePathU8(outPath);
                    }
                }
            }
            ImGui::PopID();

            // Label
            ImGui::Spacing();
            std::string labelOpen = "Open Existing Project";
            float labelOpenWidth = ImGui::CalcTextSize(labelOpen.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - labelOpenWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", labelOpen.c_str());

            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();

            // Exit Button
            ImGui::SetCursorPosX((contentWidth - 120) * 0.5f);
            // Make exit button slightly more visible/styled
            if (ImGui::Button("Exit Editor", ImVec2(120, 35)))
            {
                TraceLog(LOG_INFO, "[UIManager] Exit button clicked, setting m_shouldExit = true");
                m_shouldExit = true;
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();   // Pop rounding
    ImGui::PopStyleColor(); // Pop BG

    // Show Save Prompt if requested
    RenderSavePrompt();
}

void EditorUIManager::RenderSavePrompt()
{
    if (m_showSavePrompt)
    {
        ImGui::OpenPopup("Unsaved Changes");
    }

    // Always center the modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("You have unsaved changes.");
        ImGui::Text("Do you want to save them before continuing?");
        ImGui::Separator();

        // SAVE
        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            // Trigger Save Logic
            std::string currentPath = m_editor->GetCurrentMapPath();
            if (currentPath.empty())
            {
                // Need to ask for path if new map
                nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_SaveDialogU8(&outPath, filterItem, 1, nullptr, "map.json");
                if (result == NFD_OKAY)
                {
                    currentPath = std::string(outPath);
                    NFD_FreePath(outPath);
                }
                else
                {
                    // Cancelled save, abort action
                    currentPath = "";
                }
            }

            if (!currentPath.empty())
            {
                m_editor->SaveMap(currentPath);
                m_editor->SetSceneModified(false); // Changes saved

                // Now proceed with pending action
                ImGui::CloseCurrentPopup();
                m_showSavePrompt = false;
                ExecutePendingAction();
            }
            else
            {
                // If currentPath is empty, it means save was cancelled.
                // We should close the popup and reset pending action.
                ImGui::CloseCurrentPopup();
                m_showSavePrompt = false;
                m_pendingAction = PendingAction::NONE;
            }
        }

        ImGui::SameLine();

        // DON'T SAVE
        if (ImGui::Button("Don't Save", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_showSavePrompt = false;
            m_editor->SetSceneModified(false); // Discard changes

            ExecutePendingAction();
        }

        ImGui::SameLine();

        // CANCEL
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_showSavePrompt = false;
            m_pendingAction = PendingAction::NONE;
        }

        ImGui::EndPopup();
    }
}
