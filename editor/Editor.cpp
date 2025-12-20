#include "editor/Editor.h"
#include "core/events/Event.h"
#include "editor/render/EditorRenderer.h"
#include "scene/resources/map/core/MapLoader.h"
#include "scene/resources/map/mapfilemanager/json/jsonMapFileManager.h"

// Subsystem implementations
#include "editor/mapgui/UIManager.h"
#include "editor/panels/AssetBrowserPanel.h"
#include "editor/panels/ConsolePanel.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ToolbarPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "editor/tool/ToolManager.h"

#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui/rlImGui.h>

#include "components/rendering/utils/RenderUtils.h"
#include <raymath.h>

// ECS and Simulation
#include "components/physics/collision/core/CollisionManager.h"
#include "core/Engine.h"
#include "core/ecs/ECSRegistry.h"
#include "core/ecs/Examples.h"
#include "core/ecs/components/PlayerComponent.h"
#include "core/ecs/components/RenderComponent.h"
#include <cstdlib>
#include <thread>

namespace fs = std::filesystem;

Editor::Editor(ChainedDecos::Ref<CameraController> cameraController,
               ChainedDecos::Ref<IModelLoader> modelLoader)
    // About m_gridsize i know it stupid
    : m_gridSize(99999), m_spawnTextureLoaded(false), m_skybox(std::make_unique<Skybox>()),
      m_cameraController(std::move(cameraController)), m_modelLoader(std::move(modelLoader))
{
    // Initialize spawn texture (will be loaded after window initialization)
    m_spawnTexture = {0};
    m_clearColor = DARKGRAY;

    InitializeSubsystems();
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
}

void Editor::InitializeSubsystems()
{
    // Initialize NFD once (before subsystems that use it)
    NFD_Init();

    // Initialize subsystems
    m_mapManager = std::make_unique<MapManager>();
    m_toolManager = std::make_unique<ToolManager>();

    // Create UIManager
    UIManagerConfig uiConfig;
    uiConfig.editor = this;
    m_uiManager = std::make_unique<EditorUIManager>(uiConfig);

    m_renderer = std::make_unique<EditorRenderer>(this, m_toolManager.get());

    // Initialize new panel system
    m_panelManager = std::make_unique<EditorPanelManager>(this);
    m_panelManager->AddPanel<ToolbarPanel>(this)->SetVisible(false);
    m_panelManager->AddPanel<HierarchyPanel>(this)->SetVisible(false);
    m_panelManager->AddPanel<InspectorPanel>(this)->SetVisible(false);
    m_panelManager->AddPanel<ViewportPanel>(this)->SetVisible(false);
    m_panelManager->AddPanel<AssetBrowserPanel>(this)->SetVisible(false);
    m_panelManager->AddPanel<ConsolePanel>(this)->SetVisible(false);
}

CameraController &Editor::GetCameraController()
{
    return *m_cameraController;
}
void Editor::Update()
{
    // Update camera controller bypass based on viewport focus/hover
    if (m_cameraController && m_panelManager)
    {
        auto viewport = m_panelManager->GetPanel<ViewportPanel>("Viewport");
        if (viewport)
        {
            // Allow camera to move if viewport is focused OR (hovered AND middle/right mouse button
            // is down)
            bool shouldBypass =
                viewport->IsFocused() ||
                (viewport->IsHovered() && (IsMouseButtonDown(1) || IsMouseButtonDown(2)));
            m_cameraController->SetInputCaptureBypass(shouldBypass);
        }
    }

    // Update camera controller
    if (m_cameraController)
        m_cameraController->Update();

    // Update camera in tool manager for gizmo calculations
    if (m_toolManager && m_cameraController)
    {
        m_toolManager->SetCamera(m_cameraController->GetCamera());
    }
}

void Editor::Render()
{
    // Use the appropriate camera based on current mode
    Camera3D &activeCamera =
        m_isInPlayMode ? RenderManager::Get().GetCamera() : m_cameraController->GetCamera();

    if (m_skybox && m_skybox->IsLoaded())
    {
        m_skybox->UpdateGammaFromConfig();
        m_skybox->DrawSkybox(activeCamera.position);
    }

    // Render all objects in the map
    const auto &objects = m_mapManager->GetGameMap().GetMapObjects();
    for (const auto &obj : objects)
    {
        RenderObject(obj);
    }

    // Draw grid only in editor mode
    if (!m_isInPlayMode)
    {
        DrawGrid(m_gridSize, 1.0f);
    }
}

