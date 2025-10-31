//
// Created by Kilo Code
//

#include "UIManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Map/MapLoader.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include <raymath.h>

namespace fs = std::filesystem;

UIManager::UIManager(ISceneManager* sceneManager, IFileManager* fileManager,
                     IToolManager* toolManager, IModelManager* modelManager)
    : m_sceneManager(sceneManager), m_fileManager(fileManager),
      m_toolManager(toolManager), m_modelManager(modelManager),
      m_displayImGuiInterface(true), m_displayObjectListPanel(true),
      m_displayPropertiesPanel(true), m_pendingObjectCreation(false),
      m_displayFileDialog(false), m_isFileLoadDialog(true),
      m_isJsonExportDialog(false), m_displayNewFolderDialog(false),
      m_displayDeleteConfirmationDialog(false), m_displayParkourMapDialog(false),
      m_currentlySelectedParkourMapIndex(0), m_gridSizes(50)
{
    // Initialize file dialog to project root
    m_currentWorkingDirectory = PROJECT_ROOT_DIR "/resources";
    m_newFileNameInput = "new_map.json";
    RefreshDirectoryItems();
    // NFD init
    NFD_Init();
}

UIManager::~UIManager() { NFD_Quit(); }

void UIManager::Render()
{
    // Note: rlImGuiBegin() is now called in Application::Run() for docking support
    // Render all ImGui panels in specific order
    RenderImGuiToolbar();

    if (m_displayObjectListPanel)
    {
        RenderImGuiObjectPanel();
    }

    if (m_sceneManager->GetSelectedObject() != nullptr)
    {
        RenderImGuiPropertiesPanel();
    }

    // Render file dialog if shown
    if (m_displayFileDialog)
    {
        RenderFileDialog();
    }
    // Render parkour map dialog if shown
    RenderParkourMapDialog();

    // Note: rlImGuiEnd() is now called in Application::Run() for docking support
}

void UIManager::HandleInput()
{
    // Get ImGui IO for input handling
    const ImGuiIO &io = ImGui::GetIO();

    // Handle keyboard input only when ImGui is not capturing
    if (!io.WantCaptureKeyboard)
    {
        HandleKeyboardInput();
    }
}

void UIManager::ShowObjectPanel(bool show)
{
    m_displayObjectListPanel = show;
}

void UIManager::ShowPropertiesPanel(bool show)
{
    m_displayPropertiesPanel = show;
}

void UIManager::ShowFileDialog(bool show)
{
    m_displayFileDialog = show;
}

void UIManager::ShowParkourMapDialog(bool show)
{
    m_displayParkourMapDialog = show;
}

::Tool UIManager::GetActiveTool() const
{
    // Convert from IToolManager enum to local Tool enum
    // This assumes the enums are compatible
    return static_cast<Tool>(m_toolManager->GetActiveTool());
}

void UIManager::SetActiveTool(::Tool tool)
{
    // Convert to IToolManager enum
    m_toolManager->SetActiveTool(tool);
}

