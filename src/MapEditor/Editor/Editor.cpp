//
// Created by I#Oleg
//

#include "Editor.h"
#include "../MapFileManager/MapFileManager.h"
#include <fstream>
#include <iostream>
#include <imgui.h>
#include <rlImGui.h>

Editor::Editor() 
    : m_cameraController(std::make_shared<CameraController>()),
      m_selectedObjectIndex(-1),
      m_currentTool(SELECT),
      m_showImGui(true),
      m_showObjectPanel(true),
      m_showPropertiesPanel(true),
      m_lockCamera(false)
{
    // ImGui will be initialized in Application::Init() after window creation
}

Editor::~Editor() {
    // Don't call rlImGuiShutdown here - it's handled by Application
}

std::shared_ptr<CameraController> Editor::GetCameraController() const {
    return m_cameraController;
}

void Editor::Update() {
    // Handle user input and update camera
    HandleInput();
    
    // Always update camera, but with reduced sensitivity when ImGui is active
    const ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse || !io.WantCaptureKeyboard || m_lockCamera) {
        // Camera still works but with reduced sensitivity when ImGui is active
        m_cameraController->Update();
    }
}

void Editor::Render() {
    // Render all objects in the scene
    for (const auto& obj : m_objects) {
        RenderObject(obj);
    }
}

void Editor::RenderImGui() {
    // Use simple rlImGui approach
    rlImGuiBegin();
    
    // Render all ImGui panels in specific order
    RenderImGuiToolbar();

    if (m_showObjectPanel) {
        RenderImGuiObjectPanel();
    }
    
    if (m_selectedObjectIndex >= 0) {
        RenderImGuiPropertiesPanel();
    }
    
    rlImGuiEnd();
}

void Editor::HandleInput() {
    // Get ImGui IO for input handling
    const ImGuiIO& io = ImGui::GetIO();
    
    // Handle mouse input for object selection only when ImGui is not capturing
    if (!io.WantCaptureMouse) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (m_currentTool == SELECT) {
                // TODO: Implement object selection via ray casting
            }
        }
        
        // Handle camera input when ImGui is not capturing mouse
        m_cameraController->Update();
    }
    
    // Handle keyboard input only when ImGui is not capturing
    if (!io.WantCaptureKeyboard) {
        HandleKeyboardInput();
    }
}

void Editor::AddObject(const MapObject& obj) {
    // Add new object to the scene
    m_objects.push_back(obj);
}

void Editor::RemoveObject(const int index) {
    // Remove object by index and update selection
    if (index >= 0 && index < m_objects.size()) {
        m_objects.erase(m_objects.begin() + index);
        if (m_selectedObjectIndex == index) {
            m_selectedObjectIndex = -1;
        } else if (m_selectedObjectIndex > index) {
            m_selectedObjectIndex--;
        }
    }
}

void Editor::SelectObject(const int index) {
    // Clear previous selection
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < m_objects.size()) {
        m_objects[m_selectedObjectIndex].selected = false;
    }
    
    m_selectedObjectIndex = index;
    
    // Set new selection
    if (index >= 0 && index < m_objects.size()) {
        m_objects[index].selected = true;
    }
}

void Editor::ClearSelection() {
    // Clear current object selection
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < m_objects.size()) {
        m_objects[m_selectedObjectIndex].selected = false;
    }
    m_selectedObjectIndex = -1;
}

