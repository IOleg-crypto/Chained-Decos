//
//

#include "Editor.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Map/MapLoader.h" // Include the new comprehensive map loader
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "FileManager/MapObjectConverterEditor.h"

// Subsystem implementations
#include "CameraManager/CameraManager.h"
#include "FileManager/FileManager.h"
#include "SceneManager/SceneManager.h"
#include "ToolManager/ToolManager.h"
#include "UIManager/UIManager.h"

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

#include "Engine/Render/RenderUtils.h"
#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace fs = std::filesystem;


Editor::Editor(std::shared_ptr<CameraController> cameraController,
               std::unique_ptr<ModelLoader> modelLoader)
    : m_gridSizes(50), m_spawnTextureLoaded(false), m_skybox(std::make_unique<Skybox>()), m_skyboxTexturePath("")
{
    // Initialize spawn texture (will be loaded after window initialization)
    m_spawnTexture = {0};
    m_clearColor = DARKGRAY;
    m_activeMetadata = MapMetadata();

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
    if (m_skybox)
    {
        m_skybox.reset();
    }
    NFD_Quit();
};

std::shared_ptr<CameraController> Editor::GetCameraController() const
{
    if (!m_cameraManager)
    {
        TraceLog(LOG_WARNING, "Editor::GetCameraController() - CameraManager is null");
        return nullptr;
    }
    return m_cameraManager->GetController();
}

void Editor::InitializeSubsystems(std::shared_ptr<CameraController> cameraController,
                                  std::unique_ptr<ModelLoader> modelLoader)
{
    // Initialize NFD once (before subsystems that use it)
    NFD_Init();

    // Initialize subsystems in dependency order
    m_cameraManager = std::make_unique<CameraManager>(cameraController);
    m_sceneManager = std::make_unique<SceneManager>();
    m_fileManager = std::make_unique<FileManager>();
    m_toolManager = std::make_unique<ToolManager>();

    // Initialize remaining subsystems
    m_modelManager = std::make_unique<ModelManager>(std::move(modelLoader));
    m_uiManager = std::make_unique<UIManager>(this, m_sceneManager.get(), m_fileManager.get(),
                                              m_toolManager.get(), m_modelManager.get());
                                              
}

void Editor::Update()
{
    // Update subsystems
    if (m_cameraManager)
        m_cameraManager->Update();
    // if (m_skybox)
    // {
    //     SkyboxUpdate(m_skybox.get());
    // }
    // TODO: Update other subsystems when implemented
    // if (m_inputManager) m_inputManager->Update();
    // if (m_uiManager) m_uiManager->HandleInput();
    
}

void Editor::Render()
{
    if (m_skybox && m_skybox->IsLoaded())
    {
        m_skybox->DrawSkybox();
    }


    if (m_sceneManager)
    {
        const auto &objects = m_sceneManager->GetObjects();
        for (const auto &obj : objects)
        {
            // Render each object based on its type
            RenderObject(obj);
        }
    }
}

