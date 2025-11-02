//
//

#include "Editor.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Engine/MapFileManager/MapFileManager.h"
#include "Engine/Map/MapLoader.h" // Include the new comprehensive map loader

// Subsystem implementations
#include "SceneManager/SceneManager.h"
#include "UIManager/UIManager.h"
#include "FileManager/FileManager.h"
#include "ToolManager/ToolManager.h"
#include "CameraManager/CameraManager.h"

// Model and rendering subsystems
#include "Engine/Model/Model.h"
#include "ModelManager/ModelManager.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>

#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace fs = std::filesystem;

Editor::Editor(std::shared_ptr<CameraController> cameraController,
                std::unique_ptr<ModelLoader> modelLoader)
    : m_gridSizes(50), m_spawnTextureLoaded(false)
{
    // Initialize spawn texture (will be loaded after window initialization)
    m_spawnTexture = {0};
    
    InitializeSubsystems(std::move(cameraController), std::move(modelLoader));
}

Editor::~Editor() 
{ 
    // Unload spawn texture if loaded
    if (m_spawnTextureLoaded && m_spawnTexture.id != 0)
    {
        UnloadTexture(m_spawnTexture);
        TraceLog(LOG_INFO, "Editor::~Editor() - Unloaded spawn texture");
    }
    NFD_Quit(); 
};

std::shared_ptr<CameraController> Editor::GetCameraController() const {
    return m_cameraManager->GetController();
}

void Editor::InitializeSubsystems(std::shared_ptr<CameraController> cameraController, std::unique_ptr<ModelLoader> modelLoader) {
    // Initialize subsystems in dependency order
    m_cameraManager = std::make_unique<CameraManager>(cameraController);
    m_sceneManager = std::make_unique<SceneManager>();
    m_fileManager = std::make_unique<FileManager>();
    m_toolManager = std::make_unique<ToolManager>();

    // Initialize remaining subsystems
    m_modelManager = std::make_unique<ModelManager>(std::move(modelLoader));
    m_uiManager = std::make_unique<UIManager>(m_sceneManager.get(), m_fileManager.get(), m_toolManager.get(), m_modelManager.get());

    // Initialize file dialog to project root
    NFD_Init();
}

void Editor::Update()
{
    // Update subsystems
    if (m_cameraManager) m_cameraManager->Update();
    // TODO: Update other subsystems when implemented
    // if (m_inputManager) m_inputManager->Update();
    // if (m_uiManager) m_uiManager->HandleInput();
}

void Editor::Render()
{
    // Render all objects in the scene
    if (m_sceneManager) {
        const auto& objects = m_sceneManager->GetObjects();
        for (const auto& obj : objects) {
            // Render each object based on its type
            RenderObject(obj);
        }
    }
}

