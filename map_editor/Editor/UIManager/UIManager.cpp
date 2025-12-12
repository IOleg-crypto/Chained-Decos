
//
// Created by Kilo Code
//

#include "UIManager.h"
#include "../ECS/GameObject.h"
#include "../ECS/MeshRendererComponent.h"
#include "../ECS/TransformComponent.h"
#include "Editor/SceneManager/ISceneManager.h"
#include "ObjectFactory.h"
#include "SkyboxBrowser.h"
#include "map_editor/Editor/Editor.h"

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
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include <raymath.h>

namespace fs = std::filesystem;

EditorUIManager::EditorUIManager(const UIManagerConfig &config)
    : m_editor(config.editor), m_sceneManager(config.sceneManager),
      m_fileManager(config.fileManager), m_toolManager(config.toolManager),
      m_modelManager(config.modelManager), m_displayImGuiInterface(true),
      m_displayObjectListPanel(true), m_displayPropertiesPanel(true),
      m_pendingObjectCreation(false), m_displaySkyboxPanel(false), m_displayParkourMapDialog(false),
      m_currentlySelectedParkourMapIndex(0), m_gridSizes(config.initialGridSize),
      m_skyboxBrowser(std::make_unique<SkyboxBrowser>(config.editor)),
      m_objectFactory(std::make_unique<ObjectFactory>(config.sceneManager, config.toolManager))
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

    if (m_sceneManager->GetSelectedObject() != nullptr ||
        m_sceneManager->GetSelectedGameObject() != nullptr)
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
    // This assumes the enums are compatible
    return static_cast<Tool>(m_toolManager->GetActiveTool());
}

