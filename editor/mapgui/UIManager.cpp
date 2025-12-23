#include "editor/mapgui/UIManager.h"
#include "core/Log.h"
#include "editor/EditorTypes.h"
#include "editor/IEditor.h"
#include "editor/mapgui/skyboxBrowser.h"
#include "editor/panels/EditorPanelManager.h"
#include "nfd.h"
#include "scene/resources/map/core/MapData.h"
#include "scene/resources/map/core/SceneLoader.h"
#include <cstdlib>
#include <filesystem>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include <raymath.h>

namespace fs = std::filesystem;

EditorUIManager::EditorUIManager(const UIManagerConfig &config)
    : m_editor(config.editor), m_displayImGuiInterface(true), m_pendingObjectCreation(false),
      m_displaySkyboxPanel(false), m_displayParkourMapDialog(false),
      m_currentlySelectedParkourMapIndex(0), m_displayWelcomeScreen(true),
      m_skyboxBrowser(std::make_unique<SkyboxBrowser>(config.editor))
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

// Methods delegated to PanelManager
void EditorUIManager::ShowObjectPanel(bool show)
{
    m_editor->GetPanelManager().SetPanelVisible("Hierarchy", show);
}

void EditorUIManager::ShowPropertiesPanel(bool show)
{
    m_editor->GetPanelManager().SetPanelVisible("Inspector", show);
}

Tool EditorUIManager::GetActiveTool() const
{
    // Convert from IToolManager enum to local Tool enum
    return static_cast<Tool>(m_editor->GetState().GetActiveTool());
}