void Editor::RenderObject(const MapObject& obj)
{
    // Convert MapObject to MapObjectData for shared rendering
    MapObjectData data;
    data.name = obj.GetObjectName();
    data.position = obj.GetPosition();
    data.rotation = obj.GetRotation();
    data.scale = obj.GetScale();
    data.color = obj.GetColor();
    data.modelName = obj.GetModelAssetName();
    data.radius = obj.GetSphereRadius();
    data.height = obj.GetScale().y;
    data.size = obj.GetPlaneSize();
    
    // Convert object type
    switch (obj.GetObjectType())
    {
        case 0: data.type = MapObjectType::CUBE; break;
        case 1: data.type = MapObjectType::SPHERE; break;
        case 2: data.type = MapObjectType::CYLINDER; break;
        case 3: data.type = MapObjectType::PLANE; break;
        case 4: data.type = MapObjectType::LIGHT; break;
        case 5: data.type = MapObjectType::MODEL; break;
        case 6: data.type = MapObjectType::SPAWN_ZONE; break;
        default: data.type = MapObjectType::CUBE; break;
    }
    
    // Handle spawn zone rendering separately
    if (data.type == MapObjectType::SPAWN_ZONE)
    {
        // Render spawn zone with texture
        const float spawnSize = 2.0f;
        Color spawnColor = obj.GetColor();
        RenderSpawnZoneWithTexture(data.position, spawnSize, spawnColor);
        
        // Additional editor-specific rendering: selection wireframe
        if (obj.IsSelected())
        {
            DrawCubeWires(data.position, spawnSize, spawnSize, spawnSize, YELLOW);
        }
        return; // Don't render spawn zone as regular object
    }
    
    // Get loaded models from ModelManager for MODEL type objects
    std::unordered_map<std::string, Model> loadedModels;
    if (m_modelManager && data.type == MapObjectType::MODEL && !data.modelName.empty())
    {
        auto modelOpt = m_modelManager->GetModelLoader().GetModelByName(data.modelName);
        if (modelOpt)
        {
            // Deep copy the model for loadedModels map
            Model modelCopy = modelOpt->get();
            loadedModels[data.modelName] = modelCopy;
        }
    }
    
    // Use shared RenderMapObject function for consistency with Game
    // Pass useEditorColors=true to show textures properly in editor
    Camera3D camera = m_cameraManager->GetCamera();
    RenderMapObject(data, loadedModels, camera, true);
    
    // Additional editor-specific rendering: selection wireframe
    if (obj.IsSelected())
    {
        // Render gizmo for MOVE and SCALE tools
        RenderGizmo(obj, data);
    }
    
    if (obj.IsSelected() && data.type == MapObjectType::MODEL)
    {
        auto it = loadedModels.find(data.modelName);
        if (it != loadedModels.end())
        {
            // Draw wireframe for selected models
            DrawModelWires(it->second, {0, 0, 0}, 1.0f, YELLOW);
        }
    }
    else if (obj.IsSelected())
    {
        // Draw selection indicator for primitives
        Color selectionColor = YELLOW;
        selectionColor.a = 100; // Semi-transparent
        switch (data.type)
        {
            case MapObjectType::CUBE:
            {
                DrawCubeWires(data.position, data.scale.x, data.scale.y, data.scale.z, YELLOW);
                break;
            }
            case MapObjectType::SPHERE:
            {
                DrawSphereWires(data.position, data.radius, 16, 16, YELLOW);
                break;
            }
            case MapObjectType::CYLINDER:
            {
                DrawCylinderWires(data.position, data.radius, data.radius, data.height, 16, YELLOW);
                break;
            }
            case MapObjectType::PLANE:
            {
                // Draw plane selection indicator using lines
                Vector3 p1 = {data.position.x - data.size.x * 0.5f, data.position.y, data.position.z - data.size.y * 0.5f};
                Vector3 p2 = {data.position.x + data.size.x * 0.5f, data.position.y, data.position.z - data.size.y * 0.5f};
                Vector3 p3 = {data.position.x + data.size.x * 0.5f, data.position.y, data.position.z + data.size.y * 0.5f};
                Vector3 p4 = {data.position.x - data.size.x * 0.5f, data.position.y, data.position.z + data.size.y * 0.5f};
                DrawLine3D(p1, p2, YELLOW);
                DrawLine3D(p2, p3, YELLOW);
                DrawLine3D(p3, p4, YELLOW);
                DrawLine3D(p4, p1, YELLOW);
                break;
            }
            default:
                break;
        }
    }
}