void EditorUIManager::SetActiveTool(::Tool tool)
{
    // Convert to IToolManager enum
    m_toolManager->SetActiveTool(tool);
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
            if (ImGui::MenuItem("Load Map..."))
            {
                if (m_sceneManager->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::LOAD_MAP;
                }
                else
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
                            // Use unified ClearScene to wipe both legacy and ECS objects
                            m_sceneManager->ClearScene();

                            for (const auto &obj : loadedObjects)
                            {
                                m_sceneManager->AddObject(obj);
                            }
                            // ClearScene already clears selection, so this might be redundant but
                            // safe
                            m_sceneManager->ClearSelection();
                            m_sceneManager->SetSceneModified(false); // Just loaded, clean state

                            m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                            if (m_editor)
                            {
                                m_editor->ApplyMetadata(m_fileManager->GetCurrentMetadata());
                            }
                        }
                        NFD_FreePath(outPath);
                    }
                }
            }
            if (ImGui::MenuItem("Quick Save", nullptr, false,
                                !m_fileManager->GetCurrentlyLoadedMapFilePath().empty()))
            {
                const auto &objects = m_sceneManager->GetObjects();
                m_fileManager->SaveMap(m_fileManager->GetCurrentlyLoadedMapFilePath(), objects);
            }

            // Back to Welcome Screen
            if (ImGui::MenuItem("Back to Welcome Screen"))
            {
                if (m_sceneManager->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_PROJECT;
                }
                else
                {
                    m_sceneManager->ClearScene();
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
        std::string mapName =
            m_fileManager->GetCurrentlyLoadedMapFilePath().empty()
                ? "Untitled"
                : fs::path(m_fileManager->GetCurrentlyLoadedMapFilePath()).filename().string();
        std::string infoText = "Map: " + mapName + " | Grid: " + std::to_string(m_gridSizes);

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
                const auto &availableModels = m_modelManager->GetAvailableModels();
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
    float mainMenuBarHeight = 19.0f; // Approx height of main menu
    ImGui::SetNextWindowPos(ImVec2(0, mainMenuBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, static_cast<float>(GetScreenHeight()) - mainMenuBarHeight),
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
            // Create a default cube object
            MapObject newObj;
            newObj.SetObjectType(0); // Cube
            newObj.SetObjectName("New Cube " + std::to_string(m_sceneManager->GetObjects().size()));
            newObj.SetPosition({0.0f, 0.0f, 0.0f});
            newObj.SetScale({1.0f, 1.0f, 1.0f});
            newObj.SetScale({1.0f, 1.0f, 1.0f});
            m_sceneManager->AddObject(newObj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Entity"))
        {
            auto go = std::make_unique<GameObject>(
                "New Entity " + std::to_string(m_sceneManager->GetGameObjects().size()));
            go->AddComponent<TransformComponent>();
            go->AddComponent<MeshRendererComponent>(); // Default cube
            m_sceneManager->AddGameObject(std::move(go));
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

        // ECS GameObjects
        const auto &gameObjects = m_sceneManager->GetGameObjects();
        if (!gameObjects.empty())
        {
            ImGui::Separator();
            ImGui::TextDisabled("ECS Entities");

            for (size_t i = 0; i < gameObjects.size(); i++)
            {
                auto &obj = gameObjects[i];
                GameObject *rawPtr = obj.get();
                bool isSelected = (rawPtr == m_sceneManager->GetSelectedGameObject());

                std::string label =
                    obj->GetName() + "##" + std::to_string(reinterpret_cast<uintptr_t>(rawPtr));

                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    m_sceneManager->SelectGameObject(rawPtr);
                }
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
    float mainMenuBarHeight = 19.0f;
    float panelWidth = 300.0f;
    ImGui::SetNextWindowPos(
        ImVec2(static_cast<float>(GetScreenWidth()) - panelWidth, mainMenuBarHeight),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(panelWidth, static_cast<float>(GetScreenHeight()) - mainMenuBarHeight),
        ImGuiCond_Always);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    bool propertiesPanelOpen = true;
    if (ImGui::Begin("Properties", &propertiesPanelOpen, windowFlags))
    {
        // Ensure window stays within screen bounds
        EnsureWindowInBounds();

        auto *selectedMapObj = m_sceneManager->GetSelectedObject();
        auto *selectedGameObj = m_sceneManager->GetSelectedGameObject();

        if (selectedMapObj != nullptr)
        {
            auto &obj = *selectedMapObj;

            // Name
            static std::string nameLabel;
            if (nameLabel.empty() || nameLabel != obj.GetObjectName())
                nameLabel = obj.GetObjectName();

            char buffer[256];
            strncpy(buffer, nameLabel.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = 0;

            if (ImGui::InputText("Name##Name", buffer, sizeof(buffer)))
            {
                nameLabel = std::string(buffer);
                if (nameLabel.empty())
                    nameLabel = obj.GetObjectName();
                obj.SetObjectName(nameLabel);
            }

            const char *types[] = {"Cube",    "Sphere", "Cylinder",  "Plane",
                                   "Ellipse", "Model",  "Spawn Zone"};
            int typeIndex = obj.GetObjectType();
            if (ImGui::Combo("Type", &typeIndex, types, IM_ARRAYSIZE(types)))
            {
                obj.SetObjectType(typeIndex);
            }

            ImGui::Separator();

            // Transform
            float pos[3] = {obj.GetPosition().x, obj.GetPosition().y, obj.GetPosition().z};
            if (ImGui::DragFloat3("Position", pos, 0.1f))
                obj.SetPosition({pos[0], pos[1], pos[2]});

            // Rotation (all except Spawn Zone)
            if (obj.GetObjectType() != 6)
            {
                float rot[3] = {obj.GetRotation().x * RAD2DEG, obj.GetRotation().y * RAD2DEG,
                                obj.GetRotation().z * RAD2DEG};
                if (ImGui::DragFloat3("Rotation", rot, 1.0f))
                    obj.SetRotation({rot[0] * DEG2RAD, rot[1] * DEG2RAD, rot[2] * DEG2RAD});
            }

            // Scale & Type Specifics
            switch (obj.GetObjectType())
            {
            case 0: // Cube
            case 2: // Cylinder
            case 5: // Model
            {
                if (obj.GetObjectType() == 5)
                {
                    ImGui::Text("Model Asset:");
                    if (ImGui::BeginCombo("##ModelSelect", obj.GetModelAssetName().c_str()))
                    {
                        const auto &availableModels = m_modelManager->GetAvailableModels();
                        for (const auto &modelName : availableModels)
                        {
                            bool isSelected = (obj.GetModelAssetName() == modelName);
                            if (ImGui::Selectable(modelName.c_str(), isSelected))
                                obj.SetModelAssetName(modelName);
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }

                float scale[3] = {obj.GetScale().x, obj.GetScale().y, obj.GetScale().z};
                if (ImGui::DragFloat3("Scale", scale, 0.1f))
                    obj.SetScale({scale[0], scale[1], scale[2]});
                break;
            }
            case 1: // Sphere
            {
                float r = obj.GetSphereRadius();
                if (ImGui::DragFloat("Radius", &r, 0.1f))
                    obj.SetSphereRadius(r);
                break;
            }
            case 3: // Plane
            {
                float s[2] = {obj.GetPlaneSize().x, obj.GetPlaneSize().y};
                if (ImGui::DragFloat2("Size", s, 0.1f))
                    obj.SetPlaneSize({s[0], s[1]});
                break;
            }
            case 4: // Ellipse
            {
                float r[2] = {obj.GetHorizontalRadius(), obj.GetVerticalRadius()};
                if (ImGui::DragFloat2("Radius H/V", r, 0.1f))
                {
                    obj.SetHorizontalRadius(r[0]);
                    obj.SetVerticalRadius(r[1]);
                }
                break;
            }
            }

            // Color
            if (obj.GetObjectType() != 6)
            {
                float c[4] = {obj.GetColor().r / 255.0f, obj.GetColor().g / 255.0f,
                              obj.GetColor().b / 255.0f, obj.GetColor().a / 255.0f};
                if (ImGui::ColorEdit4("Color", c))
                {
                    obj.SetColor({(unsigned char)(c[0] * 255), (unsigned char)(c[1] * 255),
                                  (unsigned char)(c[2] * 255), (unsigned char)(c[3] * 255)});
                }
            }
        }
        else if (selectedGameObj != nullptr)
        {
            ImGui::Text("Selected Entity: %s", selectedGameObj->GetName().c_str());

            char nameBuffer[256];
            strncpy(nameBuffer, selectedGameObj->GetName().c_str(), sizeof(nameBuffer));
            nameBuffer[sizeof(nameBuffer) - 1] = 0;
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            {
                selectedGameObj->SetName(std::string(nameBuffer));
            }

            ImGui::Separator();

            for (const auto &component : selectedGameObj->GetComponents())
            {
                component->OnInspectorGUI();
                ImGui::Separator();
            }

            if (ImGui::Button("Add Component"))
                ImGui::OpenPopup("AddComponentPopup");

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                if (ImGui::MenuItem("Transform"))
                    selectedGameObj->AddComponent<TransformComponent>();
                if (ImGui::MenuItem("Mesh Renderer"))
                    selectedGameObj->AddComponent<MeshRendererComponent>();
                ImGui::EndPopup();
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
    if (IsKeyPressed(KEY_DELETE) && (m_sceneManager->GetSelectedObject() != nullptr ||
                                     m_sceneManager->GetSelectedGameObject() != nullptr))
    {
        if (m_sceneManager->GetSelectedObject() != nullptr)
        {
            // Remove selected object
            m_sceneManager->RemoveObject(m_sceneManager->GetSelectedObjectIndex());
        }
        else if (m_sceneManager->GetSelectedGameObject() != nullptr)
        {
            // Remove selected game object
            m_sceneManager->RemoveGameObject(m_sceneManager->GetSelectedGameObject());
        }
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

void EditorUIManager::ProcessPendingObjectCreation()
{
    if (m_pendingObjectCreation && m_objectFactory)
    {
        m_objectFactory->CreateObject(GetActiveTool(), m_currentlySelectedModelName);
        m_pendingObjectCreation = false;
        SetActiveTool(SELECT);
    }
}

int EditorUIManager::GetGridSize() const
{
    return m_gridSizes;
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
                if (m_sceneManager->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::NEW_PROJECT;
                }
                else
                {
                    m_sceneManager->ClearScene();
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
                if (m_sceneManager->IsSceneModified())
                {
                    m_showSavePrompt = true;
                    m_pendingAction = PendingAction::OPEN_PROJECT;
                }
                else
                {
                    nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                    nfdchar_t *outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
                    if (result == NFD_OKAY)
                    {
                        std::vector<MapObject> loadedObjects;
                        if (m_fileManager->LoadMap(outPath, loadedObjects))
                        {
                            m_sceneManager->ClearScene();
                            for (const auto &obj : loadedObjects)
                                m_sceneManager->AddObject(obj);

                            m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                            m_sceneManager->SetSceneModified(false);
                            if (m_editor)
                                m_editor->ApplyMetadata(m_fileManager->GetCurrentMetadata());

                            m_displayWelcomeScreen = false;
                        }
                        NFD_FreePath(outPath);
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
            std::string currentPath = m_fileManager->GetCurrentlyLoadedMapFilePath();
            if (currentPath.empty())
            {
                // Need to ask for path if new map
                nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "map.json");
                if (result == NFD_OKAY)
                {
                    currentPath = outPath;
                    NFD_FreePath(outPath);
                }
                else
                {
                    // Cancelled save, abort action or stay?
                    // Let's abort the whole action if save cancelled
                    ImGui::CloseCurrentPopup();
                    m_showSavePrompt = false;
                    m_pendingAction = PendingAction::NONE;
                    ImGui::EndPopup();
                    return;
                }
            }

            // Save logic
            std::vector<MapObject> objects;
            // MapObject objects
            for (const auto &obj : m_sceneManager->GetObjects())
            {
                objects.push_back(obj);
            }
            // Note: ECS Objects not yet serialized in SaveMap??
            // Currently using legacy SaveMap which takes vector<MapObject>
            // TODO: Update SaveMap to handle GameObject serialization

            if (m_fileManager->SaveMap(currentPath, objects))
            {
                m_fileManager->SetCurrentlyLoadedMapFilePath(currentPath);
                m_sceneManager->SetSceneModified(false); // Changes saved

                // Now proceed with pending action
                ImGui::CloseCurrentPopup();
                m_showSavePrompt = false;

                // Execute deferred action
                if (m_pendingAction == PendingAction::NEW_PROJECT)
                {
                    m_sceneManager->ClearScene();
                    if (m_editor)
                        m_editor->SetSkyboxTexture("");
                    m_displayWelcomeScreen = false;
                }
                else if (m_pendingAction == PendingAction::OPEN_PROJECT ||
                         m_pendingAction == PendingAction::LOAD_MAP)
                {
                    // Open NFD for loading
                    // We need to do this OUTSIDE the popup or it might interact weirdly
                    // But NFD blocks... so it's fine.
                    nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                    nfdchar_t *outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
                    if (result == NFD_OKAY)
                    {
                        std::vector<MapObject> loadedObjects;
                        if (m_fileManager->LoadMap(outPath, loadedObjects))
                        {
                            m_sceneManager->ClearScene();
                            for (const auto &obj : loadedObjects)
                                m_sceneManager->AddObject(obj);
                            m_sceneManager->ClearSelection();
                            m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                            if (m_editor)
                                m_editor->ApplyMetadata(m_fileManager->GetCurrentMetadata());
                            m_displayWelcomeScreen = false;
                        }
                        NFD_FreePath(outPath);
                    }
                }

                m_pendingAction = PendingAction::NONE;
            }
        }

        ImGui::SameLine();

        // DON'T SAVE
        if (ImGui::Button("Don't Save", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_showSavePrompt = false;
            m_sceneManager->SetSceneModified(false); // Discard changes

            // Proceed pending action
            if (m_pendingAction == PendingAction::NEW_PROJECT)
            {
                m_sceneManager->ClearScene();
                if (m_editor)
                    m_editor->SetSkyboxTexture("");
                m_displayWelcomeScreen = false;
            }
            else if (m_pendingAction == PendingAction::OPEN_PROJECT ||
                     m_pendingAction == PendingAction::LOAD_MAP)
            {
                nfdfilteritem_t filterItem[1] = {{"JSON", "json"}};
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
                if (result == NFD_OKAY)
                {
                    std::vector<MapObject> loadedObjects;
                    if (m_fileManager->LoadMap(outPath, loadedObjects))
                    {
                        m_sceneManager->ClearScene();
                        for (const auto &obj : loadedObjects)
                            m_sceneManager->AddObject(obj);
                        m_sceneManager->ClearSelection();
                        m_fileManager->SetCurrentlyLoadedMapFilePath(outPath);
                        if (m_editor)
                            m_editor->ApplyMetadata(m_fileManager->GetCurrentMetadata());
                        m_displayWelcomeScreen = false;
                    }
                    NFD_FreePath(outPath);
                }
            }
            m_pendingAction = PendingAction::NONE;
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