void Editor::RenderObject(const MapObject &obj)
{
    // Convert MapObject to MapObjectData using MapObjectConverterEditor
    MapObjectData data = MapObjectConverterEditor::MapObjectToMapObjectData(obj);

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

    // Use MapLoader::RenderMapObject for consistency with Game
    // Pass useEditorColors=true to show textures properly in editor
    Camera3D camera = m_cameraManager->GetCamera();
    MapLoader loader;
    loader.RenderMapObject(data, loadedModels, camera, true);
    
    // Note: This could also use MapService::RenderMapObject for consistency,
    // but MapLoader is already available and works correctly

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
            Vector3 p1 = {data.position.x - data.size.x * 0.5f, data.position.y,
                          data.position.z - data.size.y * 0.5f};
            Vector3 p2 = {data.position.x + data.size.x * 0.5f, data.position.y,
                          data.position.z - data.size.y * 0.5f};
            Vector3 p3 = {data.position.x + data.size.x * 0.5f, data.position.y,
                          data.position.z + data.size.y * 0.5f};
            Vector3 p4 = {data.position.x - data.size.x * 0.5f, data.position.y,
                          data.position.z + data.size.y * 0.5f};
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

void Editor::RenderGizmo(const MapObject &obj, const MapObjectData &data)
{
    // Only show gizmo for MOVE and SCALE tools
    if (!m_toolManager || !m_cameraManager)
        return;
    Tool activeTool = m_toolManager->GetActiveTool();
    if (activeTool != MOVE && activeTool != SCALE)
        return;

    Vector3 pos = obj.GetPosition();
    float gizmoLength = 2.0f;
    float gizmoThickness = 0.1f;

    // Get camera to determine gizmo scale
    Camera3D camera = m_cameraManager->GetCamera();
    Vector3 toCamera = Vector3Subtract(camera.position, pos);
    float distance = Vector3Length(toCamera);
    float scale = distance * 0.1f; // Scale gizmo based on distance from camera
    if (scale < 0.5f)
        scale = 0.5f;
    if (scale > 2.0f)
        scale = 2.0f;

    float arrowLength = gizmoLength * scale;
    float arrowRadius = gizmoThickness * scale;

    // Draw X axis (red) - right
    DrawLine3D(pos, Vector3Add(pos, {arrowLength, 0, 0}), RED);
    DrawCylinderEx(Vector3Add(pos, {arrowLength * 0.7f, 0, 0}),
                   Vector3Add(pos, {arrowLength, 0, 0}), arrowRadius * 0.5f, arrowRadius, 8, RED);

    // Draw Y axis (green) - up
    DrawLine3D(pos, Vector3Add(pos, {0, arrowLength, 0}), GREEN);
    DrawCylinderEx(Vector3Add(pos, {0, arrowLength * 0.7f, 0}),
                   Vector3Add(pos, {0, arrowLength, 0}), arrowRadius * 0.5f, arrowRadius, 8, GREEN);

    // Draw Z axis (blue) - forward
    DrawLine3D(pos, Vector3Add(pos, {0, 0, arrowLength}), BLUE);
    DrawCylinderEx(Vector3Add(pos, {0, 0, arrowLength * 0.7f}),
                   Vector3Add(pos, {0, 0, arrowLength}), arrowRadius * 0.5f, arrowRadius, 8, BLUE);

    // Draw center sphere
    DrawSphere(pos, arrowRadius * 1.5f, YELLOW);
}

void Editor::RenderImGui()
{
    if (m_uiManager)
    {
        m_uiManager->Render();
    }
}

void Editor::HandleInput()
{
    if (m_uiManager)
    {
        m_uiManager->HandleInput();
    }

    // Handle tool-specific input
    if (m_toolManager && m_sceneManager && m_cameraManager)
    {
        const ImGuiIO &io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
        {
            // Create ray from screen to world using camera
            Ray ray = GetScreenToWorldRay(GetMousePosition(), m_cameraManager->GetCamera());

            // Handle mouse button press/release
            bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            bool mouseReleased = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
            bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            if (mousePressed)
            {
                m_toolManager->HandleToolInput(true, ray, *m_sceneManager);
            }
            else if (mouseReleased)
            {
                m_toolManager->HandleToolInput(false, ray, *m_sceneManager);
            }
            else if (mouseDown)
            {
                // Update tool during drag operations
                m_toolManager->UpdateTool(ray, *m_sceneManager);
            }
        }
    }
}

void Editor::AddObject(const MapObject &obj)
{
    // Delegate to scene manager
    if (m_sceneManager)
    {
        m_sceneManager->AddObject(obj);
    }
}

void Editor::RemoveObject(const int index)
{
    // Delegate to scene manager
    if (m_sceneManager)
    {
        m_sceneManager->RemoveObject(index);
    }
}

void Editor::SelectObject(const int index)
{
    // Delegate to scene manager
    if (m_sceneManager)
    {
        m_sceneManager->SelectObject(index);
    }
}

void Editor::ClearSelection()
{
    // Delegate to scene manager
    if (m_sceneManager)
    {
        m_sceneManager->ClearSelection();
    }
}

void Editor::SaveMap(const std::string &filename)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager)
    {
        const auto &objects = m_sceneManager->GetObjects();
        if (m_fileManager->SaveMap(filename, objects))
        {
            std::cout << "Map saved successfully!" << std::endl;
            m_fileManager->SetCurrentlyLoadedMapFilePath(filename);
        }
        else
        {
            std::cout << "Failed to save map!" << std::endl;
        }
    }
}

