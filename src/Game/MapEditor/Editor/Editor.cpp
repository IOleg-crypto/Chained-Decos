//
//

#include "Editor.h"
#include "../MapFileManager/MapFileManager.h"
#include "../../Game/Map/MapLoader.h"  // Include the new comprehensive map loader
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <misc/cpp/imgui_stdlib.h>
#include <raylib.h>
#include <rlImGui.h>

#include <raymath.h>
#include <string>

namespace fs = std::filesystem;

Editor::Editor()
    : m_cameraController(std::make_shared<CameraController>()), m_selectedObjectIndex(-1),
      m_currentTool(SELECT), m_showImGui(true), m_showObjectPanel(true),
      m_showPropertiesPanel(true), m_shouldAddObject(false), m_modelsInitialized(false),
      m_showFileDialog(false), m_isLoadDialog(true), m_showNewFolderDialog(false),
      m_showDeleteDialog(false), m_showParkourDialog(false), m_selectedParkourMap(0)
{
    // Initialize file dialog to project root
    m_currentDirectory = PROJECT_ROOT_DIR;
    m_newFileName = "new_map.json";
    RefreshDirectoryItems();
}

Editor::~Editor() = default;

std::shared_ptr<CameraController> Editor::GetCameraController() const { return m_cameraController; }

void Editor::Update()
{
    // Handle user input and update camera
    HandleInput();
}

void Editor::Render()
{
    // Render all objects in the scene
    for (auto &obj : m_objects)
    {
        RenderObject(obj);
    }
}

void Editor::RenderImGui()
{
    // Use simple rlImGui approach
    rlImGuiBegin();

    // Render all ImGui panels in specific order
    RenderImGuiToolbar();

    if (m_showObjectPanel)
    {
        RenderImGuiObjectPanel();
    }

    if (m_selectedObjectIndex >= 0)
    {
        RenderImGuiPropertiesPanel();
    }

    // Render file dialog if shown
    if (m_showFileDialog)
    {
        RenderFileDialog();
    }

    // Render new folder dialog if shown
    RenderNewFolderDialog();

    // Render delete confirmation dialog if shown
    RenderDeleteConfirmDialog();

    // Render parkour map dialog if shown
    RenderParkourMapDialog();

    rlImGuiEnd();
}

void Editor::HandleInput()
{
    // Get ImGui IO for input handling
    const ImGuiIO &io = ImGui::GetIO();

    // Handle mouse input for object selection only when ImGui is not capturing
    if (!io.WantCaptureMouse)
    {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            m_cameraController->Update();
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (m_currentTool == SELECT)
            {
                PickObject();
            }
        }
    }

    // Handle keyboard input only when ImGui is not capturing
    if (!io.WantCaptureKeyboard)
    {
        HandleKeyboardInput();
    }
}

void Editor::AddObject(const MapObject &obj)
{
    // Add new object to the scene
    m_objects.push_back(obj);
}

void Editor::RemoveObject(const int index)
{
    // Remove object by index and update selection
    if (index >= 0 && index < m_objects.size())
    {
        m_objects.erase(m_objects.begin() + index);
        if (m_selectedObjectIndex == index)
        {
            m_selectedObjectIndex = -1;
        }
        else if (m_selectedObjectIndex > index)
        {
            m_selectedObjectIndex--;
        }
    }
}

void Editor::SelectObject(const int index)
{
    // Clear previous selection
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < m_objects.size())
    {
        m_objects[m_selectedObjectIndex].SetSelected(false);
    }

    m_selectedObjectIndex = index;

    // Set new selection
    if (index >= 0 && index < m_objects.size())
    {
        m_objects[index].SetSelected(true);
    }
}

void Editor::ClearSelection()
{
    // Clear current object selection
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < m_objects.size())
    {
        m_objects[m_selectedObjectIndex].SetSelected(false);
    }
    m_selectedObjectIndex = -1;
}

void Editor::SaveMap(const std::string &filename)
{
    // Convert MapObjects to SerializableObjects for saving
    std::vector<SerializableObject> serializableObjects;

    for (auto &obj : m_objects)
    {
        SerializableObject serializableObj;
        serializableObj.position = obj.GetPosition();
        serializableObj.scale = obj.GetScale();
        serializableObj.rotation = obj.GetRotation();
        serializableObj.color = obj.GetColor();
        serializableObj.name = obj.GetName();
        serializableObj.type = obj.GetType();
        serializableObj.modelName = obj.GetModelName();
        serializableObjects.push_back(serializableObj);
    }

    // Save map to file
    if (MapFileManager::SaveMap(serializableObjects, filename))
    {
        std::cout << "Map saved successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to save map!" << std::endl;
    }
}