void UIManager::RenderImGuiToolbar()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_FirstUseEver);

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;

    bool toolbarOpen = true;
    if (ImGui::Begin("Toolbar##foo2", nullptr, windowFlags))
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::Text("Map Editor Tools");
        ImGui::PopFont();

        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        // Clamp X/Y so window stays fully visible
        if (pos.x < 0)
            pos.x = 0;
        if (pos.y < 0)
            pos.y = 0;
        if (pos.x + size.x > displaySize.x)
            pos.x = displaySize.x - size.x;
        if (pos.y + size.y > displaySize.y)
            pos.y = displaySize.y - size.y;

        // Apply corrected position
        ImGui::SetWindowPos(pos);

        ImGui::Separator();

        const char *toolNames[] = {"Select",   "Move",       "Rotate",       "Scale",
                                    "Add Cube", "Add Sphere", "Add Cylinder", "Add Model"};

        for (int i = 0; i < std::size(toolNames); i++)
        {
            // Store current tool state before potential change
            bool isCurrentTool = (GetActiveTool() == static_cast<Tool>(i));

            // Highlight current tool
            if (isCurrentTool)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.9f, 1.0f));
            }

            if (ImGui::Button(toolNames[i]))
            {
                SetActiveTool(static_cast<Tool>(i));

                if (GetActiveTool() == ADD_CUBE || GetActiveTool() == ADD_SPHERE ||
                    GetActiveTool() == ADD_CYLINDER || GetActiveTool() == ADD_MODEL)
                {
                    m_pendingObjectCreation = true;
                }
            }

            // Use stored state, not current m_currentTool
            if (isCurrentTool)
            {
                ImGui::PopStyleColor();
            }

            if (i < std::size(toolNames) - 1)
                ImGui::SameLine();
        }

        if (ImGui::Button("Save Map As..."))
        {
            // Use NFD to show save dialog
            nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
            nfdchar_t *outPath = nullptr;
            nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "map.json");
            if (result == NFD_OKAY)
            {
                const auto &objects = m_sceneManager->GetObjects();
                if (m_fileManager->SaveMap(outPath, objects))
                {
                    m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                }
                NFD_FreePath(outPath);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Map..."))
        {
            // Use NFD to show open dialog
            nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
            nfdchar_t *outPath = nullptr;
            nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
            if (result == NFD_OKAY)
            {
                std::vector<MapObject> loadedObjects;
                if (m_fileManager->LoadMap(outPath, loadedObjects))
                {
                    // Replace current objects in the scene manager
                    // SceneManager API doesn't expose a clear; emulate by removing one by one
                    while (!m_sceneManager->GetObjects().empty())
                    {
                        m_sceneManager->RemoveObject(static_cast<int>(m_sceneManager->GetObjects().size() - 1));
                    }
                    for (const auto &obj : loadedObjects)
                    {
                        m_sceneManager->AddObject(obj);
                    }
                    m_sceneManager->ClearSelection();
                    m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                }
                NFD_FreePath(outPath);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Quick Save") && !m_fileManager->GetCurrentlyLoadedMapFilePath().empty())
        {
            // Quick save to currently loaded file
            const auto& objects = m_sceneManager->GetObjects();
            m_fileManager->SaveMap(m_fileManager->GetCurrentlyLoadedMapFilePath(), objects);
        }
        ImGui::SameLine();

        // Show current file path
        ImGui::Separator();
        ImGui::Text("Current: %s", m_fileManager->GetCurrentlyLoadedMapFilePath().empty() ? m_fileManager->GetCurrentlyLoadedMapFilePath().c_str() : "No map loaded");

        // Model selection dropdown (only show when adding models)
        if (GetActiveTool() == ADD_MODEL)
        {
            ImGui::Text("Select Model:");
            if (ImGui::BeginCombo("##ModelSelect", m_currentlySelectedModelName.c_str()))
            {
                // Use model manager for available models
                const auto& availableModels = m_modelManager->GetAvailableModels();
                for (const auto &modelName : availableModels)
                {
                    bool isSelected = (m_currentlySelectedModelName == modelName);

                    if (ImGui::Selectable(modelName.c_str(), isSelected))
                    {
                        m_currentlySelectedModelName = modelName;
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        ImGui::Checkbox("Show Object Panel", &m_displayObjectListPanel);
        ImGui::SameLine();
        ImGui::Checkbox("Show Properties", &m_displayPropertiesPanel);
        ImGui::SameLine();
        if (ImGui::SliderInt("Increase/Decrease editor grid", &m_gridSizes, 50, 900))
        {
            if (m_gridSizes < 50)
            {
                m_gridSizes = 50;
            }
            // Ensure grid size is within valid range
            if (m_gridSizes > 900)
            {
                m_gridSizes = 900;
            }
        }

        // Process pending object creation
        ProcessPendingObjectCreation();
    }

    if (!toolbarOpen)
    {
        m_displayImGuiInterface = false;
    }

    ImGui::End();
}

void UIManager::RenderImGuiObjectPanel()
{
    const int screenWidth = GetScreenWidth();
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(screenWidth) - 250, 10),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(240, 400), ImGuiCond_FirstUseEver);

    // Use minimal flags for testing
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;

    bool objectPanelOpen = true;
    if (ImGui::Begin("Objects##foo1", &objectPanelOpen, windowFlags))
    {
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        // Clamp X/Y so window stays fully visible
        if (pos.x < 0)
            pos.x = 0;
        if (pos.y < 0)
            pos.y = 0;
        if (pos.x + size.x > displaySize.x)
            pos.x = displaySize.x - size.x;
        if (pos.y + size.y > displaySize.y)
            pos.y = displaySize.y - size.y;

        // Apply corrected position
        ImGui::SetWindowPos(pos);

        if (ImGui::Button("Add Object"))
        {
            // Create a default cube object
            MapObject newObj;
            newObj.SetObjectType(0); // Cube
            newObj.SetObjectName("New Cube " + std::to_string(m_sceneManager->GetObjects().size()));
            newObj.SetPosition({0.0f, 0.0f, 0.0f});
            newObj.SetScale({1.0f, 1.0f, 1.0f});
            m_sceneManager->AddObject(newObj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove") && m_sceneManager->GetSelectedObject() != nullptr)
        {
            // Remove selected object
            m_sceneManager->RemoveObject(m_sceneManager->GetSelectedObjectIndex());
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All"))
        {
            // Clear all objects - need to clear the vector
            // Since SceneManager doesn't have a ClearAll method, we'll clear selection and remove all objects
            m_sceneManager->ClearSelection();
            // Note: SceneManager doesn't expose a way to clear all objects, so this is limited
            // For now, just clear selection
        }

        ImGui::Separator();

        // List all objects
        const auto& objects = m_sceneManager->GetObjects();
        for (size_t i = 0; i < objects.size(); i++)
        {
            const auto &obj = objects[i];

            if (const bool isSelected = (static_cast<int>(i) == m_sceneManager->GetSelectedObjectIndex());
                ImGui::Selectable(obj.GetObjectName().c_str(), isSelected))
            {
                m_sceneManager->SelectObject(static_cast<int>(i));
            }

            // Show object info on hover
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Position: %.1f, %.1f, %.1f", obj.GetPosition().x, obj.GetPosition().y,
                            obj.GetPosition().z);
                ImGui::Text("Type: %s", obj.GetObjectType() == 0   ? "Cube"
                                        : obj.GetObjectType() == 1 ? "Sphere"
                                        : obj.GetObjectType() == 2 ? "Cylinder"
                                        : obj.GetObjectType() == 3 ? "Plane"
                                        : obj.GetObjectType() == 4
                                            ? "Ellipse"
                                            : obj.GetObjectType() == 5
                                                ? ("Model: " + obj.GetModelAssetName()).c_str()
                                                : "Unknown");
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

void UIManager::RenderImGuiPropertiesPanel()
{
    if (m_sceneManager->GetSelectedObject() == nullptr)
        return;

    auto& obj = *m_sceneManager->GetSelectedObject();
    const int screenHeight = GetScreenHeight();

    ImGui::SetNextWindowPos(ImVec2(10, static_cast<float>(screenHeight - 400)),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;
    bool propertiesPanelOpen = true;
    std::string nameLabel;
    if (ImGui::Begin("Properties##Panel", &propertiesPanelOpen, windowFlags))
    {
        // If nameLabel is empty, initialize it with the object's current name
        if (nameLabel.empty())
            nameLabel = obj.GetObjectName();

        // InputText requires a char array buffer, so we convert std::string to char[]
        // Alternatively, ImGui::InputText with std::string can be used but requires memory
        // allocation
        char buffer[256];
        strncpy(buffer, nameLabel.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = 0; // Ensure null-termination

        if (ImGui::InputText("Name##Name", buffer, sizeof(buffer)))
        {
            nameLabel = std::string(buffer);
            if (nameLabel.empty())
            {
                nameLabel = obj.GetObjectName(); // Prevent empty names
            }
            obj.SetObjectName(nameLabel); // Update the object's name
        }

        const char *types[] = {"Cube", "Sphere", "Cylinder", "Plane", "Ellipse", "Model"};
        int typeIndex = obj.GetObjectType();
        if (ImGui::Combo("Type", &typeIndex, types, IM_ARRAYSIZE(types)))
        {
            obj.SetObjectType(typeIndex);
        }

        float pos[3] = {obj.GetPosition().x, obj.GetPosition().y, obj.GetPosition().z};
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            obj.SetPosition({pos[0], pos[1], pos[2]});
        }

        float scale[3] = {obj.GetScale().x, obj.GetScale().y, obj.GetScale().z};
        float size[2] = {obj.GetPlaneSize().x, obj.GetPlaneSize().y};
        float radiusEllipse[2] = {obj.GetHorizontalRadius(), obj.GetVerticalRadius()};
        float radiusSphere = obj.GetSphereRadius();

        switch (obj.GetObjectType())
        {
        case 0: // Cube
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.SetScale({scale[0], scale[1], scale[2]});
            }
            break;

        case 1: // Sphere
            if (ImGui::DragFloat("Radius", &radiusSphere, 0.1f))
            {
                obj.SetSphereRadius(radiusSphere);
            }
            break;

        case 2: // Cylinder
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.SetScale({scale[0], scale[1], scale[2]});
            }
            break;

        case 3: // Plane
            if (ImGui::DragFloat2("Size", size, 0.1f))
            {
                obj.SetPlaneSize({size[0], size[1]});
            }
            break;

        case 4: // Ellipse
            if (ImGui::DragFloat2("Radius H/V", radiusEllipse, 0.1f))
            {
                obj.SetHorizontalRadius(radiusEllipse[0]);
                obj.SetVerticalRadius(radiusEllipse[1]);
            }
            break;

        case 5: // Model
            // Model selection dropdown
            ImGui::Text("Model:");
            if (ImGui::BeginCombo("##ModelSelect", obj.GetModelAssetName().c_str()))
            {
                // Use model manager for available models
                const auto& availableModels = m_modelManager->GetAvailableModels();
                for (const auto &modelName : availableModels)
                {
                    bool isSelected = (obj.GetModelAssetName() == modelName);

                    if (ImGui::Selectable(modelName.c_str(), isSelected))
                    {
                        obj.SetModelAssetName(modelName);
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Scale controls for models
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.SetScale({scale[0], scale[1], scale[2]});
            }
            break;
        }

        float rot[3] = {obj.GetRotation().x * RAD2DEG, obj.GetRotation().y * RAD2DEG,
                        obj.GetRotation().z * RAD2DEG};
        if (ImGui::DragFloat3("Rotation", rot, 1.0f))
        {
            obj.SetRotation({rot[0] * DEG2RAD, rot[1] * DEG2RAD, rot[2] * DEG2RAD});
        }

        float color[4] = {obj.GetColor().r / 255.0f, obj.GetColor().g / 255.0f,
                          obj.GetColor().b / 255.0f, obj.GetColor().a / 255.0f};

        if (ImGui::ColorEdit4("Color", color))
        {
            obj.SetColor({static_cast<unsigned char>(color[0] * 255),
                          static_cast<unsigned char>(color[1] * 255),
                          static_cast<unsigned char>(color[2] * 255),
                          static_cast<unsigned char>(color[3] * 255)});
        }
    }

    if (!propertiesPanelOpen)
    {
        m_displayPropertiesPanel = false;
    }

    ImGui::End();
}

void UIManager::RenderParkourMapDialog()
{
    if (m_displayParkourMapDialog)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(
            ImVec2(GetScreenWidth() * 0.5f - 250, GetScreenHeight() * 0.5f - 200),
            ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Parkour Maps", &m_displayParkourMapDialog, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Select a Parkour Map:");
            ImGui::Separator();

            // List all available parkour maps
            for (int i = 0; i < m_availableParkourMaps.size(); i++)
            {
                const auto &gameMap = m_availableParkourMaps[i];

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s (%.1f/5.0)",
                         gameMap.metadata.displayName.c_str(), gameMap.metadata.difficulty);
                if (ImGui::Selectable(buffer, m_currentlySelectedParkourMapIndex == i))
                {
                    m_currentlySelectedParkourMapIndex = i;
                }

                // Show tooltip with description on hover
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", gameMap.metadata.description.c_str());
                    ImGui::Text("Elements: %zu", gameMap.objects.size());
                    ImGui::EndTooltip();
                }
            }

            ImGui::Separator();

            // Action buttons
            if (ImGui::Button("Load Selected Map", ImVec2(150, 30)))
            {
                if (m_currentlySelectedParkourMapIndex >= 0 &&
                    m_currentlySelectedParkourMapIndex < m_availableParkourMaps.size())
                {
                    // TODO: Load parkour map through file manager
                    m_displayParkourMapDialog = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 30)))
            {
                m_displayParkourMapDialog = false;
            }

            // Show selected map details
            if (m_currentlySelectedParkourMapIndex >= 0 &&
                m_currentlySelectedParkourMapIndex < m_availableParkourMaps.size())
            {
                const auto &selectedGameMap =
                    m_availableParkourMaps[m_currentlySelectedParkourMapIndex];
                ImGui::Separator();
                ImGui::Text("Selected Map Details:");
                ImGui::Text("Name: %s", selectedGameMap.metadata.displayName.c_str());
                ImGui::Text("Description: %s", selectedGameMap.metadata.description.c_str());
                ImGui::Text("Difficulty: %.1f/5.0", selectedGameMap.metadata.difficulty);
                ImGui::Text("Elements: %zu", selectedGameMap.objects.size());
                ImGui::Text("Start: (%.1f, %.1f, %.1f)", selectedGameMap.metadata.startPosition.x,
                            selectedGameMap.metadata.startPosition.y,
                            selectedGameMap.metadata.startPosition.z);
                ImGui::Text("End: (%.1f, %.1f, %.1f)", selectedGameMap.metadata.endPosition.x,
                            selectedGameMap.metadata.endPosition.y,
                            selectedGameMap.metadata.endPosition.z);
            }
        }
        ImGui::End();
    }
}

void UIManager::RenderFileDialog()
{
    // TODO: Implement file dialog rendering
    // This is a complex dialog that needs to be extracted from Editor.cpp
    // For now, just close the dialog if it's open
    m_displayFileDialog = false;
}

void UIManager::HandleKeyboardInput()
{
    // Handle keyboard shortcuts for scene objects
    if (!m_displayFileDialog && IsKeyPressed(KEY_DELETE) && m_sceneManager->GetSelectedObject() != nullptr)
    {
        // Remove selected object
        m_sceneManager->RemoveObject(m_sceneManager->GetSelectedObjectIndex());
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_displayFileDialog)
        {
            m_displayFileDialog = false;
        }
        else
        {
            m_sceneManager->ClearSelection();
        }
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

void UIManager::OpenFileDialog(bool isLoad)
{
    m_isFileLoadDialog = isLoad;
    m_displayFileDialog = true;
    m_currentlySelectedFile.clear();
    m_newFileNameInput = isLoad ? "new_map.json" : "save_map.json";
    RefreshDirectoryItems();
}

void UIManager::RefreshDirectoryItems()
{
    m_currentDirectoryContents.clear();
    try
    {
        for (const auto &entry : fs::directory_iterator(m_currentWorkingDirectory))
        {
            m_currentDirectoryContents.push_back(entry.path().filename().string());
        }
        std::sort(m_currentDirectoryContents.begin(), m_currentDirectoryContents.end());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Failed to refresh directory items: %s", e.what());
    }
}

void UIManager::NavigateToDirectory(const std::string &path)
{
    std::string newPath = path;
    if (newPath == "..")
    {
        // Go up one directory
        fs::path currentPath(m_currentWorkingDirectory);
        if (currentPath.has_parent_path())
        {
            newPath = currentPath.parent_path().string();
        }
        else
        {
            return; // Already at root
        }
    }
    else
    {
        // Navigate to subdirectory
        newPath = m_currentWorkingDirectory + "/" + path;
    }

    // Check if the new path is a directory
    if (fs::is_directory(newPath))
    {
        m_currentWorkingDirectory = newPath;
        RefreshDirectoryItems();
    }
}

void UIManager::ProcessPendingObjectCreation()
{
    if (m_pendingObjectCreation)
    {
        // Create object through tool manager
        // Note: ToolManager expects SceneManager, but we have ISceneManager
        // For now, we'll handle object creation directly here
        MapObject newObj;
        std::string baseName = "New Object " + std::to_string(m_sceneManager->GetObjects().size());

        switch (GetActiveTool())
        {
        case ADD_CUBE:
            newObj.SetObjectType(0); // Cube
            newObj.SetObjectName(baseName + " (Cube)");
            break;
        case ADD_SPHERE:
            newObj.SetObjectType(1); // Sphere
            newObj.SetObjectName(baseName + " (Sphere)");
            break;
        case ADD_CYLINDER:
            newObj.SetObjectType(2); // Cylinder
            newObj.SetObjectName(baseName + " (Cylinder)");
            break;
        case ADD_MODEL:
            newObj.SetObjectType(5); // Model
            newObj.SetModelAssetName(m_currentlySelectedModelName);
            newObj.SetObjectName(m_currentlySelectedModelName + " " + std::to_string(m_sceneManager->GetObjects().size()));
            break;
        default:
            break;
        }

        // Set default position, scale, and other properties
        newObj.SetPosition({0.0f, 0.0f, 0.0f});
        newObj.SetScale({1.0f, 1.0f, 1.0f});
        newObj.SetRotation({0.0f, 0.0f, 0.0f});
        newObj.SetColor({255, 255, 255, 255}); // White color

        m_sceneManager->AddObject(newObj);
        m_pendingObjectCreation = false;
        SetActiveTool(SELECT);
    }
}

int UIManager::GetGridSize() const 
{ 
    return m_gridSizes; 
}