void Editor::RenderObject(const MapObjectData &obj)
{
    if (!m_renderer)
        return;

    bool isSelected = (m_mapManager->GetSelectedObject() == &obj);

    // Handle spawn zone rendering separately
    if (obj.type == MapObjectType::SPAWN_ZONE)
    {
        m_renderer->RenderSpawnZoneWithTexture(m_spawnTexture, obj.position, obj.scale, obj.color,
                                               m_spawnTextureLoaded);

        if (isSelected)
        {
            DrawCubeWires(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, YELLOW);
        }
        return;
    }

    // Delegate rendering to EditorRenderer
    m_renderer->RenderObject(obj, isSelected);
}

Tool Editor::GetActiveTool() const
{
    return static_cast<Tool>(m_activeTool);
}

void Editor::SetActiveTool(Tool tool)
{
    m_activeTool = static_cast<int>(tool);
    if (m_toolManager)
    {
        m_toolManager->SetActiveTool(tool);
    }
}

void Editor::RenderImGui()
{
    bool welcomeActive = false;
    if (m_uiManager)
    {
        welcomeActive = m_uiManager->IsWelcomeScreenActive();
        m_uiManager->Render();
    }

    // Render new dockable panels
    if (m_panelManager && !welcomeActive)
    {
        m_panelManager->Render();
    }
}

void Editor::HandleInput()
{
    if (m_uiManager)
    {
        m_uiManager->HandleInput();
    }

    // Handle tool-specific input
    if (m_toolManager && m_cameraController)
    {
        // Skip editor tool interactions in Play Mode
        if (m_isInPlayMode)
            return;

        const ImGuiIO &io = ImGui::GetIO();

        bool viewportHovered = false;
        if (m_panelManager)
        {
            auto viewport = m_panelManager->GetPanel<ViewportPanel>("Viewport");
            if (viewport && viewport->IsHovered())
            {
                viewportHovered = true;
            }
        }

        // Allow input if no ImGui capture OR if we are specifically hovering the viewport
        if (!io.WantCaptureMouse || viewportHovered)
        {
            Ray ray = GetScreenToWorldRay(GetMousePosition(), m_cameraController->GetCamera());

            bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            bool mouseReleased = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
            bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            if (mousePressed)
            {
                m_toolManager->HandleToolInput(true, ray, *this);
            }
            else if (mouseReleased)
            {
                m_toolManager->HandleToolInput(false, ray, *this);
            }
            else if (mouseDown)
            {
                m_toolManager->UpdateTool(ray, *this);
            }
        }
    }
}

void Editor::AddObject(const MapObjectData &obj)
{
    m_mapManager->AddObject(obj);
}

void Editor::RemoveObject(int index)
{
    m_mapManager->RemoveObject(index);
}

void Editor::SelectObject(int index)
{
    m_mapManager->SelectObject(index);
}

void Editor::ClearSelection()
{
    m_mapManager->ClearSelection();
}

void Editor::ClearScene()
{
    m_mapManager->ClearScene();
}

void Editor::ClearObjects()
{
    m_mapManager->ClearObjects();
}

void Editor::CreateDefaultObject(MapObjectType type, const std::string &modelName)
{
    MapObjectData newObj;
    std::string typeStr;

    switch (type)
    {
    case MapObjectType::CUBE:
        typeStr = "Cube";
        break;
    case MapObjectType::SPHERE:
        typeStr = "Sphere";
        break;
    case MapObjectType::CYLINDER:
        typeStr = "Cylinder";
        break;
    case MapObjectType::MODEL:
        typeStr = modelName;
        newObj.modelName = modelName;
        break;
    case MapObjectType::SPAWN_ZONE:
        typeStr = "Spawn Zone";
        newObj.color = {255, 100, 100, 200}; // Semi-transparent red
        break;
    default:
        typeStr = "Object";
        break;
    }

    newObj.type = type;
    newObj.name = typeStr + " " + std::to_string(m_mapManager->GetGameMap().GetMapObjects().size());
    newObj.position = {0.0f, 0.0f, 0.0f};
    newObj.rotation = {0.0f, 0.0f, 0.0f};
    newObj.scale = {1.0f, 1.0f, 1.0f};
    if (type != MapObjectType::SPAWN_ZONE)
        newObj.color = WHITE;

    m_mapManager->AddObject(newObj);
}