void Editor::LoadMap(const std::string &filename)
{
    // Load map from file
    std::vector<SerializableObject> serializableObjects;

    if (MapFileManager::LoadMap(serializableObjects, filename))
    {
        // Clear current scene
        m_objects.clear();
        m_selectedObjectIndex = -1;

        // Convert SerializableObjects back to MapObjects
        for (const auto &[position, scale, rotation, color, name, type, modelName] :
             serializableObjects)
        {
            MapObject obj;
            obj.SetPosition(position);
            obj.SetScale(scale);
            obj.SetRotation(rotation);
            obj.SetColor(color);
            obj.SetName(name);
            obj.SetType(type);
            obj.SetModelName(modelName);
            obj.SetSelected(false);

            m_objects.push_back(obj);
        }

        std::cout << "Map loaded successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to load map!" << std::endl;
    }
}

void Editor::ExportMapForGame(const std::string &filename)
{
    // Create a GameMap from current editor objects
    GameMap gameMap;

    // Set basic metadata
    gameMap.metadata.name = m_mapFileName.empty() ? "exported_map" : m_mapFileName;
    gameMap.metadata.displayName = "Exported Map";
    gameMap.metadata.description = "Map exported from editor";
    gameMap.metadata.author = "Map Editor";
    gameMap.metadata.version = "1.0";
    gameMap.metadata.difficulty = 1.0f;
    gameMap.metadata.skyColor = SKYBLUE;
    gameMap.metadata.groundColor = DARKGREEN;

    // Convert MapObjects to MapObjectData
    for (const auto& obj : m_objects)
    {
        MapObjectData objectData;

        objectData.name = obj.GetName();
        objectData.position = obj.GetPosition();
        objectData.rotation = obj.GetRotation();
        objectData.scale = obj.GetScale();
        objectData.color = obj.GetColor();
        objectData.modelName = obj.GetModelName();

        // Convert type (MapObject uses 0-5, MapObjectData uses enum)
        switch (obj.GetType())
        {
            case 0: objectData.type = MapObjectType::CUBE; break;
            case 1: objectData.type = MapObjectType::SPHERE; break;
            case 2: objectData.type = MapObjectType::CYLINDER; break;
            case 3: objectData.type = MapObjectType::PLANE; break;
            case 5: objectData.type = MapObjectType::MODEL; break;
            default: objectData.type = MapObjectType::CUBE; break;
        }

        // Set shape-specific properties
        switch (obj.GetType())
        {
            case 1: // Sphere
                objectData.radius = obj.GetRadiusSphere();
                break;
            case 2: // Cylinder
                objectData.radius = obj.GetScale().x;
                objectData.height = obj.GetScale().y;
                break;
            case 3: // Plane
                objectData.size = obj.GetSize();
                break;
        }

        gameMap.objects.push_back(objectData);
    }

    // Save using the new comprehensive format
    if (SaveGameMap(gameMap, filename))
    {
        std::cout << "Map exported for game successfully!" << std::endl;
        std::cout << "Saved " << gameMap.objects.size() << " objects" << std::endl;
    }
    else
    {
        std::cout << "Failed to export map for game!" << std::endl;
    }
}

void Editor::RenderImGuiObjectPanel()
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

        if (ImGui::Button("Add Object"))
        {
            MapObject newObj;
            newObj.SetName("New Object " + std::to_string(m_objects.size()));
            AddObject(newObj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove") && m_selectedObjectIndex >= 0)
        {
            RemoveObject(m_selectedObjectIndex);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All"))
        {
            m_objects.clear();
        }

        ImGui::Separator();

        // List all objects
        for (int i = 0; i < m_objects.size(); i++)
        {
            auto &obj = m_objects[i];

            if (const bool isSelected = (i == m_selectedObjectIndex);
                ImGui::Selectable(obj.GetName().c_str(), isSelected))
            {
                SelectObject(i);
            }

            // Show object info on hover
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Position: %.1f, %.1f, %.1f", obj.GetPosition().x, obj.GetPosition().y,
                            obj.GetPosition().z);
                ImGui::Text("Type: %s", obj.GetType() == 0   ? "Cube"
                                        : obj.GetType() == 1 ? "Sphere"
                                        : obj.GetType() == 2 ? "Cylinder"
                                        : obj.GetType() == 3 ? "Plane"
                                        : obj.GetType() == 4 ? "Ellipse"
                                        : obj.GetType() == 5
                                            ? ("Model: " + obj.GetModelName()).c_str()
                                            : "Unknown");
                ImGui::EndTooltip();
            }
        }
    }

    // If window was closed, don't show it next frame
    if (!objectPanelOpen)
    {
        m_showObjectPanel = false;
    }

    ImGui::End();
}

