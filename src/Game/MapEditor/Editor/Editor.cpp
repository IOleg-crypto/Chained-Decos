//
//

#include "Editor.h"
#include "../MapFileManager/MapFileManager.h"
#include "../MapFileManager/JsonMapFileManager.h"
#include "../../Game/Map/MapLoader.h"  // Include the new comprehensive map loader
#include <cstdlib>
#include <ctime>
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
#include <nfd.h>


#include <raymath.h>
#include <string>

namespace fs = std::filesystem;

Editor::Editor()
    : m_cameraController(std::make_shared<CameraController>()), m_currentlySelectedObjectIndex(-1),
      m_activeEditorTool(SELECT), m_displayImGuiInterface(true), m_displayObjectListPanel(true),
      m_displayPropertiesPanel(true), m_pendingObjectCreation(false), m_modelsInitialized(false),
      m_displayFileDialog(false), m_isFileLoadDialog(true), m_isJsonExportDialog(false), m_displayNewFolderDialog(false),
      m_displayDeleteConfirmationDialog(false), m_displayParkourMapDialog(false), m_currentlySelectedParkourMapIndex(0) , m_gridSizes(50)
{
    // Initialize file dialog to project root
    m_currentWorkingDirectory = PROJECT_ROOT_DIR;
    m_newFileNameInput = "new_map.json";
    RefreshDirectoryItems();
    // NFD init
    NFD_Init();
}

Editor::~Editor() {
    NFD_Quit();
};

std::shared_ptr<CameraController> Editor::GetCameraController() const { return m_cameraController; }

void Editor::Update()
{
    // Handle user input and update camera
    HandleInput();
}

void Editor::Render()
{
    // Render all objects in the scene
    for (auto &obj : m_editorSceneObjects)
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

    if (m_displayObjectListPanel)
    {
        RenderImGuiObjectPanel();
    }

    if (m_currentlySelectedObjectIndex >= 0)
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
            if (m_activeEditorTool == SELECT)
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
    m_editorSceneObjects.push_back(obj);
}

void Editor::RemoveObject(const int index)
{
    // Remove object by index and update selection
    if (index >= 0 && index < m_editorSceneObjects.size())
    {
        m_editorSceneObjects.erase(m_editorSceneObjects.begin() + index);
        if (m_currentlySelectedObjectIndex == index)
        {
            m_currentlySelectedObjectIndex = -1;
        }
        else if (m_currentlySelectedObjectIndex > index)
        {
            m_currentlySelectedObjectIndex--;
        }
    }
}

void Editor::SelectObject(const int index)
{
    // Clear previous selection
    if (m_currentlySelectedObjectIndex >= 0 && m_currentlySelectedObjectIndex < m_editorSceneObjects.size())
    {
        m_editorSceneObjects[m_currentlySelectedObjectIndex].SetSelected(false);
    }

    m_currentlySelectedObjectIndex = index;

    // Set new selection
    if (index >= 0 && index < m_editorSceneObjects.size())
    {
        m_editorSceneObjects[index].SetSelected(true);
    }
}

void Editor::ClearSelection()
{
    // Clear current object selection
    if (m_currentlySelectedObjectIndex >= 0 && m_currentlySelectedObjectIndex < m_editorSceneObjects.size())
    {
        m_editorSceneObjects[m_currentlySelectedObjectIndex].SetSelected(false);
    }
    m_currentlySelectedObjectIndex = -1;
}