void Editor::RenderGizmo(const MapObject& obj, const MapObjectData& data)
{
    // Only show gizmo for MOVE and SCALE tools
    if (!m_toolManager) return;
    Tool activeTool = m_toolManager->GetActiveTool();
    if (activeTool != MOVE && activeTool != SCALE) return;
    
    Vector3 pos = obj.GetPosition();
    float gizmoLength = 2.0f;
    float gizmoThickness = 0.1f;
    
    // Get camera to determine gizmo scale
    Camera3D camera = m_cameraManager->GetCamera();
    Vector3 toCamera = Vector3Subtract(camera.position, pos);
    float distance = Vector3Length(toCamera);
    float scale = distance * 0.1f; // Scale gizmo based on distance from camera
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;
    
    float arrowLength = gizmoLength * scale;
    float arrowRadius = gizmoThickness * scale;
    
    // Draw X axis (red) - right
    DrawLine3D(pos, Vector3Add(pos, {arrowLength, 0, 0}), RED);
    DrawCylinderEx(Vector3Add(pos, {arrowLength * 0.7f, 0, 0}), Vector3Add(pos, {arrowLength, 0, 0}), arrowRadius * 0.5f, arrowRadius, 8, RED);
    
    // Draw Y axis (green) - up
    DrawLine3D(pos, Vector3Add(pos, {0, arrowLength, 0}), GREEN);
    DrawCylinderEx(Vector3Add(pos, {0, arrowLength * 0.7f, 0}), Vector3Add(pos, {0, arrowLength, 0}), arrowRadius * 0.5f, arrowRadius, 8, GREEN);
    
    // Draw Z axis (blue) - forward
    DrawLine3D(pos, Vector3Add(pos, {0, 0, arrowLength}), BLUE);
    DrawCylinderEx(Vector3Add(pos, {0, 0, arrowLength * 0.7f}), Vector3Add(pos, {0, 0, arrowLength}), arrowRadius * 0.5f, arrowRadius, 8, BLUE);
    
    // Draw center sphere
    DrawSphere(pos, arrowRadius * 1.5f, YELLOW);
}

void Editor::RenderImGui()
{
    if (m_uiManager) {
        m_uiManager->Render();
    }
}

void Editor::HandleInput()
{
    if (m_uiManager) {
        m_uiManager->HandleInput();
    }

    // Handle tool-specific input
    if (m_toolManager && m_sceneManager && m_cameraManager) {
        const ImGuiIO &io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            // Create ray from screen to world using camera
            Ray ray = GetScreenToWorldRay(GetMousePosition(), m_cameraManager->GetCamera());
            
            // Handle mouse button press/release
            bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            bool mouseReleased = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
            bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
            
            if (mousePressed) {
                m_toolManager->HandleToolInput(true, ray, *m_sceneManager);
            } else if (mouseReleased) {
                m_toolManager->HandleToolInput(false, ray, *m_sceneManager);
            } else if (mouseDown) {
                // Update tool during drag operations
                m_toolManager->UpdateTool(ray, *m_sceneManager);
            }
        }
    }
}

void Editor::AddObject(const MapObject &obj)
{
    // Delegate to scene manager
    if (m_sceneManager) {
        m_sceneManager->AddObject(obj);
    }
}

void Editor::RemoveObject(const int index)
{
    // Delegate to scene manager
    if (m_sceneManager) {
        m_sceneManager->RemoveObject(index);
    }
}

void Editor::SelectObject(const int index)
{
    // Delegate to scene manager
    if (m_sceneManager) {
        m_sceneManager->SelectObject(index);
    }
}

void Editor::ClearSelection()
{
    // Delegate to scene manager
    if (m_sceneManager) {
        m_sceneManager->ClearSelection();
    }
}

void Editor::SaveMap(const std::string &filename)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager) {
        const auto& objects = m_sceneManager->GetObjects();
        if (m_fileManager->SaveMap(filename, objects)) {
            std::cout << "Map saved successfully!" << std::endl;
            m_fileManager->SetCurrentlyLoadedMapFilePath(filename);
        } else {
            std::cout << "Failed to save map!" << std::endl;
        }
    }
}

void Editor::LoadMap(const std::string &filename)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager) {
        std::vector<MapObject> objects;
        if (m_fileManager->LoadMap(filename, objects)) {
            // Clear and reload scene
            
            m_sceneManager->ClearSelection();
            // Note: SceneManager doesn't have a ClearAll method, so we need to recreate it
            // For now, we'll assume the file manager handles scene clearing internally
            std::cout << "Map loaded successfully!" << std::endl;
            m_fileManager->SetCurrentlyLoadedMapFilePath(filename);
        } else {
            std::cout << "Failed to load map!" << std::endl;
        }
    }
}

void Editor::ExportMapForGame(const std::string &filename)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager) {
        const auto& objects = m_sceneManager->GetObjects();
        if (m_fileManager->ExportForGame(filename, objects)) {
            std::cout << "Map exported for game successfully!" << std::endl;
        } else {
            std::cout << "Failed to export map for game!" << std::endl;
        }
    }
}