void Editor::RenderObject(MapObject &obj)
{
    // Choose color based on selection state
    Color drawColor = obj.GetSelected() ? YELLOW : obj.GetColor();

    // Render object based on its type
    switch (obj.GetType())
    {
    case 0: // Cube
        DrawCube(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z,
                 drawColor);
        if (obj.GetSelected())
        {
            DrawCubeWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z,
                          RED);
        }
        break;
    case 1: // Sphere
        DrawSphere(obj.GetPosition(), obj.GetScale().x, drawColor);
        if (obj.GetSelected())
        {
            DrawSphereWires(obj.GetPosition(), obj.GetRadiusSphere(), 5, 5, RED);
        }
        break;
    case 2: // Cylinder
        DrawCylinder(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().y, 8,
                     drawColor);
        if (obj.GetSelected())
        {
            DrawCylinderWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().x,
                              obj.GetScale().y, 8, RED);
        }
        break;
    case 3:
        DrawPlane(obj.GetPosition(), obj.GetSize(), drawColor);
        break;
    case 4:
        DrawEllipse(obj.GetPosition().x, obj.GetPosition().y, obj.GetRadiusH(), obj.GetRadiusV(),
                    drawColor);
        break;
    case 5: // 3D Model
        if (!obj.GetModelName().empty())
        {
            // Ensure models are loaded before trying to use them
            EnsureModelsLoaded();

            // Get model safely without exceptions
            Model *modelPtr = GetModelSafe(obj.GetModelName());

            if (modelPtr != nullptr)
            {
                // Calculate rotation from Euler angles
                Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};
                float rotationAngle = obj.GetRotation().y * RAD2DEG;

                // Draw the model with transformations
                DrawModelEx(*modelPtr, obj.GetPosition(), rotationAxis, rotationAngle,
                            obj.GetScale(), drawColor);

                // Draw wireframe if selected
                if (obj.GetSelected())
                {
                    DrawModelWiresEx(*modelPtr, obj.GetPosition(), rotationAxis, rotationAngle,
                                     obj.GetScale(), RED);
                }
            }
            else
            {
                // Fallback to drawing a cube if model not found
                DrawCube(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z,
                         RED);
                if (obj.GetSelected())
                {
                    DrawCubeWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y,
                                  obj.GetScale().z, RED);
                }

                // Draw text label to show it's a missing model
                DrawText(("Missing: " + obj.GetModelName()).c_str(), obj.GetPosition().x - 50,
                         obj.GetPosition().y + obj.GetScale().y + 10, 20, RED);
            }
        }
        else
        {
            // No model name specified
            DrawCube(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z, GRAY);
            if (obj.GetSelected())
            {
                DrawCubeWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y,
                              obj.GetScale().z, RED);
            }
        }
        break;
    }
}