void Editor::LoadAndSpawnModel(const std::string &path)
{
    namespace fs = std::filesystem;
    fs::path p(path);
    std::string modelName = p.stem().string();

    if (m_modelLoader)
    {
        // 1. Ensure model is loaded in the global model loader
        m_modelLoader->LoadSingleModel(modelName, path);

        // 2. Add to GameMap's internal model registry so it can be rendered/saved
        auto modelOpt = m_modelLoader->GetModelByName(modelName);
        if (modelOpt)
        {
            m_mapManager->GetGameMap().GetMapModelsMutable()[modelName] = modelOpt->get();
            TraceLog(LOG_INFO, "[Editor] Model '%s' registered for spawning", modelName.c_str());
        }
    }

    // 3. Create the object in the scene
    CreateDefaultObject(MapObjectType::MODEL, modelName);

    // 4. Mark scene as modified
    SetSceneModified(true);
}

void Editor::SaveMap(const std::string &filename)
{
    m_mapManager->SaveMap(filename);
}

void Editor::LoadMap(const std::string &filename)
{
    m_mapManager->LoadMap(filename);
    ApplyMetadata(m_mapManager->GetGameMap().GetMapMetaData());
}

void Editor::ApplyMetadata(const MapMetadata &metadata)
{
    m_mapManager->GetGameMap().SetMapMetaData(metadata);
    m_clearColor = metadata.skyColor;
    SetSkyboxTexture(metadata.skyboxTexture);
}

void Editor::SetSkybox(const std::string &name)
{
    SetSkyboxTexture(name);
}

void Editor::SetSkyboxTexture(const std::string &texturePath)
{
    // Avoid redundant loading if the texture is already set
    if (m_mapManager->GetGameMap().GetMapMetaData().skyboxTexture == texturePath && m_skybox &&
        m_skybox->IsLoaded())
    {
        return;
    }

    if (!texturePath.empty())
    {
        m_skybox->Init();
    }
    else
    {
        // Handle unloading
        if (m_skybox)
        {
            m_skybox->UnloadSkybox();
        }
        m_mapManager->GetGameMap().GetMapMetaDataMutable().skyboxTexture = "";
        return;
    }
    m_skybox->LoadMaterialTexture(texturePath);
    m_mapManager->GetGameMap().GetMapMetaDataMutable().skyboxTexture = texturePath;
    TraceLog(LOG_INFO, "[Editor] Applied skybox texture: %s", texturePath.c_str());
}

std::string Editor::GetSkyboxAbsolutePath() const
{
    const std::string &path = m_mapManager->GetGameMap().GetMapMetaData().skyboxTexture;
    if (path.empty())
        return "";
    return path;
}

MapObjectData *Editor::GetSelectedObject()
{
    return m_mapManager->GetSelectedObject();
}

void Editor::LoadSpawnTexture()
{
    if (m_spawnTextureLoaded)
        return;

    std::string texturePath =
        std::string(PROJECT_ROOT_DIR) + "/resources/boxes/playerSpawnTexture.png";
    if (FileExists(texturePath.c_str()))
    {
        m_spawnTexture = ::LoadTexture(texturePath.c_str());
        m_spawnTextureLoaded = (m_spawnTexture.id != 0);
    }
}

void Editor::PreloadModelsFromResources()
{
    std::string resourcesDir = std::string(PROJECT_ROOT_DIR) + "/resources";
    MapLoader loader;
    auto modelInfos = loader.LoadModelsFromDirectory(resourcesDir);

    for (const auto &info : modelInfos)
    {
        if (m_modelLoader)
        {
            m_modelLoader->LoadSingleModel(info.name, info.path);
        }
        // Also add to GameMap's models so they are available for rendering
        auto modelOpt = m_modelLoader->GetModelByName(info.name);
        if (modelOpt)
        {
            m_mapManager->GetGameMap().GetMapModelsMutable()[info.name] = modelOpt->get();
        }
    }
}

void Editor::SetGridSize(int size)
{
    m_gridSize = size;
}

int Editor::GetGridSize() const
{
    return m_gridSize;
}

ChainedDecos::Ref<IModelLoader> Editor::GetModelLoader()
{
    return m_modelLoader;
}

int Editor::GetSelectedObjectIndex() const
{
    return m_mapManager->GetSelectedIndex();
}

GameMap &Editor::GetGameMap()
{
    return m_mapManager->GetGameMap();
}

