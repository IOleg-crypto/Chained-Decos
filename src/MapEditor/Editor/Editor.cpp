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
      m_showPropertiesPanel(true), m_shouldAddObject(false)
{
    // ImGui will be initialized in Application::Init() after window creation
}

Editor::~Editor() = default;

std::shared_ptr<CameraController> Editor::GetCameraController() const { return m_cameraController; }

void Editor::Update()
{
    // Handle user input and update camera
    HandleInput();
}

void Editor::Render() const
{
    // Render all objects in the scene
    for (const auto &obj : m_objects)
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
        m_objects[m_selectedObjectIndex].selected = false;
    }

    m_selectedObjectIndex = index;

    // Set new selection
    if (index >= 0 && index < m_objects.size())
    {
        m_objects[index].selected = true;
    }
}

void Editor::ClearSelection()
{
    // Clear current object selection
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < m_objects.size())
    {
        m_objects[m_selectedObjectIndex].selected = false;
    }
    m_selectedObjectIndex = -1;
}

void Editor::SaveMap(const std::string &filename) const
{
    // Convert MapObjects to SerializableObjects for saving
    std::vector<SerializableObject> serializableObjects;

    for (const auto &obj : m_objects)
    {
        SerializableObject serializableObj;
        serializableObj.position = obj.position;
        serializableObj.scale = obj.scale;
        serializableObj.rotation = obj.rotation;
        serializableObj.color = obj.color;
        serializableObj.name = obj.name;
        serializableObj.type = obj.type;
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
        for (const auto &[position, scale, rotation, color, name, type] : serializableObjects)
        {
            MapObject obj;
            obj.position = position;
            obj.scale = scale;
            obj.rotation = rotation;
            obj.color = color;
            obj.name = name;
            obj.type = type;
            obj.selected = false;
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
            newObj.name = "New Object " + std::to_string(m_objects.size());
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
            const auto &obj = m_objects[i];

            if (const bool isSelected = (i == m_selectedObjectIndex);
                ImGui::Selectable(obj.name.c_str(), isSelected))
            {
                SelectObject(i);
            }

            // Show object info on hover
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Position: %.1f, %.1f, %.1f", obj.position.x, obj.position.y,
                            obj.position.z);
                ImGui::Text("Type: %s", obj.type == 0   ? "Cube"
                                        : obj.type == 1 ? "Sphere"
                                                        : "Cylinder");
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

void Editor::RenderObject(const MapObject &obj)
{
    // Choose color based on selection state
    Color drawColor = obj.selected ? YELLOW : obj.color;

    // Render object based on its type
    switch (obj.type)
    {
    case 0: // Cube
        DrawCube(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, drawColor);
        if (obj.selected)
        {
            DrawCubeWires(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, RED);
        }
        break;
    case 1: // Sphere
        DrawSphere(obj.position, obj.scale.x, drawColor);
        if (obj.selected)
        {
            DrawSphereWires(obj.position, obj.radiusSphere, 5, 5, RED);
        }
        break;
    case 2: // Cylinder
        DrawCylinder(obj.position, obj.scale.x, obj.scale.y, obj.scale.y, 8, drawColor);
        if (obj.selected)
        {
            DrawCylinderWires(obj.position, obj.scale.x, obj.scale.x, obj.scale.y, 8, RED);
        }
        break;
    case 3:
        DrawPlane(obj.position, obj.size, drawColor);
        break;
    case 4:
        DrawEllipse(obj.position.x, obj.position.y, obj.radiusH, obj.radiusV, drawColor);
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

        const char *toolNames[] = {"Select",   "Move",       "Rotate",      "Scale",
                                   "Add Cube", "Add Sphere", "Add Cylinder"};

        for (int i = 0; i < std::size(toolNames); i++)
        {
            if (ImGui::Button(toolNames[i]))
            {
                m_currentTool = static_cast<Tool>(i);

                if (m_currentTool == ADD_CUBE || m_currentTool == ADD_SPHERE ||
                    m_currentTool == ADD_CYLINDER)
                {
                    m_shouldAddObject = true;
                }
            }

            if (i < std::size(toolNames) - 1)
                ImGui::SameLine();
        }

        ImGui::Separator();

        if (ImGui::Button("Save Map"))
        {
            SaveMap("map.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Map"))
        {
            LoadMap("map.json");
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
        newObj.name = "New Object " + std::to_string(m_objects.size());

        switch (m_currentTool)
        {
        case ADD_CUBE:
            newObj.type = 0;
            break;
        case ADD_SPHERE:
            newObj.type = 1;
            break;
        case ADD_CYLINDER:
            newObj.type = 2;
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
        const BoundingBox box = {Vector3{obj.position.x - obj.scale.x, obj.position.y - obj.scale.y,
                                         obj.position.z - obj.scale.z / 2},
                                 Vector3{obj.position.x + obj.scale.x, obj.position.y + obj.scale.y,
                                         obj.position.z + obj.scale.z}};
        if (const RayCollision collision = GetRayCollisionBox(ray, box);
            collision.hit && collision.distance < minDistance)
        {
            minDistance = collision.distance;
            pickedIndex = i;
        }
        // If we have object selected before using properties panel , we cancel coloring other
        // objects , besides object took by mouse(MOUSE_LEFT_BUTTON)
        if (obj.selected == true)
        {
            obj.selected = false;
        }
    }

    m_selectedObjectIndex = pickedIndex;

    if (m_selectedObjectIndex != -1)
    {
        auto &obj = m_objects[m_selectedObjectIndex];
        obj.selected = true;
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

    if (ImGui::Begin("Properties##Panel", &propertiesPanelOpen, windowFlags))
    {
        char nameBuffer[256] = {0};
        strncpy_s(nameBuffer, obj.name.c_str(), sizeof(nameBuffer) - 1);

        std::string inputLabel = "Name##obj" + std::to_string(m_selectedObjectIndex);
        if (ImGui::InputText(inputLabel.c_str(), nameBuffer, sizeof(nameBuffer)))
        {
            obj.name = nameBuffer;
        }

        const char *types[] = {"Cube", "Sphere", "Cylinder", "Plane", "Ellipse"};
        ImGui::Combo("Type", &obj.type, types, IM_ARRAYSIZE(types));

        // Позиція
        float pos[3] = {obj.position.x, obj.position.y, obj.position.z};
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            obj.position = {pos[0], pos[1], pos[2]};
        }

        float scale[3] = {obj.scale.x, obj.scale.y, obj.scale.z};
        float size[2] = {obj.size.x, obj.size.y};
        float radiusEllipse[2] = {obj.radiusH, obj.radiusV};
        float radiusSphere = obj.radiusSphere;

        switch (obj.type)
        {
        case 0: // Cube
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.scale = {scale[0], scale[1], scale[2]};
            }
            break;

        case 1: // Sphere
            if (ImGui::DragFloat("Radius", &radiusSphere, 0.1f))
            {
                obj.radiusSphere = radiusSphere;
            }
            break;

        case 2: // Cylinder
            if (ImGui::DragFloat3("Scale", scale, 0.1f))
            {
                obj.scale = {scale[0], scale[1], scale[2]};
            }
            break;

        case 3: // Plane
            if (ImGui::DragFloat2("Size", size, 0.1f))
            {
                obj.size = {size[0], size[1]};
            }
            break;

        case 4: // Ellipse
            if (ImGui::DragFloat2("Radius H/V", radiusEllipse, 0.1f))
            {
                obj.radiusH = radiusEllipse[0];
                obj.radiusV = radiusEllipse[1];
            }
            break;
        }

        // Ротація
        float rot[3] = {obj.rotation.x, obj.rotation.y, obj.rotation.z};
        if (ImGui::DragFloat3("Rotation", rot, 1.0f))
        {
            obj.rotation = {rot[0], rot[1], rot[2]};
        }

        // Колір
        float color[4] = {obj.color.r / 255.0f, obj.color.g / 255.0f, obj.color.b / 255.0f,
                          obj.color.a / 255.0f};

        if (ImGui::ColorEdit4("Color", color))
        {
            obj.color = {static_cast<unsigned char>(color[0] * 255),
                         static_cast<unsigned char>(color[1] * 255),
                         static_cast<unsigned char>(color[2] * 255),
                         static_cast<unsigned char>(color[3] * 255)};
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