void Editor::RenderImGuiToolbar()
{

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_FirstUseEver);

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;

    bool toolbarOpen = true;
    if (ImGui::Begin("Toolbar##foo2", &toolbarOpen, windowFlags))
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::Text("Map Editor Tools");
        ImGui::PopFont();

        ImGui::Separator();

        const char *toolNames[] = {"Select",   "Move",       "Rotate",       "Scale",
                                   "Add Cube", "Add Sphere", "Add Cylinder", "Add Model"};

        for (int i = 0; i < std::size(toolNames); i++)
        {
            // Store current tool state before potential change
            bool isCurrentTool = (m_currentTool == i);

            // Highlight current tool
            if (isCurrentTool)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.9f, 1.0f));
            }

            if (ImGui::Button(toolNames[i]))
            {
                m_currentTool = static_cast<Tool>(i);

                if (m_currentTool == ADD_CUBE || m_currentTool == ADD_SPHERE ||
                    m_currentTool == ADD_CYLINDER || m_currentTool == ADD_MODEL)
                {
                    m_shouldAddObject = true;
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
            OpenFileDialog(false); // false for Save dialog
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Map..."))
        {
            OpenFileDialog(true); // true for Load dialog
        }
        ImGui::SameLine();
        if (ImGui::Button("Quick Save") && !m_mapFileName.empty())
        {
            SaveMap(m_mapFileName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Export for Game"))
        {
            // Open save dialog for game export
            m_isLoadDialog = false;
            m_showFileDialog = true;
            m_selectedFile.clear();
            m_newFileName = "game_map.json";
            RefreshDirectoryItems();
        }

        ImGui::Separator();

        // Parkour map tools
        if (ImGui::Button("Load Parkour Map"))
        {
            ShowParkourMapSelector();
        }
        ImGui::SameLine();
        if (ImGui::Button("Generate Parkour Map"))
        {
            ShowParkourMapSelector();
        }

        // Show current file path
        ImGui::Text("Current: %s", m_mapFileName.c_str());

        // Model selection dropdown (only show when adding models)
        if (m_currentTool == ADD_MODEL)
        {
            // Ensure models are loaded for toolbar
            EnsureModelsLoaded();

            ImGui::Text("Select Model:");
            if (ImGui::BeginCombo("##ModelSelect", m_selectedModelName.c_str()))
            {
                for (const auto &modelName : m_availableModels)
                {
                    bool isSelected = (m_selectedModelName == modelName);
                    if (ImGui::Selectable(modelName.c_str(), isSelected))
                    {
                        m_selectedModelName = modelName;
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

        ImGui::Checkbox("Show Object Panel", &m_showObjectPanel);
        ImGui::SameLine();
        ImGui::Checkbox("Show Properties", &m_showPropertiesPanel);

        // TraceLog(LOG_INFO, TextFormat("Current Tool: %d", m_currentTool));
    }

    if (m_shouldAddObject)
    {
        MapObject newObj;
        newObj.SetName("New Object " + std::to_string(m_objects.size()));

        switch (m_currentTool)
        {
        case ADD_CUBE:
            newObj.SetType(0);
            break;
        case ADD_SPHERE:
            newObj.SetType(1);
            break;
        case ADD_CYLINDER:
            newObj.SetType(2);
            break;
        case ADD_MODEL:
            // Ensure models are loaded before adding model objects
            EnsureModelsLoaded();

            newObj.SetType(5);
            newObj.SetModelName(m_selectedModelName);
            newObj.SetName(m_selectedModelName + " " + std::to_string(m_objects.size()));
            break;
        default:
            break;
        }

        AddObject(newObj);

        m_shouldAddObject = false;
        m_currentTool = SELECT;
    }

    if (!toolbarOpen)
    {
        m_showImGui = false;
    }

    ImGui::End();
}

void Editor::PickObject()
{
    const Camera &camera = GetCameraController()->GetCamera();
    const Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);

    int pickedIndex = -1;
    float minDistance = FLT_MAX;

    for (int i = 0; i < m_objects.size(); ++i)
    {
        auto &obj = m_objects[i];
        const BoundingBox box = {
            Vector3{obj.GetPosition().x - obj.GetScale().x, obj.GetPosition().y - obj.GetScale().y,
                    obj.GetPosition().z - obj.GetScale().z},
            Vector3{obj.GetPosition().x + obj.GetScale().x, obj.GetPosition().y + obj.GetScale().y,
                    obj.GetPosition().z + obj.GetScale().z}};

        if (const RayCollision collision = GetRayCollisionBox(ray, box);
            collision.hit && collision.distance < minDistance)
        {
            minDistance = collision.distance;
            pickedIndex = i;
        }
        // If we have object selected before using properties panel , we cancel coloring other
        // objects , besides object took by mouse(MOUSE_LEFT_BUTTON)
        if (obj.GetSelected() && pickedIndex != i)
        {
            obj.SetSelected(false);
        }
    }

    m_selectedObjectIndex = pickedIndex;

    if (m_selectedObjectIndex != -1)
    {
        auto &obj = m_objects[m_selectedObjectIndex];
        obj.SetSelected(true);
        TraceLog(LOG_INFO, "Picked object %d", m_selectedObjectIndex);
    }
}

void Editor::RenderImGuiPropertiesPanel()
{
    if (m_selectedObjectIndex < 0 || m_selectedObjectIndex >= m_objects.size())
        return;

    MapObject &obj = m_objects[m_selectedObjectIndex];
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
            nameLabel = obj.GetName();

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
                nameLabel = obj.GetName(); // Prevent empty names
            }
            obj.SetName(nameLabel); // Update the object's name
        }

        const char *types[] = {"Cube", "Sphere", "Cylinder", "Plane", "Ellipse", "Model"};
        int typeIndex = obj.GetType();
        if (ImGui::Combo("Type", &typeIndex, types, IM_ARRAYSIZE(types)))
        {
            obj.SetType(typeIndex);
        }

        float pos[3] = {obj.GetPosition().x, obj.GetPosition().y, obj.GetPosition().z};
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            obj.SetPosition({pos[0], pos[1], pos[2]});
        }

        float scale[3] = {obj.GetScale().x, obj.GetScale().y, obj.GetScale().z};
        float size[2] = {obj.GetSize().x, obj.GetSize().y};
        float radiusEllipse[2] = {obj.GetRadiusH(), obj.GetRadiusV()};
        float radiusSphere = obj.GetRadiusSphere();

        switch (obj.GetType())
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
                obj.SetRadiusSphere(radiusSphere);
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
                obj.SetSize({size[0], size[1]});
            }
            break;

        case 4: // Ellipse
            if (ImGui::DragFloat2("Radius H/V", radiusEllipse, 0.1f))
            {
                obj.SetRadiusH(radiusEllipse[0]);
                obj.SetRadiusV(radiusEllipse[1]);
            }
            break;

        case 5: // Model
            // Ensure models are loaded for properties panel
            EnsureModelsLoaded();

            // Model selection dropdown
            ImGui::Text("Model:");
            if (ImGui::BeginCombo("##ModelSelect", obj.GetModelName().c_str()))
            {
                for (const auto &modelName : m_availableModels)
                {
                    bool isSelected = (obj.GetModelName() == modelName);
                    if (ImGui::Selectable(modelName.c_str(), isSelected))
                    {
                        obj.SetModelName(modelName);
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
        m_showPropertiesPanel = false;
    }

    ImGui::End();
}

void Editor::HandleKeyboardInput()
{
    // Handle keyboard shortcuts for file dialog
    if (m_showFileDialog && IsKeyPressed(KEY_DELETE) && !m_selectedFile.empty())
    {
        DeleteFolder(m_selectedFile);
    }
    // Handle keyboard shortcuts for scene objects
    else if (!m_showFileDialog && IsKeyPressed(KEY_DELETE) && m_selectedObjectIndex >= 0)
    {
        RemoveObject(m_selectedObjectIndex);
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_showFileDialog)
        {
            m_showFileDialog = false;
        }
        else
        {
            ClearSelection();
        }
    }

    // Toggle UI panels with different keys
    if (IsKeyPressed(KEY_TWO))
    {
        m_showObjectPanel = !m_showObjectPanel;
    }

    if (IsKeyPressed(KEY_F))
    {
        m_showPropertiesPanel = !m_showPropertiesPanel;
    }
}

void Editor::EnsureModelsLoaded()
{
    if (!m_modelsInitialized)
    {
        TraceLog(LOG_INFO, "Loading models...");

        bool loadSuccess = false;
        try
        {
            m_models.LoadModelsFromJson(PROJECT_ROOT_DIR "/src/models.json");
            loadSuccess = true;
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Failed to load models from JSON: %s", e.what());
            loadSuccess = false;
        }

        if (loadSuccess)
        {
            try
            {
                m_availableModels = m_models.GetAvailableModels();
                m_modelsInitialized = true;
                TraceLog(LOG_INFO, "Models loaded successfully! Available models: %zu",
                         m_availableModels.size());
            }
            catch (const std::exception &e)
            {
                TraceLog(LOG_ERROR, "Failed to get available models list: %s", e.what());
                // Fallback to hardcoded list if GetAvailableModels fails
                m_availableModels = {"arc"};
                m_modelsInitialized = true;
                TraceLog(LOG_WARNING, "Using fallback model list");
            }
        }
        else
        {
            // Complete failure - use fallback
            TraceLog(LOG_WARNING, "Models failed to load, using fallback model list");
            m_availableModels = {"arc"};
            // Don't set m_modelsInitialized to true, so we can retry later
        }
    }
}

Model *Editor::GetModelSafe(const std::string &modelName)
{
    if (!m_modelsInitialized || modelName.empty())
    {
        return nullptr;
    }

    // Check if model exists in available models list first
    auto it = std::find(m_availableModels.begin(), m_availableModels.end(), modelName);
    if (it == m_availableModels.end())
    {
        TraceLog(LOG_WARNING, "Model '%s' not found in available models list", modelName.c_str());
        return nullptr;
    }

    // Try to get model, but still handle potential exception safely
    try
    {
        return &m_models.GetModelByName(modelName);
    }
    catch (const std::exception &)
    {
        // Only catch standard exceptions, not system errors
        TraceLog(LOG_WARNING, "Model '%s' found in list but failed to load", modelName.c_str());
        return nullptr;
    }
}

void Editor::OpenFileDialog(bool isLoad)
{
    m_isLoadDialog = isLoad;
    m_showFileDialog = true;
    m_selectedFile.clear();

    // Reset new file name for save dialog
    if (!isLoad)
    {
        m_newFileName = "new_map.json";
    }

    RefreshDirectoryItems();
}

void Editor::RefreshDirectoryItems()
{
    m_directoryItems.clear();

    try
    {
        if (!fs::exists(m_currentDirectory) || !fs::is_directory(m_currentDirectory))
        {
            m_currentDirectory = PROJECT_ROOT_DIR;
        }

        // Add parent directory option (unless we're at root)
        fs::path currentPath(m_currentDirectory);
        if (currentPath.has_parent_path() && currentPath != currentPath.root_path())
        {
            m_directoryItems.push_back("../");
        }

        // Add directories first
        for (const auto &entry : fs::directory_iterator(m_currentDirectory))
        {
            if (entry.is_directory())
            {
                std::string name = entry.path().filename().string();
                if (!name.empty() && name[0] != '.') // Skip hidden directories
                {
                    m_directoryItems.push_back(name + "/");
                }
            }
        }

        // Add files (only .json files for maps)
        for (const auto &entry : fs::directory_iterator(m_currentDirectory))
        {
            if (entry.is_regular_file())
            {
                std::string fileName = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                // Show .json files for maps
                if (extension == ".json" || extension == ".map")
                {
                    m_directoryItems.push_back(fileName);
                }
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        TraceLog(LOG_ERROR, "File system error: %s", e.what());
        m_currentDirectory = PROJECT_ROOT_DIR;
        // Try again with project root
        try
        {
            for (const auto &entry : fs::directory_iterator(m_currentDirectory))
            {
                if (entry.is_directory())
                {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && name[0] != '.')
                    {
                        m_directoryItems.push_back(name + "/");
                    }
                }
                else if (entry.is_regular_file())
                {
                    std::string fileName = entry.path().filename().string();
                    std::string extension = entry.path().extension().string();
                    if (extension == ".json" || extension == ".map")
                    {
                        m_directoryItems.push_back(fileName);
                    }
                }
            }
        }
        catch (...)
        {
            TraceLog(LOG_ERROR, "Failed to read directory");
        }
    }
}

void Editor::NavigateToDirectory(const std::string &path)
{
    try
    {
        fs::path newPath;

        if (path == "../")
        {
            newPath = fs::path(m_currentDirectory).parent_path();
        }
        else
        {
            newPath = fs::path(m_currentDirectory) / path;
        }

        if (fs::exists(newPath) && fs::is_directory(newPath))
        {
            m_currentDirectory = newPath.string();
            RefreshDirectoryItems();
        }
    }
    catch (const fs::filesystem_error &e)
    {
        TraceLog(LOG_ERROR, "Navigation error: %s", e.what());
    }
}

void Editor::RenderFileDialog()
{
    const char *title = m_isLoadDialog ? "Load Map" : "Save Map As";

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() * 0.5f - 300, GetScreenHeight() * 0.5f - 200),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin(title, &m_showFileDialog, ImGuiWindowFlags_NoCollapse))
    {
        // Current directory display
        ImGui::Text("Directory: %s", m_currentDirectory.c_str());

        // Position buttons in top-right corner
        float buttonWidth = 90.0f;  // Smaller button width
        float buttonHeight = 20.0f; // Smaller button height
        float spacing = 5.0f;
        float totalButtonsWidth = buttonWidth * 2 + spacing;

        ImGui::SameLine(ImGui::GetWindowWidth() - totalButtonsWidth -
                        20); // 20 = padding from right edge

        if (ImGui::Button("Add Folder", ImVec2(buttonWidth, buttonHeight)))
        {
            AddFolder();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete", ImVec2(buttonWidth, buttonHeight)))
        {
            DeleteFolder(m_selectedFile);
        }
        ImGui::Separator();

        // Directory and file list
        if (ImGui::BeginChild("DirectoryList", ImVec2(0, -60)))
        {
            std::string directoryToNavigate; // Store directory to navigate to

            for (const auto &item : m_directoryItems)
            {
                bool isDirectory = !item.empty() && item.back() == '/';
                bool isSelected = (m_selectedFile == item);

                if (isDirectory)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.4f, 0.8f, 1.0f, 1.0f)); // Blue for directories
                }

                if (ImGui::Selectable(item.c_str(), isSelected,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    if (isDirectory)
                    {
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            directoryToNavigate = item; // Store for navigation after loop
                        }
                        m_selectedFile = item;
                    }
                    else
                    {
                        m_selectedFile = item;
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            // Double-click on file - immediate action
                            if (m_isLoadDialog)
                            {
                                std::string fullPath =
                                    (fs::path(m_currentDirectory) / m_selectedFile).string();
                                LoadMap(fullPath);
                                m_mapFileName = fullPath;
                            }
                            else
                            {
                                std::string fullPath =
                                    (fs::path(m_currentDirectory) / m_selectedFile).string();
                                SaveMap(fullPath);
                                m_mapFileName = fullPath;
                            }
                            m_showFileDialog = false;
                        }
                    }
                }

                if (isDirectory)
                {
                    ImGui::PopStyleColor();
                }
            }

            // Navigate after the loop to avoid iterator invalidation
            if (!directoryToNavigate.empty())
            {
                NavigateToDirectory(directoryToNavigate);
            }
        }
        ImGui::EndChild();

        ImGui::Separator();

        // File name input (for save dialog)
        if (!m_isLoadDialog)
        {
            ImGui::Text("File name:");
            if (ImGui::InputText("##FileName", &m_newFileName))
            {
                m_selectedFile = m_newFileName;
            }
            ImGui::Separator();
        }

        bool canProceed = false;
        if (m_isLoadDialog)
        {
            canProceed = !m_selectedFile.empty();
        }
        else
        {
            canProceed = !m_selectedFile.empty() || !m_newFileName.empty();
            if (!canProceed && !m_newFileName.empty())
            {
                m_selectedFile = m_newFileName;
                canProceed = true;
            }
        }

        // Main action buttons with smaller size
        ImVec2 mainButtonSize(80.0f, 25.0f);

        if (ImGui::Button(m_isLoadDialog ? "Load" : "Save", mainButtonSize) && canProceed)
        {
            std::string fullPath = (fs::path(m_currentDirectory) / m_selectedFile).string();

            if (m_isLoadDialog)
            {
                LoadMap(fullPath);
                m_mapFileName = fullPath;
            }
            else
            {
                SaveMap(fullPath);
                m_mapFileName = fullPath;
            }
            m_showFileDialog = false;
        }

        // Handle export for game (when not in load dialog mode)
        if (!m_isLoadDialog && ImGui::Button("Export for Game", mainButtonSize) && canProceed)
        {
            std::string fullPath = (fs::path(m_currentDirectory) / m_selectedFile).string();
            ExportMapForGame(fullPath);
            m_showFileDialog = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", mainButtonSize))
        {
            m_showFileDialog = false;
        }

        // Show selected file
        if (!m_selectedFile.empty())
        {
            // ImGui::SameLine();
            ImGui::Text("Selected: %s", m_selectedFile.c_str());
        }
    }
    ImGui::End();

    if (!m_showFileDialog)
    {
        m_selectedFile.clear();
    }
}

