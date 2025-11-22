//
//

#include "Editor.h"
#include "core/object/kernel/Core/Kernel.h"
#include "scene/resources/map/Core/MapLoader.h" // Include the new comprehensive map loader
#include "Engine/MapFileManager/Json/JsonMapFileManager.h"
#include "FileManager/MapObjectConverterEditor.h"
#include "Renderer/EditorRenderer.h"
#include "Utils/PathUtils.h"

// Subsystem implementations
#include "CameraManager/CameraManager.h"
#include "FileManager/FileManager.h"
#include "SceneManager/SceneManager.h"
#include "ToolManager/ToolManager.h"
#include "UIManager/UIManager.h"

// Model and rendering subsystems
#include "scene/resources/model/Core/Model.h"
#include "ModelManager/ModelManager.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui/imgui.h>
#include <iostream>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>

#include "servers/rendering/Utils/RenderUtils.h"
#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace fs = std::filesystem;


Editor::Editor(std::shared_ptr<CameraController> cameraController,
               std::unique_ptr<ModelLoader> modelLoader)
    : m_gridSizes(900), m_spawnTextureLoaded(false), m_skybox(std::make_unique<Skybox>()), m_skyboxTexturePath("")
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
    
    // Create UIManager with config
    UIManagerConfig uiConfig;
    uiConfig.editor = this;
    uiConfig.sceneManager = m_sceneManager.get();
    uiConfig.fileManager = m_fileManager.get();
    uiConfig.toolManager = m_toolManager.get();
    uiConfig.modelManager = m_modelManager.get();
    m_uiManager = std::make_unique<UIManager>(uiConfig);
    
    // Initialize renderer
    m_renderer = std::make_unique<EditorRenderer>(m_toolManager.get(), m_cameraManager.get(), m_modelManager.get());
                                              
}

void Editor::Update()
{
    // Update subsystems
    if (m_cameraManager)
        m_cameraManager->Update();

    // Update camera in tool manager for gizmo calculations
    if (m_toolManager && m_cameraManager)
    {
        m_toolManager->SetCamera(m_cameraManager->GetCamera());
    }
}

void Editor::Render()
{
    if (m_skybox && m_skybox->IsLoaded())
    {
        // Update gamma settings from config before rendering
        m_skybox->UpdateGammaFromConfig();
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
    if (!m_renderer)
        return;
        
    // Convert MapObject to MapObjectData using MapObjectConverterEditor
    MapObjectData data = MapObjectConverterEditor::MapObjectToMapObjectData(obj);

    // Handle spawn zone rendering separately
    if (data.type == MapObjectType::SPAWN_ZONE)
    {
        // Render spawn zone with texture
        const float spawnSize = 2.0f;
        Color spawnColor = obj.GetColor();
        m_renderer->RenderSpawnZoneWithTexture(m_spawnTexture, data.position, spawnSize, spawnColor, m_spawnTextureLoaded);

        // Additional editor-specific rendering: selection wireframe
        if (obj.IsSelected())
        {
            DrawCubeWires(data.position, spawnSize, spawnSize, spawnSize, YELLOW);
        }
        return; // Don't render spawn zone as regular object
    }

    // Delegate rendering to EditorRenderer
    m_renderer->RenderObject(obj, data, obj.IsSelected());
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
    // NFD returns absolute paths, so we can use the path directly
    // For relative paths from JSON files, ResolveSkyboxAbsolutePath will handle them
    if (texturePath == m_skyboxTexturePath && m_skybox && !texturePath.empty())
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
    // Init() automatically loads shaders, so shaders are ready before loading texture
    if (!m_skybox->IsInitialized())
    {
        m_skybox->Init();
    }

    // Resolve absolute path (handles both absolute from NFD and relative from JSON)
    std::string absolutePath = PathUtils::ResolveSkyboxAbsolutePath(texturePath);

    if (!texturePath.empty())
    {
        std::error_code fileErr;
        if (!fs::exists(absolutePath, fileErr))
        {
            TraceLog(LOG_WARNING, "Editor::SetSkyboxTexture() - Skybox texture not found: %s",
                     absolutePath.c_str());
            absolutePath.clear();
        }
    }

    // Store the original path (absolute from NFD or relative from JSON)
    m_skyboxTexturePath = texturePath;
    m_activeMetadata.skyboxTexture = m_skyboxTexturePath;

    if (!absolutePath.empty())
    {
        m_skybox->LoadMaterialTexture(absolutePath);
        TraceLog(LOG_INFO, "Editor::SetSkyboxTexture() - Loaded skybox from %s",
                 absolutePath.c_str());
    }
    else if (texturePath.empty())
    {
        // Clear skybox - use skyColor instead
        m_clearColor = m_activeMetadata.skyColor;
    }

    if (updateFileManager && m_fileManager)
    {
        m_fileManager->SetSkyboxTexture(m_skyboxTexturePath);
    }
}

std::string Editor::GetSkyboxAbsolutePath() const
{
    if (m_skyboxTexturePath.empty())
    {
        return "";
    }
    return PathUtils::ResolveSkyboxAbsolutePath(m_skyboxTexturePath);
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