void Editor::SaveMap(const std::string &filename)
{
    // Convert MapObjects to SerializableObjects for saving
    std::vector<SerializableObject> serializableObjects;

    for (auto &obj : m_editorSceneObjects)
    {
        SerializableObject serializableObj;
        serializableObj.position = obj.GetPosition();
        serializableObj.scale = obj.GetScale();
        serializableObj.rotation = obj.GetRotation();
        serializableObj.color = obj.GetColor();
        serializableObj.name = obj.GetObjectName();
        serializableObj.type = obj.GetObjectType();
        serializableObj.modelName = obj.GetModelAssetName();
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
        m_editorSceneObjects.clear();
        m_currentlySelectedObjectIndex = -1;

        // Convert SerializableObjects back to MapObjects
        for (const auto &[position, scale, rotation, color, name, type, modelName] :
             serializableObjects)
        {
            MapObject obj;
            obj.SetPosition(position);
            obj.SetScale(scale);
            obj.SetRotation(rotation);
            obj.SetColor(color);
            obj.SetObjectName(name);
            obj.SetObjectType(type);
            obj.SetModelAssetName(modelName);
            obj.SetSelected(false);

            m_editorSceneObjects.push_back(obj);
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
    // Convert MapObjects to JsonSerializableObjects for models.json export
    std::vector<JsonSerializableObject> jsonObjects;

    for (const auto& obj : m_editorSceneObjects)
    {
        JsonSerializableObject jsonObj;

        jsonObj.position = obj.GetPosition();
        jsonObj.scale = obj.GetScale();
        jsonObj.rotation = obj.GetRotation();
        jsonObj.color = obj.GetColor();
        jsonObj.name = obj.GetObjectName();
        jsonObj.type = obj.GetObjectType();
        jsonObj.modelName = obj.GetModelAssetName();
        jsonObj.visible = true;
        jsonObj.layer = "default";
        jsonObj.tags = "exported";
        jsonObj.id = "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

        // Set shape-specific properties for non-model objects
        switch (obj.GetObjectType())
        {
            case 1: // Sphere
                jsonObj.radiusSphere = obj.GetSphereRadius();
                break;
            case 2: // Cylinder
                jsonObj.radiusH = obj.GetScale().x;
                jsonObj.radiusV = obj.GetScale().y;
                break;
            case 3: // Plane
                jsonObj.size = obj.GetPlaneSize();
                break;
        }

        jsonObjects.push_back(jsonObj);
    }

    // Create metadata for models.json format
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name = m_currentlyLoadedMapFilePath.empty() ? "exported_map" : m_currentlyLoadedMapFilePath;
    metadata.displayName = "Exported Map";
    metadata.description = "Map exported from ChainedDecos Map Editor";
    metadata.author = "Map Editor";
    metadata.startPosition = {0.0f, 2.0f, 0.0f};
    metadata.endPosition = {0.0f, 2.0f, 0.0f};
    metadata.skyColor = SKYBLUE;
    metadata.groundColor = DARKGREEN;
    metadata.difficulty = 1.0f;
    metadata.createdDate = "2024-01-01T00:00:00Z";
    metadata.modifiedDate = "2024-01-01T00:00:00Z";
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture = "";

    // Export using the new models.json format
    if (JsonMapFileManager::ExportGameMap(jsonObjects, filename, metadata))
    {
        std::cout << "Map exported for game successfully in models.json format!" << std::endl;
        std::cout << "Saved " << jsonObjects.size() << " objects" << std::endl;
    }
    else
    {
        std::cout << "Failed to export map for game!" << std::endl;
    }
}

void Editor::ExportMapAsJSON(const std::string &filename)
{
    // Convert MapObjects to JsonSerializableObjects for JSON export
    std::vector<JsonSerializableObject> jsonObjects;

    for (const auto& obj : m_editorSceneObjects)
    {
        JsonSerializableObject jsonObj;

        jsonObj.position = obj.GetPosition();
        jsonObj.scale = obj.GetScale();
        jsonObj.rotation = obj.GetRotation();
        jsonObj.color = obj.GetColor();
        jsonObj.name = obj.GetObjectName();
        jsonObj.type = obj.GetObjectType();
        jsonObj.modelName = obj.GetModelAssetName();
        jsonObj.visible = true; // Default to visible
        jsonObj.layer = "default";
        jsonObj.tags = "exported";
        jsonObj.id = "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

        // Set shape-specific properties
        switch (obj.GetObjectType())
        {
            case 1: // Sphere
                jsonObj.radiusSphere = obj.GetSphereRadius();
                break;
            case 2: // Cylinder
                jsonObj.radiusH = obj.GetScale().x;
                jsonObj.radiusV = obj.GetScale().y;
                break;
            case 3: // Plane
                jsonObj.size = obj.GetPlaneSize();
                break;
        }

        jsonObjects.push_back(jsonObj);
    }

    // Create metadata
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name = m_currentlyLoadedMapFilePath.empty() ? "exported_map" : m_currentlyLoadedMapFilePath;
    metadata.displayName = "Exported Map";
    metadata.description = "Map exported from ChainedDecos Map Editor as JSON";
    metadata.author = "Map Editor";
    metadata.startPosition = {0.0f, 2.0f, 0.0f};
    metadata.endPosition = {0.0f, 2.0f, 0.0f};
    metadata.skyColor = SKYBLUE;
    metadata.groundColor = DARKGREEN;
    metadata.difficulty = 1.0f;
    metadata.createdDate = "2024-01-01T00:00:00Z"; // Default timestamp
    metadata.modifiedDate = "2024-01-01T00:00:00Z"; // Default timestamp
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture = "";

    // Export using JsonMapFileManager
    if (JsonMapFileManager::ExportGameMap(jsonObjects, filename, metadata))
    {
        std::cout << "Map exported as JSON successfully!" << std::endl;
        std::cout << "Saved " << jsonObjects.size() << " objects" << std::endl;
    }
    else
    {
        std::cout << "Failed to export map as JSON!" << std::endl;
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
            newObj.SetObjectName("New Object " + std::to_string(m_editorSceneObjects.size()));
            AddObject(newObj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove") && m_currentlySelectedObjectIndex >= 0)
        {
            RemoveObject(m_currentlySelectedObjectIndex);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All"))
        {
            m_editorSceneObjects.clear();
        }

        ImGui::Separator();

        // List all objects
        for (int i = 0; i < m_editorSceneObjects.size(); i++)
        {
            auto &obj = m_editorSceneObjects[i];

            if (const bool isSelected = (i == m_currentlySelectedObjectIndex);
                ImGui::Selectable(obj.GetObjectName().c_str(), isSelected))
            {
                SelectObject(i);
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

void Editor::RenderObject(MapObject &obj)
{
    // Choose color based on selection state
    Color drawColor = obj.IsSelected() ? YELLOW : obj.GetColor();

    // Render object based on its type
    switch (obj.GetObjectType())
    {
    case 0: // Cube
        DrawCube(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z,
                 drawColor);
        if (obj.IsSelected())
        {
            DrawCubeWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z,
                          RED);
        }
        break;
    case 1: // Sphere
        DrawSphere(obj.GetPosition(), obj.GetScale().x, drawColor);
        if (obj.IsSelected())
        {
            DrawSphereWires(obj.GetPosition(), obj.GetSphereRadius(), 5, 5, RED);
        }
        break;
    case 2: // Cylinder
        DrawCylinder(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().y, 8,
                     drawColor);
        if (obj.IsSelected())
        {
            DrawCylinderWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().x,
                              obj.GetScale().y, 8, RED);
        }
        break;
    case 3:
        DrawPlane(obj.GetPosition(), obj.GetPlaneSize(), drawColor);
        break;
    case 4:
        DrawEllipse(obj.GetPosition().x, obj.GetPosition().y, obj.GetHorizontalRadius(), obj.GetVerticalRadius(),
                    drawColor);
        break;
    case 5: // 3D Model
        if (!obj.GetModelAssetName().empty())
        {
            // Ensure models are loaded before trying to use them
            EnsureModelsLoaded();

            // Get model safely without exceptions
            Model *modelPtr = GetModelSafe(obj.GetModelAssetName());

            if (modelPtr != nullptr)
            {
                // Calculate rotation from Euler angles
                Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};
                float rotationAngle = obj.GetRotation().y * RAD2DEG;

                // Draw the model with transformations
                DrawModelEx(*modelPtr, obj.GetPosition(), rotationAxis, rotationAngle,
                            obj.GetScale(), drawColor);

                // Draw wireframe if selected
                if (obj.IsSelected())
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
                if (obj.IsSelected())
                {
                    DrawCubeWires(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y,
                                  obj.GetScale().z, RED);
                }

                // Draw text label to show it's a missing model
                DrawText(("Missing: " + obj.GetModelAssetName()).c_str(), obj.GetPosition().x - 50,
                         obj.GetPosition().y + obj.GetScale().y + 10, 20, RED);
            }
        }
        else
        {
            // No model name specified
            DrawCube(obj.GetPosition(), obj.GetScale().x, obj.GetScale().y, obj.GetScale().z, GRAY);
            if (obj.IsSelected())
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
            bool isCurrentTool = (m_activeEditorTool == i);

            // Highlight current tool
            if (isCurrentTool)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.9f, 1.0f));
            }

            if (ImGui::Button(toolNames[i]))
            {
                m_activeEditorTool = static_cast<Tool>(i);

                if (m_activeEditorTool == ADD_CUBE || m_activeEditorTool == ADD_SPHERE ||
                    m_activeEditorTool == ADD_CYLINDER || m_activeEditorTool == ADD_MODEL)
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
            OpenFileDialog(false); // false for Save dialog
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Map..."))
        {
            OpenFileDialog(true); // true for Load dialog
        }
        ImGui::SameLine();
        if (ImGui::Button("Quick Save") && !m_currentlyLoadedMapFilePath.empty())
        {
            SaveMap(m_currentlyLoadedMapFilePath);
        }
        ImGui::SameLine();
        if (ImGui::Button("Export for Game"))
        {
            // Open save dialog for game export
            m_isFileLoadDialog = false;
            m_displayFileDialog = true;
            m_currentlySelectedFile.clear();
            m_newFileNameInput = "game_map.json";
            RefreshDirectoryItems();
        }
        ImGui::SameLine();
        if (ImGui::Button("Export as JSON"))
        {
            // Open save dialog for JSON export
            m_isFileLoadDialog = false;
            m_isJsonExportDialog = true;
            m_displayFileDialog = true;
            m_currentlySelectedFile.clear();
            m_newFileNameInput = "exported_map.json";
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
        ImGui::Text("Current: %s", m_currentlyLoadedMapFilePath.c_str());

        // Model selection dropdown (only show when adding models)
        if (m_activeEditorTool == ADD_MODEL)
        {
            // Ensure models are loaded for toolbar
            EnsureModelsLoaded();

            ImGui::Text("Select Model:");
            if (ImGui::BeginCombo("##ModelSelect", m_currentlySelectedModelName.c_str()))
            {
                for (const auto &modelName : m_availableModelNamesList)
                {
                    bool isSelected = (m_currentlySelectedModelName == modelName);

                    // Find model info to get category and description
                    std::string displayName = modelName;
                    std::string tooltip = "";

                    // Look up model info if available
                    for (const auto& modelInfo : m_availableModels)
                    {
                        if (modelInfo.name == modelName)
                        {
                            displayName = modelInfo.category + " (" + modelName + ")";
                            tooltip = modelInfo.description + " [" + modelInfo.extension + "]";
                            break;
                        }
                    }

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        m_currentlySelectedModelName = modelName;
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    // Show tooltip with description on hover
                    if (ImGui::IsItemHovered() && !tooltip.empty())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", tooltip.c_str());
                        ImGui::EndTooltip();
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
        if(ImGui::SliderInt("Increase/Decrease editor grid" , &m_gridSizes , 50 , 600))
        {
            if(m_gridSizes < 50)
            {
                m_gridSizes = 50;
            }
        }

        // TraceLog(LOG_INFO, TextFormat("Current Tool: %d", m_activeEditorTool));
    }

    if (m_pendingObjectCreation)
    {
        MapObject newObj;
        newObj.SetObjectName("New Object " + std::to_string(m_editorSceneObjects.size()));

        switch (m_activeEditorTool)
        {
        case ADD_CUBE:
            newObj.SetObjectType(0);
            break;
        case ADD_SPHERE:
            newObj.SetObjectType(1);
            break;
        case ADD_CYLINDER:
            newObj.SetObjectType(2);
            break;
        case ADD_MODEL:
            // Ensure models are loaded before adding model objects
            EnsureModelsLoaded();

            newObj.SetObjectType(5);
            newObj.SetModelAssetName(m_currentlySelectedModelName);
            newObj.SetObjectName(m_currentlySelectedModelName + " " + std::to_string(m_editorSceneObjects.size()));
            break;
        default:
            break;
        }

        AddObject(newObj);

        m_pendingObjectCreation = false;
        m_activeEditorTool = SELECT;
    }

    if (!toolbarOpen)
    {
        m_displayImGuiInterface = false;
    }

    ImGui::End();
}

void Editor::PickObject()
{
    const Camera &camera = GetCameraController()->GetCamera();
    const Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);

    int pickedIndex = -1;
    float minDistance = FLT_MAX;

    for (int i = 0; i < m_editorSceneObjects.size(); ++i)
    {
        auto &obj = m_editorSceneObjects[i];
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
        if (obj.IsSelected() && pickedIndex != i)
        {
            obj.SetSelected(false);
        }
    }

    m_currentlySelectedObjectIndex = pickedIndex;

    if (m_currentlySelectedObjectIndex != -1)
    {
        auto &obj = m_editorSceneObjects[m_currentlySelectedObjectIndex];
        obj.SetSelected(true);
        TraceLog(LOG_INFO, "Picked object %d", m_currentlySelectedObjectIndex);
    }
}

void Editor::RenderImGuiPropertiesPanel()
{
    if (m_currentlySelectedObjectIndex < 0 || m_currentlySelectedObjectIndex >= m_editorSceneObjects.size())
        return;

    MapObject &obj = m_editorSceneObjects[m_currentlySelectedObjectIndex];
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
            // Ensure models are loaded for properties panel
            EnsureModelsLoaded();

            // Model selection dropdown
            ImGui::Text("Model:");
            if (ImGui::BeginCombo("##ModelSelect", obj.GetModelAssetName().c_str()))
            {
                for (const auto &modelName : m_availableModelNamesList)
                {
                    bool isSelected = (obj.GetModelAssetName() == modelName);

                    // Find model info to get category and description
                    std::string displayName = modelName;
                    std::string tooltip = "";

                    // Look up model info if available
                    for (const auto& modelInfo : m_availableModels)
                    {
                        if (modelInfo.name == modelName)
                        {
                            displayName = modelInfo.category + " (" + modelName + ")";
                            tooltip = modelInfo.description + " [" + modelInfo.extension + "]";
                            break;
                        }
                    }

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        obj.SetModelAssetName(modelName);
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    // Show tooltip with description on hover
                    if (ImGui::IsItemHovered() && !tooltip.empty())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", tooltip.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }

            // Show model info if available
            for (const auto& modelInfo : m_availableModels)
            {
                if (modelInfo.name == obj.GetModelAssetName())
                {
                    ImGui::Text("Category: %s", modelInfo.category.c_str());
                    ImGui::Text("Description: %s", modelInfo.description.c_str());
                    ImGui::Text("File: %s", modelInfo.extension.c_str());
                    break;
                }
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

void Editor::HandleKeyboardInput()
{
    // Handle keyboard shortcuts for scene objects
    if (!m_displayFileDialog && IsKeyPressed(KEY_DELETE) && m_currentlySelectedObjectIndex >= 0)
    {
        RemoveObject(m_currentlySelectedObjectIndex);
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_displayFileDialog)
        {
            m_displayFileDialog = false;
        }
        else
        {
            ClearSelection();
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

void Editor::EnsureModelsLoaded()
{
    if (!m_modelsInitialized)
    {
        TraceLog(LOG_INFO, "Loading models...");

        bool loadSuccess = false;
        std::vector<ModelInfo> models;
        try
        {
            // Use the new MapLoader to scan for models in the resources directory
            MapLoader mapLoader;
            std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
            models = mapLoader.LoadModelsFromDirectory(resourcesDir);

            if (!models.empty())
            {
                TraceLog(LOG_INFO, "Editor::EnsureModelsLoaded() - Found %d models in resources directory", models.size());

                // Load each model found in the directory
                for (const auto& modelInfo : models)
                {
                    try
                    {
                        // Fix double path issue - modelInfo.path already contains leading slash
                        std::string modelPath = modelInfo.path;
                     
                        TraceLog(LOG_INFO, "Editor::EnsureModelsLoaded() - Loading model: %s from %s",
                                 modelInfo.name.c_str(), modelPath.c_str());

                        // Load the model using the existing model loading system
                        m_modelAssetManager.LoadSingleModel(modelInfo.name, modelPath, true);
                    }
                    catch (const std::exception& modelException)
                    {
                        TraceLog(LOG_WARNING, "Editor::EnsureModelsLoaded() - Failed to load model %s: %s",
                                 modelInfo.name.c_str(), modelException.what());
                    }
                }

                loadSuccess = true;
            }
            else
            {
                TraceLog(LOG_WARNING, "Editor::EnsureModelsLoaded() - No models found in resources directory");
                loadSuccess = false;
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Failed to load models from directory: %s", e.what());
            loadSuccess = false;
        }

        if (loadSuccess)
        {
            try
            {
                m_availableModelNamesList = m_modelAssetManager.GetAvailableModels();
                m_availableModels = models;  // Store detailed model information
                m_modelsInitialized = true;
                TraceLog(LOG_INFO, "Models loaded successfully! Available models: %zu",
                         m_availableModelNamesList.size());
            }
            catch (const std::exception &e)
            {
                TraceLog(LOG_ERROR, "Failed to get available models list: %s", e.what());
                // Fallback to hardcoded list if GetAvailableModels fails
                m_availableModelNamesList = {"arc"};
                m_modelsInitialized = true;
                TraceLog(LOG_WARNING, "Using fallback model list");
            }
        }
        else
        {
            // Complete failure - use fallback
            TraceLog(LOG_WARNING, "Models failed to load, using fallback model list");
            m_availableModelNamesList = {"arc"};
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
    auto it = std::find(m_availableModelNamesList.begin(), m_availableModelNamesList.end(), modelName);
    if (it == m_availableModelNamesList.end())
    {
        TraceLog(LOG_WARNING, "Model '%s' not found in available models list", modelName.c_str());
        return nullptr;
    }

    // Try to get model, but still handle potential exception safely
    try
    {
        return &m_modelAssetManager.GetModelByName(modelName);
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
    m_isFileLoadDialog = isLoad;
    m_displayFileDialog = true;
    m_currentlySelectedFile.clear();

    // Reset new file name for save dialog
    if (!isLoad)
    {
        m_newFileNameInput = "new_map.json";
    }

    RefreshDirectoryItems();
}

void Editor::RefreshDirectoryItems()
{
    m_currentDirectoryContents.clear();

    try
    {
        if (!fs::exists(m_currentWorkingDirectory) || !fs::is_directory(m_currentWorkingDirectory))
        {
            m_currentWorkingDirectory = PROJECT_ROOT_DIR;
        }

        // Add parent directory option (unless we're at root)
        fs::path currentPath(m_currentWorkingDirectory);
        if (currentPath.has_parent_path() && currentPath != currentPath.root_path())
        {
            m_currentDirectoryContents.push_back("../");
        }

        // Add directories first
        for (const auto &entry : fs::directory_iterator(m_currentWorkingDirectory))
        {
            if (entry.is_directory())
            {
                std::string name = entry.path().filename().string();
                if (!name.empty() && name[0] != '.') // Skip hidden directories
                {
                    m_currentDirectoryContents.push_back(name + "/");
                }
            }
        }

        // Add files (only .json files for maps)
        for (const auto &entry : fs::directory_iterator(m_currentWorkingDirectory))
        {
            if (entry.is_regular_file())
            {
                std::string fileName = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                // Show .json files for maps
                if (extension == ".json" || extension == ".map")
                {
                    m_currentDirectoryContents.push_back(fileName);
                }
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        TraceLog(LOG_ERROR, "File system error: %s", e.what());
        m_currentWorkingDirectory = PROJECT_ROOT_DIR;
        // Try again with project root
        try
        {
            for (const auto &entry : fs::directory_iterator(m_currentWorkingDirectory))
            {
                if (entry.is_directory())
                {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && name[0] != '.')
                    {
                        m_currentDirectoryContents.push_back(name + "/");
                    }
                }
                else if (entry.is_regular_file())
                {
                    std::string fileName = entry.path().filename().string();
                    std::string extension = entry.path().extension().string();
                    if (extension == ".json" || extension == ".map")
                    {
                        m_currentDirectoryContents.push_back(fileName);
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
            newPath = fs::path(m_currentWorkingDirectory).parent_path();
        }
        else
        {
            newPath = fs::path(m_currentWorkingDirectory) / path;
        }

        if (fs::exists(newPath) && fs::is_directory(newPath))
        {
            m_currentWorkingDirectory = newPath.string();
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
    nfdu8filteritem_t filters[] = { { "Maps (json format)", "json" } };
    nfdu8char_t* outPath = nullptr;
    nfdresult_t result;

    if (m_isFileLoadDialog)
        result = NFD_OpenDialogU8(&outPath, filters, 1, nullptr);
    else
        result = NFD_SaveDialogU8(&outPath, filters, 1, nullptr , "exported_map");

    if (result == NFD_OKAY && outPath)
    {
        std::string path(outPath);

        if (m_isFileLoadDialog)
            LoadMap(path);
        else
            SaveMap(path);

        m_currentlyLoadedMapFilePath = std::move(path);

        NFD_FreePathU8(outPath); 
    }
    else if (result == NFD_ERROR)
    {
        std::cerr << "NFD error: " << NFD_GetError() << std::endl;
    }


    m_displayFileDialog = false;
    m_isFileLoadDialog = false;
    m_isJsonExportDialog = false;
    m_currentlySelectedFile.clear();
}

int Editor::GetGridSize() const { return m_gridSizes; }

void Editor::RenderParkourMapDialog()
{
    if (m_displayParkourMapDialog)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() * 0.5f - 250, GetScreenHeight() * 0.5f - 200),
                               ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Parkour Maps", &m_displayParkourMapDialog, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Select a Parkour Map:");
            ImGui::Separator();

            // List all available parkour maps
            for (int i = 0; i < m_availableParkourMaps.size(); i++)
            {
                const auto& gameMap = m_availableParkourMaps[i];

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s (%.1f/5.0)", gameMap.metadata.displayName.c_str(), gameMap.metadata.difficulty);
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
                if (m_currentlySelectedParkourMapIndex >= 0 && m_currentlySelectedParkourMapIndex < m_availableParkourMaps.size())
                {
                    LoadParkourMap(m_availableParkourMaps[m_currentlySelectedParkourMapIndex].metadata.name);
                    m_displayParkourMapDialog = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 30)))
            {
                m_displayParkourMapDialog = false;
            }

            // Show selected map details
            if (m_currentlySelectedParkourMapIndex >= 0 && m_currentlySelectedParkourMapIndex < m_availableParkourMaps.size())
            {
                const auto& selectedGameMap = m_availableParkourMaps[m_currentlySelectedParkourMapIndex];
                ImGui::Separator();
                ImGui::Text("Selected Map Details:");
                ImGui::Text("Name: %s", selectedGameMap.metadata.displayName.c_str());
                ImGui::Text("Description: %s", selectedGameMap.metadata.description.c_str());
                ImGui::Text("Difficulty: %.1f/5.0", selectedGameMap.metadata.difficulty);
                ImGui::Text("Elements: %zu", selectedGameMap.objects.size());
                ImGui::Text("Start: (%.1f, %.1f, %.1f)", selectedGameMap.metadata.startPosition.x,
                           selectedGameMap.metadata.startPosition.y, selectedGameMap.metadata.startPosition.z);
                ImGui::Text("End: (%.1f, %.1f, %.1f)", selectedGameMap.metadata.endPosition.x,
                           selectedGameMap.metadata.endPosition.y, selectedGameMap.metadata.endPosition.z);
            }
        }
        ImGui::End();
    }
}

void Editor::LoadParkourMap(const std::string& mapName)
{
    // Load the map from JSON
    MapLoader loader;
    std::string mapPath = "../resources/maps/" + mapName + ".json";
    GameMap gameMap = loader.LoadMap(mapPath);

    // Clear current scene
    m_editorSceneObjects.clear();
    m_currentlySelectedObjectIndex = -1;

    // Convert GameMap objects to MapObjects
    for (const auto& object : gameMap.objects)
    {
        MapObject obj;

        // Set basic properties
        obj.SetPosition(object.position);
        obj.SetColor(object.color);
        obj.SetObjectName(object.name);

        // Convert MapObjectType to MapObject type
        switch (object.type)
        {
            case MapObjectType::CUBE:
                obj.SetObjectType(0); // Cube
                obj.SetScale(object.scale);
                break;
            case MapObjectType::SPHERE:
                obj.SetObjectType(1); // Sphere
                obj.SetSphereRadius(object.radius);
                break;
            case MapObjectType::CYLINDER:
                obj.SetObjectType(2); // Cylinder
                obj.SetScale(object.scale);
                break;
            case MapObjectType::PLANE:
                obj.SetObjectType(3); // Plane
                obj.SetPlaneSize(object.size);
                break;
            case MapObjectType::MODEL:
                obj.SetObjectType(5); // Model
                obj.SetModelAssetName(object.modelName);
                obj.SetScale(object.scale);
                break;
            case MapObjectType::LIGHT:
                obj.SetObjectType(0); // Use cube as approximation
                obj.SetScale(object.scale);
                break;
        }

        m_editorSceneObjects.push_back(obj);
    }

    TraceLog(LOG_INFO, "Loaded parkour map '%s' with %d elements", mapName.c_str(), m_editorSceneObjects.size());
}

void Editor::GenerateParkourMap(const std::string& mapName)
{
    LoadParkourMap(mapName);
}

void Editor::ShowParkourMapSelector()
{
    // Load available parkour maps
    MapLoader loader;
    m_availableParkourMaps = loader.LoadAllMapsFromDirectory("../resources/maps");
    m_currentlySelectedParkourMapIndex = 0;
    m_displayParkourMapDialog = true;
}