void Editor::SetSkyboxColor(Color color)
{
    m_clearColor = color;
    m_mapManager->GetGameMap().GetMapMetaDataMutable().skyColor = color;
    TraceLog(LOG_INFO, "[Editor] Applied skybox color");
}
const std::string &Editor::GetSkyboxTexture() const
{
    return m_mapManager->GetGameMap().GetMapMetaData().skyboxTexture;
}
bool Editor::HasSkybox() const
{
    return static_cast<bool>(m_skybox);
}
Skybox *Editor::GetSkybox() const
{
    return m_skybox.get();
}
Color Editor::GetClearColor() const
{
    return m_clearColor;
}
IToolManager *Editor::GetToolManager() const
{
    return m_toolManager.get();
}
IUIManager *Editor::GetUIManager() const
{
    return m_uiManager.get();
}
const std::string &Editor::GetCurrentMapPath() const
{
    return m_mapManager->GetCurrentMapPath();
}
bool Editor::IsSceneModified() const
{
    return m_mapManager->IsSceneModified();
}
void Editor::SetSceneModified(bool modified)
{
    m_mapManager->SetSceneModified(modified);
}

void Editor::OnEvent(ChainedDecos::Event &e)
{
    // Skip editor event handling in Play Mode
    if (m_isInPlayMode)
        return;

    if (m_cameraController)
    {
        m_cameraController->OnEvent(e);
    }
}