void Editor::LoadMap(const std::string &filename)
{
    // Delegate to file manager
    if (m_fileManager && m_sceneManager)
    {
        std::vector<MapObject> objects;
        if (m_fileManager->LoadMap(filename, objects))
        {
            // Clear selection first
            m_sceneManager->ClearSelection();
            
            // Clear existing objects by removing them one by one (from back to front to avoid index issues)
            // Note: This is a workaround until SceneManager has a ClearAll method
            const auto& existingObjects = m_sceneManager->GetObjects();
            for (int i = static_cast<int>(existingObjects.size()) - 1; i >= 0; --i)
            {
                m_sceneManager->RemoveObject(i);
            }
            
            // Add all loaded objects (including all types: CUBE, SPHERE, CYLINDER, PLANE, LIGHT, MODEL, SPAWN_ZONE)
            for (const auto& obj : objects)
            {
                m_sceneManager->AddObject(obj);
            }
            
            std::cout << "Map loaded successfully with " << objects.size() << " objects!" << std::endl;
            m_fileManager->SetCurrentlyLoadedMapFilePath(filename);
            
            // Apply metadata (including skybox, skyColor, startPosition, endPosition, etc.) from loaded map
            const MapMetadata& metadata = m_fileManager->GetCurrentMetadata();
            ApplyMetadata(metadata);
            
            // Ensure skybox is loaded if metadata contains skybox texture
            // ApplyMetadata already calls SetSkyboxTexture, but we ensure it's applied
            if (!metadata.skyboxTexture.empty())
            {
                SetSkyboxTexture(metadata.skyboxTexture, false);
            }
        }
        else
        {
            std::cout << "Failed to load map!" << std::endl;
        }
    }
}