void EditorUIManager::SetActiveTool(::Tool tool)
{
    m_editor->GetState().SetActiveTool(static_cast<Tool>(tool));
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
            if (ImGui::MenuItem("New Map", "Ctrl+N"))
            {
                if (m_editor->GetSceneManager().IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_MAP;
                }
                else
                {
                    m_pendingAction = PendingAction::NEW_MAP;
                    ExecutePendingAction();
                }
            }
            if (ImGui::MenuItem("New UI Scene", "Ctrl+Shift+N"))
            {
                if (m_editor->GetSceneManager().IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_UI_SCENE;
                }
                else
                {
                    m_pendingAction = PendingAction::NEW_UI_SCENE;
                    ExecutePendingAction();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Scene As..."))
            {
                // Use NFD to show save dialog
                nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_SaveDialogU8(&outPath, filterItem, 1, nullptr, "map.json");
                if (result == NFD_OKAY)
                {
                    m_editor->GetSceneManager().SaveScene(std::string(outPath));
                    NFD_FreePath(outPath);
                }
            }
            if (ImGui::MenuItem(
                    "Quick Save", "Ctrl+S", false,
                    !m_editor->GetSceneManager().GetGameScene().GetMapMetaData().name.empty()))
            {
                m_editor->GetSceneManager().SaveScene(
                    m_editor->GetSceneManager().GetGameScene().GetMapMetaData().name + ".json");
                m_editor->GetProjectManager().SaveProject();
            }

            // Back to Welcome Screen
            if (ImGui::MenuItem("Back to Welcome Screen"))
            {
                if (m_editor->GetSceneManager().IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_MAP; // Default to map when going back
                }
                else
                {
                    m_editor->GetSceneManager().ClearScene();
                    m_editor->GetPanelManager().SetAllPanelsVisible(false);
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

            bool isUIScene =
                (m_editor->GetSceneManager().GetGameScene().GetMapMetaData().sceneType ==
                 SceneType::UI_MENU);

            for (int i = 0; i < std::size(toolNames); i++)
            {
                // Skip 3D object tools in UI scenes
                if (isUIScene && i > 1)
                    continue;

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

        m_editor->GetPanelManager().RenderViewMenu();

        // Status info on the right
        float width = ImGui::GetWindowWidth();
        std::string mapName =
            m_editor->GetSceneManager().GetGameScene().GetMapMetaData().name.empty()
                ? "Untitled"
                : m_editor->GetSceneManager().GetGameScene().GetMapMetaData().name;
        std::string skyboxName =
            m_editor->GetSceneManager().GetGameScene().GetMapMetaData().skyboxTexture;
        if (skyboxName.empty())
            skyboxName = "None";
        else
            skyboxName = fs::path(skyboxName).filename().string();

        bool isUIScene = (m_editor->GetSceneManager().GetGameScene().GetMapMetaData().sceneType ==
                          SceneType::UI_MENU);
        std::string sceneTypeStr = isUIScene ? "[UI Scene]" : "[3D Map]";

        std::string infoText = sceneTypeStr + " Scene: " + mapName;
        if (!isUIScene)
        {
            infoText += " | Skybox: " + skyboxName +
                        " | Grid: " + std::to_string(m_editor->GetState().GetGridSize());
        }

        ImGui::SameLine(width - 500);
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

        // (Grid slider logic moved to View menu or kept in a settings panel if needed)
    }
}

// Obsolete panel methods removed. Panels are now managed by EditorPanelManager.

void EditorUIManager::HandleKeyboardInput()
{
    if (IsKeyPressed(KEY_TWO))
    {
        m_editor->GetPanelManager().TogglePanelVisibility("Hierarchy");
    }

    if (IsKeyPressed(KEY_F))
    {
        m_editor->GetPanelManager().TogglePanelVisibility("Inspector");
    }
}

void EditorUIManager::ExecutePendingAction()
{
    if (m_pendingAction == PendingAction::NEW_MAP || m_pendingAction == PendingAction::NEW_UI_SCENE)
    {
        SceneType type =
            (m_pendingAction == PendingAction::NEW_MAP) ? SceneType::LEVEL_3D : SceneType::UI_MENU;
        m_editor->GetSceneManager().ClearScene();
        m_editor->GetSceneManager().GetGameScene().GetMapMetaDataMutable().sceneType = type;

        if (m_editor)
            m_editor->GetSceneManager().SetSkyboxTexture("");
        m_displayWelcomeScreen = false; // Start editing immediately

        // Show core panels
        auto &pm = m_editor->GetPanelManager();
        pm.SetPanelVisible("Toolbar", true);
        pm.SetPanelVisible("Viewport", true);
        pm.SetPanelVisible("Scene Hierarchy", true);
        pm.SetPanelVisible("Inspector", true);

        // Auto-switch panels based on scene type
        if (type == SceneType::UI_MENU)
        {
            pm.SetPanelVisible("UI Editor", true);
            m_editor->GetState().SetEditorMode(EditorMode::UI_DESIGN);
            m_editor->GetSelectionManager().RefreshUIEntities();
        }
        else
        {
            m_editor->GetState().SetEditorMode(EditorMode::SCENE_3D);
        }

        pm.ResetLayout();
    }
    else if (m_pendingAction == PendingAction::NEW_PROJECT)
    {
        // Pick folder for new project
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_PickFolderU8(&outPath, nullptr);
        if (result == NFD_OKAY)
        {
            m_editor->GetProjectManager().CreateNewProject(std::string(outPath));
            m_displayWelcomeScreen = false;

            // Show core panels
            auto &pm = m_editor->GetPanelManager();
            pm.SetPanelVisible("Toolbar", true);
            pm.SetPanelVisible("Viewport", true);
            pm.SetPanelVisible("Scene Hierarchy", true);
            pm.SetPanelVisible("Inspector", true);
            pm.SetPanelVisible("Asset Browser", true);
            pm.ResetLayout();
            NFD_FreePath(outPath);
        }
    }
    else if (m_pendingAction == PendingAction::OPEN_PROJECT)
    {
        // Open File Dialog for .chxproj file
        nfdchar_t *outPath = nullptr;
        nfdfilteritem_t filterItem[1] = {{"Chained Project", "chxproj"}};
        nfdresult_t result = NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr);
        if (result == NFD_OKAY)
        {
            std::string projectFilePath = std::string(outPath);
            // Extract project directory from file path
            std::filesystem::path filePath(projectFilePath);
            std::string projectPath = filePath.parent_path().string();

            m_editor->GetProjectManager().SetProjectPath(projectPath);
            m_displayWelcomeScreen = false;

            // Show core panels
            auto &pm = m_editor->GetPanelManager();
            pm.SetPanelVisible("Toolbar", true);
            pm.SetPanelVisible("Viewport", true);
            pm.SetPanelVisible("Scene Hierarchy", true);
            pm.SetPanelVisible("Inspector", true);
            pm.SetPanelVisible("Asset Browser", true);
            pm.ResetLayout();
            NFD_FreePath(outPath);
            CD_INFO("[UIManager] Opened project at: %s", projectPath.c_str());
        }
    }
    else if (m_pendingAction == PendingAction::LOAD_SCENE)
    {
        // For Load, show the file dialog
        nfdfilteritem_t filterItem[2] = {{"JSON Scene", "json"}, {"Chained project", "chxproj"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&outPath, filterItem, 2, nullptr);
        if (result == NFD_OKAY)
        {
            std::string path = std::string(outPath);
            if (path.find(".chxproj") != std::string::npos)
            {
                m_editor->GetProjectManager().LoadProject(path);
            }
            else
            {
                m_editor->GetSceneManager().LoadScene(path);
            }
            m_displayWelcomeScreen = false;

            // Show core panels
            auto &pm = m_editor->GetPanelManager();
            pm.SetPanelVisible("Toolbar", true);
            pm.SetPanelVisible("Viewport", true);
            pm.SetPanelVisible("Scene Hierarchy", true);
            pm.SetPanelVisible("Inspector", true);
            pm.ResetLayout();
            NFD_FreePath(outPath);
        }
    }
    m_pendingAction = PendingAction::NONE;
}

int EditorUIManager::GetGridSize() const
{
    return m_editor ? m_editor->GetState().GetGridSize() : 50;
}

void EditorUIManager::RenderWelcomeScreen()
{
    // Lazy load icons with path fallback
    if (!m_iconsLoaded)
    {
        m_iconNewProject = LoadTexture(PROJECT_ROOT_DIR "/resources/map_editor/newproject.jpg");
        m_iconOpenProject = LoadTexture(PROJECT_ROOT_DIR "/resources/map_editor/folder.png");
        m_iconSceneProject = LoadTexture(PROJECT_ROOT_DIR "/resources/map_editor/scene.png");

        SetTextureFilter(m_iconNewProject, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(m_iconOpenProject, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(m_iconSceneProject, TEXTURE_FILTER_BILINEAR);

        m_iconsLoaded = true;
    }
    // Professional background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImVec4 bgColor = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Begin("Welcome Screen", nullptr, flags))
    {
        // --- SIDEBAR (Left) ---
        float sidebarWidth = 220.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.18f, 1.0f));
        ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), false, ImGuiWindowFlags_NoScrollbar);

        ImGui::Spacing();
        ImGui::Indent(15);
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Chained Editor");
        ImGui::SetWindowFontScale(0.8f);
        ImGui::TextDisabled("v2025.12.22");
        ImGui::Unindent(15);

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Sidebar Items
        const char *sidebarItems[] = {"Projects", "Learning", "Plugins", "Settings"};
        static int selectedSidebar = 0;
        for (int i = 0; i < 4; i++)
        {
            bool isSelected = (selectedSidebar == i);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));

            ImGui::SetCursorPosX(10);
            if (ImGui::Selectable(sidebarItems[i], isSelected, ImGuiSelectableFlags_None,
                                  ImVec2(sidebarWidth - 20, 35)))
            {
                selectedSidebar = i;
            }

            if (isSelected)
                ImGui::PopStyleColor();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
        ImGui::Separator();
        ImGui::Indent(15);
        ImGui::TextDisabled("CHEngine Engine");
        ImGui::Unindent(15);

        ImGui::EndChild();
        ImGui::PopStyleColor(); // ChildBg

        ImGui::SameLine();

        // --- MAIN AREA (Right) ---
        ImVec2 mainAreaSize = ImGui::GetContentRegionAvail();
        if (ImGui::BeginChild("MainArea", mainAreaSize, false, ImGuiWindowFlags_NoBackground))
        {
            // Title and Header
            ImGui::SetCursorPosY(60);
            ImGui::SetWindowFontScale(1.8f);
            std::string welcomeTitle = "Welcome to Chained Editor";
            float welcomeWidth = ImGui::CalcTextSize(welcomeTitle.c_str()).x;
            ImGui::SetCursorPosX((mainAreaSize.x - welcomeWidth) * 0.5f);
            ImGui::Text("%s", welcomeTitle.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::SetCursorPosY(100);
            std::string subTitle = "Create a new project or open an existing one to get started.";
            float subWidth = ImGui::CalcTextSize(subTitle.c_str()).x;
            ImGui::SetCursorPosX((mainAreaSize.x - subWidth) * 0.5f);
            ImGui::TextDisabled("%s", subTitle.c_str());

            // Action Buttons (Horizontal)
            float iconBoxSize = 100.0f;
            float buttonSpacing = 80.0f;
            int numButtons = 2;
            float totalW = (iconBoxSize * numButtons) + (buttonSpacing * (numButtons - 1));
            float startX = (mainAreaSize.x - totalW) * 0.5f;

            float buttonsY = 220.0f;
            auto renderBigButton =
                [&](const char *id, Texture2D &icon, const char *label, PendingAction action)
            {
                ImGui::SetCursorPos(ImVec2(startX, buttonsY));
                ImVec2 p = ImGui::GetCursorScreenPos();

                ImGui::PushID(id);
                if (ImGui::InvisibleButton("##btn", ImVec2(iconBoxSize, iconBoxSize + 35)))
                {
                    if (m_editor->GetSceneManager().IsSceneModified())
                    {
                        m_showSavePrompt = true;
                        m_pendingAction = action;
                    }
                    else
                    {
                        m_pendingAction = action;
                        ExecutePendingAction();
                    }
                }

                bool hovered = ImGui::IsItemHovered();
                bool active = ImGui::IsItemActive();

                ImDrawList *dl = ImGui::GetWindowDrawList();

                // Button background hover state
                if (hovered || active)
                {
                    dl->AddRectFilled(p, ImVec2(p.x + iconBoxSize, p.y + iconBoxSize),
                                      IM_COL32(255, 255, 255, 10), 8.0f);
                    dl->AddRect(p, ImVec2(p.x + iconBoxSize, p.y + iconBoxSize),
                                IM_COL32(100, 149, 237, 120), 8.0f, 0, 2.0f);
                }

                // Draw Icon centered
                float iconSize = 48.0f;
                dl->AddImage((ImTextureID)(intptr_t)icon.id,
                             ImVec2(p.x + (iconBoxSize - iconSize) * 0.5f,
                                    p.y + (iconBoxSize - iconSize) * 0.5f),
                             ImVec2(p.x + (iconBoxSize + iconSize) * 0.5f,
                                    p.y + (iconBoxSize + iconSize) * 0.5f));

                float labelW = ImGui::CalcTextSize(label).x;
                dl->AddText(ImVec2(p.x + (iconBoxSize - labelW) * 0.5f, p.y + iconBoxSize + 10),
                            hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255),
                            label);

                ImGui::PopID();
                startX += iconBoxSize + buttonSpacing;
            };

            renderBigButton("NewProject", m_iconNewProject, "New Project",
                            PendingAction::NEW_PROJECT);
            renderBigButton("OpenProject", m_iconOpenProject, "Open Project",
                            PendingAction::OPEN_PROJECT);

            // Recent Projects Section - Move down to avoid overlap
            ImGui::SetCursorPosY(420);
            ImGui::SetCursorPosX(60);
            ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "RECENT PROJECTS");
            ImGui::Separator();

            ImGui::SetCursorPosX(60);
            ImGui::BeginChild("RecentProjects", ImVec2(mainAreaSize.x - 120, 300), false);

            const auto &recent = m_editor->GetProjectManager().GetRecentProjects();
            if (recent.empty())
            {
                ImGui::SetCursorPosY(20);
                ImGui::SetCursorPosX(
                    (mainAreaSize.x - 120 - ImGui::CalcTextSize("Go create something amazing!").x) *
                    0.5f);
                ImGui::TextDisabled("No recent projects found. Go create something amazing!");
            }
            else
            {
                for (const auto &path : recent)
                {
                    std::string projName = std::filesystem::path(path).stem().string();
                    std::string projPath = std::filesystem::path(path).parent_path().string();

                    ImGui::PushID(path.c_str());
                    if (ImGui::Selectable("##proj", false, ImGuiSelectableFlags_AllowDoubleClick,
                                          ImVec2(0, 45)))
                    {
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            m_editor->GetProjectManager().LoadProject(path);
                            m_displayWelcomeScreen = false;
                        }
                    }

                    ImVec2 pMin = ImGui::GetItemRectMin();
                    ImVec2 pMax = ImGui::GetItemRectMax();
                    ImDrawList *dlist = ImGui::GetWindowDrawList();

                    dlist->AddText(ImGui::GetFont(), 17.0f, ImVec2(pMin.x + 10, pMin.y + 6),
                                   IM_COL32(255, 255, 255, 255), projName.c_str());
                    dlist->AddText(ImGui::GetFont(), 13.0f, ImVec2(pMin.x + 10, pMin.y + 26),
                                   IM_COL32(160, 160, 160, 255), projPath.c_str());

                    ImGui::PopID();
                    ImGui::Spacing();
                }
            }
            ImGui::EndChild();

            // Bottom bar / Exit
            ImGui::SetCursorPosY(mainAreaSize.y - 50);
            ImGui::Separator();
            ImGui::SetCursorPosY(mainAreaSize.y - 35);
            ImGui::SetCursorPosX(mainAreaSize.x - 120);
            if (ImGui::Button("Exit Editor", ImVec2(100, 25)))
            {
                m_shouldExit = true;
                std::exit(0);
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

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
            std::string currentPath = m_editor->GetSceneManager().GetCurrentMapPath();
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
                m_editor->GetSceneManager().SaveScene(currentPath);
                m_editor->GetSceneManager().SetSceneModified(false); // Changes saved

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
            m_editor->GetSceneManager().SetSceneModified(false); // Discard changes

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

bool EditorUIManager::IsWelcomeScreenActive() const
{
    return m_displayWelcomeScreen;
}

void EditorUIManager::ToggleSkyboxBrowser()
{
    m_displaySkyboxPanel = !m_displaySkyboxPanel;
}

bool EditorUIManager::IsImGuiInterfaceDisplayed() const
{
    return m_displayImGuiInterface;
}

bool EditorUIManager::IsParkourMapDialogDisplayed() const
{
    return m_displayParkourMapDialog;
}

const std::string &EditorUIManager::GetSelectedModelName() const
{
    return m_currentlySelectedModelName;
}

void EditorUIManager::SetSelectedModelName(const std::string &name)
{
    m_currentlySelectedModelName = name;
}

bool EditorUIManager::ShouldExit() const
{
    return m_shouldExit;
}

void EditorUIManager::ProcessPendingObjectCreation()
{
    if (m_pendingObjectCreation)
    {
        m_editor->GetSceneManager().CreateDefaultObject(static_cast<MapObjectType>(GetActiveTool()),
                                                        m_currentlySelectedModelName);
        m_pendingObjectCreation = false;
        SetActiveTool(SELECT);
    }
}