void Editor::AddFolder()
{
    m_showNewFolderDialog = true;
    if (m_newFolderName.empty())
    {
        m_newFolderName = "New Folder"; // Initialize default name
    }
}

void Editor::RenderNewFolderDialog()
{
    if (m_showNewFolderDialog)
    {
        // Center the popup
        ImGui::SetNextWindowPos(
            ImVec2(GetScreenWidth() * 0.5f - 150, GetScreenHeight() * 0.5f - 50));
        ImGui::SetNextWindowSize(ImVec2(300, 120));

        if (ImGui::Begin("Create Folder", &m_showNewFolderDialog,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Enter folder name:");

            // Use imgui_stdlib.h for std::string support
            ImGui::InputText("##FolderName", &m_newFolderName);

            if (ImGui::Button("Create", ImVec2(70, 25)) && !m_newFolderName.empty())
            {
                try
                {
                    fs::path newFolderPath = fs::path(m_currentDirectory) / m_newFolderName;

                    if (fs::create_directory(newFolderPath))
                    {
                        TraceLog(LOG_INFO, "Created folder: %s", newFolderPath.string().c_str());
                        RefreshDirectoryItems();
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Folder already exists or failed to create: %s",
                                 m_newFolderName.c_str());
                    }
                }
                catch (const fs::filesystem_error &e)
                {
                    TraceLog(LOG_ERROR, "Failed to create folder: %s", e.what());
                }

                m_showNewFolderDialog = false;
                m_newFolderName.clear();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(70, 25)))
            {
                m_showNewFolderDialog = false;
                m_newFolderName.clear();
            }
        }
        ImGui::End();
    }
}

