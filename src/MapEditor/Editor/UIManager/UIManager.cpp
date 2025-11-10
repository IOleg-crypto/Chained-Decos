//
// Created by Kilo Code
//

#include "UIManager.h"
#include "SkyboxBrowser.h"
#include "ObjectFactory.h"
#include "Editor/Editor.h"
#include "Engine/Kernel/Core/Kernel.h"
#include "Engine/Map/Core/MapLoader.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include <raymath.h>

namespace fs = std::filesystem;

UIManager::UIManager(Editor *editor, ISceneManager *sceneManager, IFileManager *fileManager,
                     IToolManager *toolManager, IModelManager *modelManager)
    : m_editor(editor), m_sceneManager(sceneManager), m_fileManager(fileManager),
      m_toolManager(toolManager), m_modelManager(modelManager), m_displayImGuiInterface(true),
      m_displayObjectListPanel(true), m_displayPropertiesPanel(true),
      m_pendingObjectCreation(false), m_displaySkyboxPanel(false), m_displayParkourMapDialog(false),
      m_currentlySelectedParkourMapIndex(0), m_gridSizes(900),
      m_skyboxBrowser(std::make_unique<SkyboxBrowser>(editor)),
      m_objectFactory(std::make_unique<ObjectFactory>(sceneManager, toolManager))
{
}

UIManager::~UIManager()
{
    // SkyboxBrowser handles its own cleanup
}

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

    // Render skybox panel (always call to handle cleanup even when closed)
    // Note: SkyboxBrowser handles its own Begin/End and can modify m_displaySkyboxPanel
    if (m_displaySkyboxPanel && m_skyboxBrowser)
    {
        m_skyboxBrowser->RenderPanel(m_displaySkyboxPanel);
    }

    // If skybox panel was closed via window X button, update checkbox state
    // (This is handled automatically by ImGui through the bool* parameter in Begin)

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


Tool UIManager::GetActiveTool() const
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

ImVec2 UIManager::ClampWindowPosition(const ImVec2 &desiredPos, ImVec2 &windowSize)
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

void UIManager::EnsureWindowInBounds()
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