void Editor::StartPlayMode()
{
    if (m_isInPlayMode)
        return;

    TraceLog(LOG_INFO, "[Editor] Starting Play Mode...");

    // 1. Generate collisions for simulation
    auto &engine = Engine::Instance();
    auto collisionManager = engine.GetService<CollisionManager>();
    if (collisionManager)
    {
        collisionManager->ClearColliders();

        // Rebuild collisions from current map objects
        for (const auto &obj : m_mapManager->GetGameMap().GetMapObjects())
        {
            if (obj.type == MapObjectType::CUBE)
            {
                Vector3 halfSize = Vector3Scale(obj.scale, 0.5f);
                auto collision = std::make_shared<Collision>(obj.position, halfSize);
                collision->SetCollisionType(CollisionType::AABB_ONLY);
                collisionManager->AddCollider(collision);
            }
            else if (obj.type == MapObjectType::SPHERE)
            {
                // Diameter is scale.x, Radius is diameter * 0.5
                float radius = obj.scale.x * 0.5f;
                Vector3 halfSize = {radius, radius, radius};
                auto collision = std::make_shared<Collision>(obj.position, halfSize);
                collision->SetCollisionType(CollisionType::AABB_ONLY);
                collisionManager->AddCollider(collision);
            }
            else if (obj.type == MapObjectType::CYLINDER)
            {
                // Diameter is scale.x, Radius is diameter * 0.5
                float radius = obj.scale.x * 0.5f;
                float height = obj.scale.y;

                Vector3 halfSize = {radius, height * 0.5f, radius};
                auto collision = std::make_shared<Collision>(obj.position, halfSize);
                collision->SetCollisionType(CollisionType::AABB_ONLY);
                collisionManager->AddCollider(collision);
            }
            else if (obj.type == MapObjectType::PLANE)
            {
                // Plane size is scale.x by scale.z
                Vector3 halfSize = {obj.scale.x * 0.5f, 0.05f, obj.scale.z * 0.5f};
                auto collision = std::make_shared<Collision>(obj.position, halfSize);
                collision->SetCollisionType(CollisionType::AABB_ONLY);
                collisionManager->AddCollider(collision);
            }
            else if (obj.type == MapObjectType::MODEL)
            {
                if (!obj.modelName.empty())
                {
                    auto modelOpt = m_modelLoader->GetModelByName(obj.modelName);
                    if (modelOpt)
                    {
                        Matrix translation =
                            MatrixTranslate(obj.position.x, obj.position.y, obj.position.z);
                        Matrix scaleMatrix = MatrixScale(obj.scale.x, obj.scale.y, obj.scale.z);
                        Vector3 rot = {obj.rotation.x * DEG2RAD, obj.rotation.y * DEG2RAD,
                                       obj.rotation.z * DEG2RAD};
                        Matrix rotationMatrix = MatrixRotateXYZ(rot);
                        // Correct order for world matrix: T * R * S
                        Matrix transform = MatrixMultiply(
                            MatrixMultiply(translation, rotationMatrix), scaleMatrix);

                        auto collision = std::make_shared<Collision>();
                        collision->BuildFromModelWithType(const_cast<Model *>(&modelOpt->get()),
                                                          CollisionType::BVH_ONLY, transform);
                        collision->SetCollisionType(CollisionType::BVH_ONLY);
                        collisionManager->AddCollider(collision);
                    }
                }
            }
        }
        collisionManager->Initialize();
    }

    // 2. Spawn Player
    auto &engineInstance = Engine::Instance();
    auto models = engineInstance.GetService<IModelLoader>();
    if (models)
    {
        if (!models->GetModelByName("player_low").has_value())
        {
            std::string playerModelPath =
                std::string(PROJECT_ROOT_DIR) + "/resources/player_low.glb";
            if (!models->LoadSingleModel("player_low", playerModelPath))
            {
                // Try .gltf as fallback
                playerModelPath = std::string(PROJECT_ROOT_DIR) + "/resources/player_low.gltf";
                models->LoadSingleModel("player_low", playerModelPath);
            }
        }
    }

    // Determine spawn position: prefer Spawn Zones over metadata startPosition
    Vector3 spawnPos = {0, 0, 0};
    bool spawnZoneFound = false;

    const auto &mapObjects = m_mapManager->GetGameMap().GetMapObjects();
    for (const auto &obj : mapObjects)
    {
        if (obj.type == MapObjectType::SPAWN_ZONE)
        {
            spawnPos = obj.position;
            spawnPos.y += 1.0f; // Offset upwards to avoid spawning into the floor
            spawnZoneFound = true;
            TraceLog(LOG_INFO, "[Editor] Player spawned at Spawn Zone '%s' at (%.2f, %.2f, %.2f)",
                     obj.name.c_str(), spawnPos.x, spawnPos.y, spawnPos.z);
            break;
        }
    }

    if (!spawnZoneFound)
    {
        spawnPos = m_mapManager->GetGameMap().GetMapMetaData().startPosition;
        if (spawnPos.x == 0 && spawnPos.y == 0 && spawnPos.z == 0)
        {
            spawnPos = {0, 2, 0}; // Fallback
        }
    }

    Model *playerModelPtr = nullptr;
    auto modelOpt = m_modelLoader->GetModelByName("player_low");
    if (modelOpt.has_value())
        playerModelPtr = &modelOpt.value().get();

    float sensitivity = 0.15f;

    // Clear registry before spawning to ensure fresh state
    REGISTRY.clear();

    auto playerEntity =
        ECSExamples::CreatePlayer(spawnPos, playerModelPtr, 6.0f, 10.0f, sensitivity, spawnPos);

    // Apply visual offset and 3rd person camera defaults
    if (REGISTRY.valid(playerEntity))
    {
        if (REGISTRY.all_of<RenderComponent>(playerEntity))
        {
            auto &renderComp = REGISTRY.get<RenderComponent>(playerEntity);
            renderComp.offset = {0.0f, -1.0f, 0.0f};
            renderComp.tint = WHITE; // Use WHITE tint to preserve model textures
        }

        if (REGISTRY.all_of<PlayerComponent>(playerEntity))
        {
            auto &player = REGISTRY.get<PlayerComponent>(playerEntity);
            player.cameraPitch = 25.0f; // Standard 3rd person angle
            player.cameraDistance = 6.0f;
            player.cameraYaw = 0.0f;
        }

        // Initialize Camera in RenderManager
        auto &camera = RenderManager::Get().GetCamera();
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        camera.up = {0, 1, 0};
    }

    m_isInPlayMode = true;
}

void Editor::StopPlayMode()
{
    if (!m_isInPlayMode)
        return;

    TraceLog(LOG_INFO, "[Editor] Stopping Play Mode...");

    // 1. Clear simulation entities
    REGISTRY.clear();

    // 2. Clear collisions
    auto collisionManager = Engine::Instance().GetService<CollisionManager>();
    if (collisionManager)
    {
        collisionManager->ClearColliders();
    }

    m_isInPlayMode = false;
}

bool Editor::IsInPlayMode() const
{
    return m_isInPlayMode;
}

void Editor::BuildGame()
{
    TraceLog(LOG_INFO, "[Editor] Saving map before build...");
    SaveMap("");

    TraceLog(LOG_INFO, "[Editor] Starting build process in background...");

    // Using a thread to avoid freezing the editor UI during compilation
    std::thread(
        [this]()
        {
            // On Windows with MSVC/CMake, this will build the project
            // We assume the editor is running from the build directory
            int result = std::system("cmake --build .");

            if (result == 0)
            {
                TraceLog(LOG_INFO, "[Editor] Build COMPLETED successfully.");
            }
            else
            {
                TraceLog(LOG_ERROR, "[Editor] Build FAILED with exit code: %d", result);
            }
        })
        .detach();
}