void Editor::RenderParkourMapDialog()
{
    if (m_showParkourDialog)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() * 0.5f - 250, GetScreenHeight() * 0.5f - 200),
                               ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Parkour Maps", &m_showParkourDialog, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Select a Parkour Map:");
            ImGui::Separator();

            // List all available parkour maps
            for (int i = 0; i < m_parkourMaps.size(); i++)
            {
                const auto& parkourMap = m_parkourMaps[i];

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s (%.1f/5.0)", parkourMap.displayName.c_str(), parkourMap.difficulty);
                if (ImGui::Selectable(buffer, m_selectedParkourMap == i))
                {
                    m_selectedParkourMap = i;
                }

                // Show tooltip with description on hover
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", parkourMap.description.c_str());
                    ImGui::Text("Elements: %zu", parkourMap.elements.size());
                    ImGui::EndTooltip();
                }
            }

            ImGui::Separator();

            // Action buttons
            if (ImGui::Button("Load Selected Map", ImVec2(150, 30)))
            {
                if (m_selectedParkourMap >= 0 && m_selectedParkourMap < m_parkourMaps.size())
                {
                    LoadParkourMap(m_parkourMaps[m_selectedParkourMap].name);
                    m_showParkourDialog = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 30)))
            {
                m_showParkourDialog = false;
            }

            // Show selected map details
            if (m_selectedParkourMap >= 0 && m_selectedParkourMap < m_parkourMaps.size())
            {
                const auto& selectedParkourMap = m_parkourMaps[m_selectedParkourMap];
                ImGui::Separator();
                ImGui::Text("Selected Map Details:");
                ImGui::Text("Name: %s", selectedParkourMap.displayName.c_str());
                ImGui::Text("Description: %s", selectedParkourMap.description.c_str());
                ImGui::Text("Difficulty: %.1f/5.0", selectedParkourMap.difficulty);
                ImGui::Text("Elements: %zu", selectedParkourMap.elements.size());
                ImGui::Text("Start: (%.1f, %.1f, %.1f)", selectedParkourMap.startPosition.x,
                          selectedParkourMap.startPosition.y, selectedParkourMap.startPosition.z);
                ImGui::Text("End: (%.1f, %.1f, %.1f)", selectedParkourMap.endPosition.x,
                          selectedParkourMap.endPosition.y, selectedParkourMap.endPosition.z);
            }
        }
        ImGui::End();
    }
}