void UIManager::RenderImGuiToolbar()
{
    ImVec2 windowSize(700, 300);
    ImVec2 desiredPos(10, 10);
    ImVec2 clampedPos = ClampWindowPosition(desiredPos, windowSize);

    ImGui::SetNextWindowPos(clampedPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    // Limit maximum window size to screen dimensions
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(100, 100),
        ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));

    // Enable docking for toolbar window - allow it to be docked if needed
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;

    bool toolbarOpen = true;
    if (ImGui::Begin("Toolbar##foo2", nullptr, windowFlags))
    {
        // Ensure window stays within screen bounds
        EnsureWindowInBounds();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::Text("Map Editor Tools");
        ImGui::PopFont();

        ImGui::Separator();

        const char *toolNames[] = {"Select",       "Move",      "Rotate",
                                   "Scale",        "Add Cube",  "Add Sphere",
                                   "Add Cylinder", "Add Model", "Add Spawn Zone"};

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
                    GetActiveTool() == ADD_CYLINDER || GetActiveTool() == ADD_MODEL ||
                    GetActiveTool() == ADD_SPAWN_ZONE)
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
                        m_sceneManager->RemoveObject(
                            static_cast<int>(m_sceneManager->GetObjects().size() - 1));
                    }
                    for (const auto &obj : loadedObjects)
                    {
                        m_sceneManager->AddObject(obj);
                    }
                    m_sceneManager->ClearSelection();
                    m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                    if (m_editor)
                    {
                        m_editor->ApplyMetadata(m_fileManager->GetCurrentMetadata());
                    }
                }
                NFD_FreePath(outPath);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Quick Save") && !m_fileManager->GetCurrentlyLoadedMapFilePath().empty())
        {
            // Quick save to currently loaded file
            const auto &objects = m_sceneManager->GetObjects();
            m_fileManager->SaveMap(m_fileManager->GetCurrentlyLoadedMapFilePath(), objects);
        }

        // Show current file path
        ImGui::Separator();
        ImGui::Text("Current: %s", m_fileManager->GetCurrentlyLoadedMapFilePath().empty()
                                       ? "No map loaded"
                                       : m_fileManager->GetCurrentlyLoadedMapFilePath().c_str());

        // Model selection dropdown (only show when adding models)
        if (GetActiveTool() == ADD_MODEL)
        {
            ImGui::Text("Select Model:");
            if (ImGui::BeginCombo("##ModelSelect", m_currentlySelectedModelName.c_str()))
            {
                // Use model manager for available models
                const auto &availableModels = m_modelManager->GetAvailableModels();
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
        ImGui::Checkbox("Show Skybox Settings", &m_displaySkyboxPanel);
        ImGui::SameLine();
        if (ImGui::SliderInt("Increase/Decrease editor grid", &m_gridSizes, 900, 10000))
        {
            if (m_gridSizes < 900)
            {
                m_gridSizes = 900;
            }
            // Ensure grid size is within valid range
            if (m_gridSizes > 10000)
            {
                m_gridSizes = 10000;
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
    ImVec2 windowSize(240, 400);
    ImVec2 desiredPos(static_cast<float>(screenWidth) - 250, 10);
    ImVec2 clampedPos = ClampWindowPosition(desiredPos, windowSize);

    ImGui::SetNextWindowPos(clampedPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    // Limit maximum window size to screen dimensions
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(100, 100),
        ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));

    // Enable docking for object panel - allow docking to sides
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_None; // Remove AlwaysAutoResize for better docking

    bool objectPanelOpen = true;
    if (ImGui::Begin("Objects##foo1", &objectPanelOpen, windowFlags))
    {
        // Ensure window stays within screen bounds
        EnsureWindowInBounds();
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
            // Since SceneManager doesn't have a ClearAll method, we'll clear selection and remove
            // all objects
            m_sceneManager->ClearSelection();
            // Note: SceneManager doesn't expose a way to clear all objects, so this is limited
            // For now, just clear selection
        }

        ImGui::Separator();

        // List all objects
        const auto &objects = m_sceneManager->GetObjects();
        for (size_t i = 0; i < objects.size(); i++)
        {
            const auto &obj = objects[i];

            if (const bool isSelected =
                    (static_cast<int>(i) == m_sceneManager->GetSelectedObjectIndex());
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
                                        : obj.GetObjectType() == 4 ? "Ellipse"
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

    auto &obj = *m_sceneManager->GetSelectedObject();
    const int screenHeight = GetScreenHeight();

    ImVec2 windowSize(300, 400);
    ImVec2 desiredPos(10, static_cast<float>(screenHeight - 400));
    ImVec2 clampedPos = ClampWindowPosition(desiredPos, windowSize);

    ImGui::SetNextWindowPos(clampedPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    // Limit maximum window size to screen dimensions
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(200, 200),
        ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));

    // Enable docking for properties panel
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_None; // Remove AlwaysAutoResize for better docking
    bool propertiesPanelOpen = true;
    std::string nameLabel;
    if (ImGui::Begin("Properties##Panel", &propertiesPanelOpen, windowFlags))
    {
        // Ensure window stays within screen bounds
        EnsureWindowInBounds();
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

        const char *types[] = {"Cube",    "Sphere", "Cylinder",  "Plane",
                               "Ellipse", "Model",  "Spawn Zone"};
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

        // Type-specific parameters (only show relevant ones for each type)
        switch (obj.GetObjectType())
        {
        case 0: // Cube
        {
            float scale[3] = {obj.GetScale().x, obj.GetScale().y, obj.GetScale().z};
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.SetScale({scale[0], scale[1], scale[2]});
            }
        }
        break;

        case 1: // Sphere
        {
            float radiusSphere = obj.GetSphereRadius();
            if (ImGui::DragFloat("Radius", &radiusSphere, 0.1f))
            {
                obj.SetSphereRadius(radiusSphere);
            }
        }
        break;

        case 2: // Cylinder
        {
            float scale[3] = {obj.GetScale().x, obj.GetScale().y, obj.GetScale().z};
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.SetScale({scale[0], scale[1], scale[2]});
            }
        }
        break;

        case 3: // Plane
        {
            float size[2] = {obj.GetPlaneSize().x, obj.GetPlaneSize().y};
            if (ImGui::DragFloat2("Size", size, 0.1f))
            {
                obj.SetPlaneSize({size[0], size[1]});
            }
        }
        break;

        case 4: // Ellipse
        {
            float radiusEllipse[2] = {obj.GetHorizontalRadius(), obj.GetVerticalRadius()};
            if (ImGui::DragFloat2("Radius H/V", radiusEllipse, 0.1f))
            {
                obj.SetHorizontalRadius(radiusEllipse[0]);
                obj.SetVerticalRadius(radiusEllipse[1]);
            }
        }
        break;

        case 5: // Model
        {
            // Model selection dropdown
            ImGui::Text("Model:");
            if (ImGui::BeginCombo("##ModelSelect", obj.GetModelAssetName().c_str()))
            {
                // Use model manager for available models
                const auto &availableModels = m_modelManager->GetAvailableModels();
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
            float scale[3] = {obj.GetScale().x, obj.GetScale().y, obj.GetScale().z};
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.SetScale({scale[0], scale[1], scale[2]});
            }
        }
        break;

        case 6: // Spawn Zone
        {
            // Spawn Zone only needs Position, no other parameters needed
        }
        break;
        }

        // Rotation - show for all types except Spawn Zone
        if (obj.GetObjectType() != 6)
        {
            float rot[3] = {obj.GetRotation().x * RAD2DEG, obj.GetRotation().y * RAD2DEG,
                            obj.GetRotation().z * RAD2DEG};
            if (ImGui::DragFloat3("Rotation", rot, 1.0f))
            {
                obj.SetRotation({rot[0] * DEG2RAD, rot[1] * DEG2RAD, rot[2] * DEG2RAD});
            }
        }

        // Color - show for all types except Spawn Zone
        if (obj.GetObjectType() != 6)
        {
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
    }

    if (!propertiesPanelOpen)
    {
        m_displayPropertiesPanel = false;
    }

    ImGui::End();
}

void UIManager::HandleKeyboardInput()
{
    // Handle keyboard shortcuts for scene objects
    if (IsKeyPressed(KEY_DELETE) && m_sceneManager->GetSelectedObject() != nullptr)
    {
        // Remove selected object
        m_sceneManager->RemoveObject(m_sceneManager->GetSelectedObjectIndex());
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        m_sceneManager->ClearSelection();
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

void UIManager::ProcessPendingObjectCreation()
{
    if (m_pendingObjectCreation && m_objectFactory)
    {
        m_objectFactory->CreateObject(GetActiveTool(), m_currentlySelectedModelName);
        m_pendingObjectCreation = false;
        SetActiveTool(SELECT);
    }
}

int UIManager::GetGridSize() const
{
    return m_gridSizes;
}