void Editor::RenderSpawnZoneWithTexture(const Vector3 &position, float size, Color color) const
{
    if (!m_spawnTextureLoaded)
    {
        // Fallback to simple cube if texture not loaded
        DrawCube(position, size, size, size, color);
        DrawCubeWires(position, size, size, size, WHITE);
        return;
    }

    // Use shared RenderUtils function to draw textured cube
    RenderUtils::DrawCubeTexture(m_spawnTexture, position, size, size, size, color);

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

void Editor::ApplyMetadata(const MapMetadata &metadata)
{
    m_activeMetadata = metadata;
    m_clearColor = metadata.skyColor;
    SetSkyboxTexture(metadata.skyboxTexture, false);
    
    // Update FileManager metadata to keep them in sync
    if (m_fileManager)
    {
        m_fileManager->SetCurrentMetadata(metadata);
    }
}

void Editor::SetSkyboxTexture(const std::string &texturePath, bool updateFileManager)
{
    std::string normalized = NormalizeSkyboxPath(texturePath);
    if (normalized == m_skyboxTexturePath && m_skybox && !normalized.empty())
    {
        if (updateFileManager && m_fileManager)
        {
            m_fileManager->SetSkyboxTexture(m_skyboxTexturePath);
        }
        return;
    }

    if (!m_skybox)
    {
        m_skybox = std::make_unique<Skybox>();
    }

    // Initialize skybox if not already initialized
    if (!m_skybox->IsInitialized())
    {
        m_skybox->Init();
        // Automatically load shaders
        m_skybox->LoadShadersAutomatically();
    }

    std::string absolutePath = ResolveSkyboxAbsolutePath(normalized.empty() ? texturePath : normalized);

    if (!normalized.empty())
    {
        std::error_code fileErr;
        if (!fs::exists(absolutePath, fileErr))
        {
            TraceLog(LOG_WARNING, "Editor::SetSkyboxTexture() - Skybox texture not found: %s",
                     absolutePath.c_str());
            normalized.clear();
            absolutePath.clear();
        }
    }

    m_skyboxTexturePath = normalized;
    m_activeMetadata.skyboxTexture = m_skyboxTexturePath;

    if (!absolutePath.empty())
    {
        m_skybox->LoadMaterialTexture(absolutePath);
        TraceLog(LOG_INFO, "Editor::SetSkyboxTexture() - Loaded skybox from %s",
                 absolutePath.c_str());
    }
    else if (normalized.empty())
    {
        // Clear skybox - use skyColor instead
        m_clearColor = m_activeMetadata.skyColor;
    }

    if (updateFileManager && m_fileManager)
    {
        m_fileManager->SetSkyboxTexture(m_skyboxTexturePath);
    }
}

std::string Editor::NormalizeSkyboxPath(const std::string &texturePath) const
{
    if (texturePath.empty())
    {
        return "";
    }

    fs::path projectRoot(PROJECT_ROOT_DIR);
    fs::path input(texturePath);
    std::error_code ec;
    fs::path absolute = input.is_absolute() ? fs::weakly_canonical(input, ec)
                                            : fs::weakly_canonical(projectRoot / input, ec);

    if (ec)
    {
        absolute = input.is_absolute() ? input : projectRoot / input;
    }

    fs::path relative = fs::relative(absolute, projectRoot, ec);
    if (!ec && !relative.empty() && relative.generic_string() != ".")
    {
        return relative.generic_string();
    }

    return absolute.generic_string();
}

std::string Editor::ResolveSkyboxAbsolutePath(const std::string &texturePath) const
{
    if (texturePath.empty())
    {
        return "";
    }

    fs::path input(texturePath);
    std::error_code ec;

    if (input.is_absolute())
    {
        fs::path canonical = fs::weakly_canonical(input, ec);
        if (!ec && fs::exists(canonical))
        {
            return canonical.string();
        }
        if (fs::exists(input))
        {
            return input.string();
        }
    }

    fs::path projectRoot(PROJECT_ROOT_DIR);
    fs::path combined = projectRoot / input;
    fs::path canonicalCombined = fs::weakly_canonical(combined, ec);
    if (!ec && fs::exists(canonicalCombined))
    {
        return canonicalCombined.string();
    }
    if (fs::exists(combined))
    {
        return combined.string();
    }

    return (input.is_absolute() ? input : combined).string();
}

std::string Editor::GetSkyboxAbsolutePath() const
{
    if (m_skyboxTexturePath.empty())
    {
        return "";
    }
    return ResolveSkyboxAbsolutePath(m_skyboxTexturePath);
}

// The Editor class has been successfully refactored to use the Facade pattern.
// All major functionality has been delegated to subsystem managers.
// Remaining old code in this file needs to be cleaned up in future iterations.

void Editor::LoadSpawnTexture()
{
    if (m_spawnTextureLoaded)
    {
        return; // Already loaded
    }

    // Load spawn texture (only after window is initialized)
    std::string texturePath =
        std::string(PROJECT_ROOT_DIR) + "/resources/boxes/PlayerSpawnTexture.png";
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
            TraceLog(LOG_WARNING,
                     "Editor::LoadSpawnTexture() - Failed to load spawn texture from: %s",
                     texturePath.c_str());
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Editor::LoadSpawnTexture() - Spawn texture not found at: %s",
                 texturePath.c_str());
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