void Editor::LoadParkourMap(const std::string& mapName)
{
    // Get the parkour map by name
    ParkourTestMap parkourMap = ParkourMapGenerator::GetMapByName(mapName);

    // Clear current scene
    m_objects.clear();
    m_selectedObjectIndex = -1;

    // Convert parkour elements to MapObjects
    for (const auto& element : parkourMap.elements)
    {
        MapObject obj;

        // Set basic properties
        obj.SetPosition(element.position);
        obj.SetColor(element.color);
        obj.SetName("Parkour_" + std::to_string(m_objects.size()));

        // Convert parkour shape type to MapObject type
        switch (element.type)
        {
            case ParkourShapeType::Cube:
                obj.SetType(0); // Cube
                obj.SetScale(element.size);
                break;
            case ParkourShapeType::Sphere:
                obj.SetType(1); // Sphere
                obj.SetRadiusSphere(element.size.x);
                break;
            case ParkourShapeType::Cylinder:
                obj.SetType(2); // Cylinder
                obj.SetScale(element.size);
                break;
            case ParkourShapeType::Plane:
                obj.SetType(3); // Plane
                obj.SetSize({element.size.x, element.size.z});
                obj.SetPosition({element.position.x, element.position.y, element.position.z});
                break;
            case ParkourShapeType::Capsule:
                obj.SetType(2); // Use cylinder as approximation
                obj.SetScale(element.size);
                break;
            case ParkourShapeType::Torus:
                obj.SetType(0); // Use cube as approximation
                obj.SetScale(element.size);
                break;
        }

        m_objects.push_back(obj);
    }

    TraceLog(LOG_INFO, "Loaded parkour map '%s' with %d elements", mapName.c_str(), m_objects.size());
}

