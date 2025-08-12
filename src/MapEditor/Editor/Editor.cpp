//
// Created by I#Oleg
//

#include "Editor.h"
#include "../MapFileManager/MapFileManager.h"
#include "raylib.h"
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <misc/cpp/imgui_stdlib.h>
#include <rlImGui.h>

#include <raymath.h>
#include <string>

Editor::Editor()
    : m_cameraController(std::make_shared<CameraController>()), m_selectedObjectIndex(-1),
      m_currentTool(SELECT), m_showImGui(true), m_showObjectPanel(true),
      m_showPropertiesPanel(true), m_shouldAddObject(false), m_modelsInitialized(false)
{
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
            // Add to scene

            m_objects.push_back(obj);
        }

        std::cout << "Map loaded successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to load map!" << std::endl;
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
        // Object management buttons
        if (ImGui::Button("Add Object"))
        {
            MapObject newObj;
            newObj.SetName("New Object " + std::to_string(m_objects.size()));
            AddObject(newObj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Selected") && m_selectedObjectIndex >= 0)
        {
            RemoveObject(m_selectedObjectIndex);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete objects"))
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
            if (ImGui::Button(toolNames[i]))
            {
                m_currentTool = static_cast<Tool>(i);

                if (m_currentTool == ADD_CUBE || m_currentTool == ADD_SPHERE ||
                    m_currentTool == ADD_CYLINDER || m_currentTool == ADD_MODEL)
                {
                    m_shouldAddObject = true;
                }
            }

            if (i < std::size(toolNames) - 1)
                ImGui::SameLine();
        }

        if (ImGui::Button("Save Map"))
        {
            SaveMap(m_mapFileName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Map"))
        {
            LoadMap(m_mapFileName);
        }

        ImGui::InputText("Map FilePath##Map Name", &m_mapFileName);

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
        strncpy_s(buffer, nameLabel.c_str(), sizeof(buffer));
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
    // Handle keyboard shortcuts
    if (IsKeyPressed(KEY_DELETE) && m_selectedObjectIndex >= 0)
    {
        RemoveObject(m_selectedObjectIndex);
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        ClearSelection();
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
        try
        {
            TraceLog(LOG_INFO, "Loading models...");
            m_models.LoadModelsFromJson(PROJECT_ROOT_DIR "/src/models.json");
            m_availableModels = m_models.GetAvailableModels();
            m_modelsInitialized = true;
            TraceLog(LOG_INFO, "Models loaded successfully!");
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Failed to load models: %s", e.what());
            // Keep m_modelsInitialized as false so we can try again later
        }
    }
}

Model *Editor::GetModelSafe(const std::string &modelName)
{
    if (!m_modelsInitialized || modelName.empty())
    {
        return nullptr;
    }

    try
    {
        return &m_models.GetModelByName(modelName);
    }
    catch (...)
    {
        // Model not found, return nullptr instead of throwing
        return nullptr;
    }
}