void Editor::SaveMap(const std::string& filename) const {
    // Convert MapObjects to SerializableObjects for saving
    std::vector<SerializableObject> serializableObjects;
    
    for (const auto& obj : m_objects) {
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
    if (MapFileManager::SaveMap(serializableObjects, filename)) {
        std::cout << "Map saved successfully!" << std::endl;
    } else {
        std::cout << "Failed to save map!" << std::endl;
    }
}

void Editor::LoadMap(const std::string& filename) {
    // Load map from file
    std::vector<SerializableObject> serializableObjects;
    
    if (MapFileManager::LoadMap(serializableObjects, filename)) {
        // Clear current scene
        m_objects.clear();
        m_selectedObjectIndex = -1;
        
        // Convert SerializableObjects back to MapObjects
        for (const auto&[position, scale, rotation, color, name, type] : serializableObjects) {
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
    } else {
        std::cout << "Failed to load map!" << std::endl;
    }
}

void Editor::RenderObject(const MapObject& obj) {
    // Choose color based on selection state
    Color drawColor = obj.selected ? YELLOW : obj.color;
    
    // Render object based on its type
    switch (obj.type) {
        case 0: // Cube
            DrawCube(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, drawColor);
            if (obj.selected) {
                DrawCubeWires(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, WHITE);
            }
            break;
        case 1: // Sphere
            DrawSphere(obj.position, obj.scale.x, drawColor);
            if (obj.selected) {
                DrawSphereWires(obj.position, obj.scale.x, 5 , 5 ,WHITE);
            }
            break;
        case 2: // Cylinder
            DrawCylinder(obj.position, obj.scale.x, obj.scale.x, obj.scale.y, 8, drawColor);
            if (obj.selected) {
                DrawCylinderWires(obj.position, obj.scale.x, obj.scale.x, obj.scale.y, 8, WHITE);
            }
            break;
        default: ;
    }
}

void Editor::RenderImGuiToolbar() {
    // Ensure window stays within screen bounds
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_FirstUseEver);
    
    // Use minimal flags for testing
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;
    
    bool toolbarOpen = true;
    if (ImGui::Begin("Toolbar", &toolbarOpen, windowFlags)) {
        // Use larger font for toolbar title
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::Text("Map Editor Tools");
        ImGui::PopFont();
        
        ImGui::Separator();
        
        // Tool buttons
        const char* tools[] = {"Select", "Move", "Rotate", "Scale", "Add Cube", "Add Sphere", "Add Cylinder"};
        
        for (int i = 0; i < 7; i++) {
            if (ImGui::Button(tools[i])) {
                m_currentTool = i;
            }
            if (i < 6) ImGui::SameLine();
        }
        
        ImGui::Separator();
        
        // File operations
        if (ImGui::Button("Save Map")) {
            SaveMap("map.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Map")) {
            LoadMap("map.json");
        }
        
        ImGui::Separator();
        
        // UI toggle options
        ImGui::Checkbox("Show Object Panel", &m_showObjectPanel);
        ImGui::SameLine();
        ImGui::Checkbox("Show Properties", &m_showPropertiesPanel);
        ImGui::SameLine();
        if(ImGui::Button("Lock camera"))
        {
           m_lockCamera != m_lockCamera;     
        }
    }
    
    // If window was closed, don't show it next frame
    if (!toolbarOpen) {
        m_showImGui = false;
    }
    
    ImGui::End();
}

void Editor::RenderImGuiObjectPanel() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    ImGui::SetNextWindowPos(ImVec2(screenWidth - 250, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(240, 400), ImGuiCond_FirstUseEver);
    
    // Use minimal flags for testing
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;
    
    bool objectPanelOpen = true;
    if (ImGui::Begin("Objects", &objectPanelOpen, windowFlags)) {
        // Object management buttons
        if (ImGui::Button("Add Object")) {
            MapObject newObj;
            newObj.name = "New Object " + std::to_string(m_objects.size());
            AddObject(newObj);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Selected") && m_selectedObjectIndex >= 0) {
            RemoveObject(m_selectedObjectIndex);
        }
        
        ImGui::Separator();
        
        // List all objects
        for (int i = 0; i < m_objects.size(); i++) {
            const auto& obj = m_objects[i];
            bool isSelected = (i == m_selectedObjectIndex);
            
            if (ImGui::Selectable(obj.name.c_str(), isSelected)) {
                SelectObject(i);
            }
            
            // Show object info on hover
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Position: %.1f, %.1f, %.1f", obj.position.x, obj.position.y, obj.position.z);
                ImGui::Text("Type: %s", obj.type == 0 ? "Cube" : obj.type == 1 ? "Sphere" : "Cylinder");
                ImGui::EndTooltip();
            }
        }
    }
    
    // If window was closed, don't show it next frame
    if (!objectPanelOpen) {
        m_showObjectPanel = false;
    }
    
    ImGui::End();
}

void Editor::RenderImGuiPropertiesPanel() {
    // Check if we have a selected object
    if (m_selectedObjectIndex < 0 || m_selectedObjectIndex >= m_objects.size()) return;
    
    MapObject& obj = m_objects[m_selectedObjectIndex];
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    ImGui::SetNextWindowPos(ImVec2(10, screenHeight - 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    // Use minimal flags for testing
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;
    
    bool propertiesPanelOpen = true;
    if (ImGui::Begin("Properties", &propertiesPanelOpen, windowFlags)) {
        // Object name editing
        char nameBuffer[256];
        strcpy_s(nameBuffer, sizeof(nameBuffer), obj.name.c_str());
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            obj.name = std::string(nameBuffer);
        }
        
        // Object type selection
        const char* types[] = {"Cube", "Sphere", "Cylinder"};
        if (ImGui::Combo("Type", &obj.type, types, 3)) {
            // Update object type
        }
        
        // Position editing
        float pos[3] = {obj.position.x, obj.position.y, obj.position.z};
        if (ImGui::DragFloat3("Position", pos, 0.1f)) {
            obj.position = {pos[0], pos[1], pos[2]};
        }
        
        // Scale editing
        float scale[3] = {obj.scale.x, obj.scale.y, obj.scale.z};
        if (ImGui::DragFloat3("Scale", scale, 0.1f)) {
            obj.scale = {scale[0], scale[1], scale[2]};
        }
        
        // Rotation editing
        float rot[3] = {obj.rotation.x, obj.rotation.y, obj.rotation.z};
        if (ImGui::DragFloat3("Rotation", rot, 1.0f)) {
            obj.rotation = {rot[0], rot[1], rot[2]};
        }
        
        // Color editing
        float color[4] = {
            obj.color.r / 255.0f,
            obj.color.g / 255.0f,
            obj.color.b / 255.0f,
            obj.color.a / 255.0f
        };
        if (ImGui::ColorEdit4("Color", color)) {
            obj.color = {
                static_cast<unsigned char>(color[0] * 255),
                static_cast<unsigned char>(color[1] * 255),
                static_cast<unsigned char>(color[2] * 255),
                static_cast<unsigned char>(color[3] * 255)
            };
        }
    }
    
    // If window was closed, don't show it next frame
    if (!propertiesPanelOpen) {
        m_showPropertiesPanel = false;
    }
    
    ImGui::End();
}

void Editor::HandleKeyboardInput() {
    // Handle keyboard shortcuts
    if (IsKeyPressed(KEY_DELETE) && m_selectedObjectIndex >= 0) {
        RemoveObject(m_selectedObjectIndex);
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        ClearSelection();
    }
    
    // Toggle UI panels with different keys
    if (IsKeyPressed(KEY_TWO)) {
        m_showObjectPanel = !m_showObjectPanel;
    }
    
    if (IsKeyPressed(KEY_F)) {
        m_showPropertiesPanel = !m_showPropertiesPanel;
    }
}