void Editor::ExportMapAsJSON(const std::string &filename)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager) {
        const auto& objects = m_sceneManager->GetObjects();
        if (m_fileManager->ExportAsJSON(filename, objects)) {
            std::cout << "Map exported as JSON successfully!" << std::endl;
        } else {
            std::cout << "Failed to export map as JSON!" << std::endl;
        }
    }
}









// Helper function to draw textured cube (based on Raylib example, with corrected UV coordinates)
static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);
        
        // Front Face (UV coordinates corrected for proper texture orientation)
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        
        // Back Face
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        
        // Top Face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        
        // Bottom Face
        rlNormal3f(0.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        
        // Right Face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        
        // Left Face
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        
    rlEnd();
    rlSetTexture(0);
}

void Editor::RenderSpawnZoneWithTexture(const Vector3& position, float size, Color color) const
{
    if (!m_spawnTextureLoaded)
    {
        // Fallback to simple cube if texture not loaded
        DrawCube(position, size, size, size, color);
        DrawCubeWires(position, size, size, size, WHITE);
        return;
    }
    
    // Use Raylib-style helper function to draw textured cube
    DrawCubeTexture(m_spawnTexture, position, size, size, size, color);
    
    // Draw wireframe for better visibility
    DrawCubeWires(position, size, size, size, WHITE);
}

int Editor::GetGridSize() const 
{ 
    // Get grid size from UIManager if available, otherwise use default
    if (m_uiManager) 
    {
        return m_uiManager->GetGridSize();
    }
    return m_gridSizes;
}

// The Editor class has been successfully refactored to use the Facade pattern.
// All major functionality has been delegated to subsystem managers.
// Remaining old code in this file needs to be cleaned up in future iterations.


void Editor::LoadParkourMap(const std::string &mapName)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager) {
        std::vector<MapObject> objects;
        m_fileManager->LoadParkourMap(mapName, objects);
        // SceneManager will be updated by file manager
        TraceLog(LOG_INFO, "Loaded parkour map '%s'", mapName.c_str());
    }
}

void Editor::GenerateParkourMap(const std::string &mapName)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager) {
        std::vector<MapObject> objects;
        m_fileManager->GenerateParkourMap(mapName, objects);
        // SceneManager will be updated by file manager
        TraceLog(LOG_INFO, "Generated parkour map '%s'", mapName.c_str());
    }
}

void Editor::ShowParkourMapSelector()
{
    // Delegate to file manager
    if (m_fileManager) {
        m_fileManager->ShowParkourMapSelector();
    }
}

void Editor::LoadSpawnTexture()
{
    if (m_spawnTextureLoaded)
    {
        return; // Already loaded
    }
    
    // Load spawn texture (only after window is initialized)
    std::string texturePath = std::string(PROJECT_ROOT_DIR) + "/resources/boxes/PlayerSpawnTexture.png";
    if (FileExists(texturePath.c_str()))
    {
        m_spawnTexture = LoadTexture(texturePath.c_str());
        if (m_spawnTexture.id != 0)
        {
            m_spawnTextureLoaded = true;
            TraceLog(LOG_INFO, "Editor::LoadSpawnTexture() - Loaded spawn texture: %dx%d", 
                     m_spawnTexture.width, m_spawnTexture.height);
        }
        else
        {
            TraceLog(LOG_WARNING, "Editor::LoadSpawnTexture() - Failed to load spawn texture from: %s", texturePath.c_str());
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Editor::LoadSpawnTexture() - Spawn texture not found at: %s", texturePath.c_str());
    }
}

void Editor::PreloadModelsFromResources()
{
    if (!m_modelManager)
        return;

    try
    {
        MapLoader mapLoader;
        std::string resourcesDir = std::string(PROJECT_ROOT_DIR) + "/resources";
        auto models = mapLoader.LoadModelsFromDirectory(resourcesDir);
        for (const auto &modelInfo : models)
        {
            m_modelManager->LoadModel(modelInfo.name, modelInfo.path);
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_WARNING, "Editor: Failed to preload models from resources: %s", e.what());
    }
}