void Editor::GenerateParkourMap(const std::string& mapName)
{
    LoadParkourMap(mapName);
}

void Editor::ShowParkourMapSelector()
{
    // Load available parkour maps
    m_parkourMaps = ParkourMapGenerator::GetAllParkourMaps();
    m_selectedParkourMap = 0;
    m_showParkourDialog = true;
}

void Editor::DeleteFolder(const std::string &selectedItem)
{
    // Check if we have something to delete and not already showing dialog
    if (!selectedItem.empty() && !m_showDeleteDialog)
    {
        m_showDeleteDialog = true;
        m_itemToDelete = selectedItem;
    }
}

void Editor::RenderDeleteConfirmDialog()
{
    if (m_showDeleteDialog)
    {
        // Center the popup
        ImGui::SetNextWindowPos(
            ImVec2(GetScreenWidth() * 0.5f - 150, GetScreenHeight() * 0.5f - 75));
        ImGui::SetNextWindowSize(ImVec2(300, 130));

        bool isDirectory = !m_itemToDelete.empty() && m_itemToDelete.back() == '/';
        const char *windowTitle = isDirectory ? "Delete Folder" : "Delete File";

        if (ImGui::Begin(windowTitle, &m_showDeleteDialog,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Are you sure you want to delete:");
            ImGui::TextWrapped("%s", m_itemToDelete.c_str());
            ImGui::Text("This action cannot be undone!");

            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(80, 25)))
            {
                try
                {
                    std::string itemName = m_itemToDelete;
                    if (isDirectory)
                    {
                        // Remove trailing '/' for directories
                        itemName = m_itemToDelete.substr(0, m_itemToDelete.length() - 1);
                    }

                    fs::path itemPath = fs::path(m_currentDirectory) / itemName;

                    bool success = false;
                    if (isDirectory)
                    {
                        success = fs::remove_all(itemPath) > 0;
                        TraceLog(LOG_INFO, "Deleted folder: %s", itemPath.string().c_str());
                    }
                    else
                    {
                        success = fs::remove(itemPath);
                        TraceLog(LOG_INFO, "Deleted file: %s", itemPath.string().c_str());
                    }

                    if (success)
                    {
                        RefreshDirectoryItems();
                        m_selectedFile.clear();
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Failed to delete %s: %s",
                                 isDirectory ? "folder" : "file", itemName.c_str());
                    }
                }
                catch (const fs::filesystem_error &e)
                {
                    TraceLog(LOG_ERROR, "Failed to delete: %s", e.what());
                }

                m_showDeleteDialog = false;
                m_itemToDelete.clear();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80, 25)))
            {
                m_showDeleteDialog = false;
                m_itemToDelete.clear();
            }
        }
        ImGui::End();
    }
}
