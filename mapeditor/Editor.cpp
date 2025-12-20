#include "mapeditor/Editor.h"
#include "core/events/Event.h"
#include "mapeditor/render/EditorRenderer.h"
#include "scene/resources/map/core/MapLoader.h"
#include "scene/resources/map/mapfilemanager/json/jsonMapFileManager.h"

// Subsystem implementations
#include "mapeditor/mapgui/UIManager.h"
#include "mapeditor/tool/ToolManager.h"

#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui/rlImGui.h>

#include "components/rendering/utils/RenderUtils.h"
#include <raymath.h>

namespace fs = std::filesystem;

Editor::Editor(ChainedDecos::Ref<CameraController> cameraController,
               ChainedDecos::Ref<IModelLoader> modelLoader)
    : m_gridSize(50), m_spawnTextureLoaded(false), m_skybox(std::make_unique<Skybox>()),
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
}

CameraController &Editor::GetCameraController()
{
    return *m_cameraController;
}
void Editor::Update()
{
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
    if (m_skybox && m_skybox->IsLoaded())
    {
        m_skybox->UpdateGammaFromConfig();
        m_skybox->DrawSkybox();
    }

    // Render all objects in the map
    const auto &objects = m_mapManager->GetGameMap().GetMapObjects();
    for (const auto &obj : objects)
    {
        RenderObject(obj);
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
        const float spawnSize = 2.0f;
        m_renderer->RenderSpawnZoneWithTexture(m_spawnTexture, obj.position, spawnSize, obj.color,
                                               m_spawnTextureLoaded);

        if (isSelected)
        {
            DrawCubeWires(obj.position, spawnSize, spawnSize, spawnSize, YELLOW);
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
    if (m_toolManager && m_cameraController)
    {
        const ImGuiIO &io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
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
    if (m_cameraController)
    {
        m_cameraController->OnEvent(e);
    }
}


