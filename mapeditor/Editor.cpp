#include "mapeditor/Editor.h"
#include "mapeditor/render/EditorRenderer.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/map/MapFileManager/Json/JsonMapFileManager.h"

// Subsystem implementations
#include "mapeditor/mapgui/UIManager.h"
#include "mapeditor/tool/ToolManager.h"

#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui/rlImGui.h>

#include "components/rendering/Utils/RenderUtils.h"
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
    const auto &objects = m_gameMap.GetMapObjects();
    for (const auto &obj : objects)
    {
        RenderObject(obj);
    }
}

void Editor::RenderObject(const MapObjectData &obj)
{
    if (!m_renderer)
        return;

    bool isSelected = false;
    if (m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_gameMap.GetMapObjects().size()))
    {
        isSelected = (&m_gameMap.GetMapObjects()[m_selectedIndex] == &obj);
    }

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
    m_gameMap.GetMapObjectsMutable().push_back(obj);
    m_isSceneModified = true;
}

void Editor::RemoveObject(int index)
{
    if (index >= 0 && index < static_cast<int>(m_gameMap.GetMapObjects().size()))
    {
        m_gameMap.GetMapObjectsMutable().erase(m_gameMap.GetMapObjectsMutable().begin() + index);
        if (m_selectedIndex == index)
            m_selectedIndex = -1;
        else if (m_selectedIndex > index)
            m_selectedIndex--;

        m_isSceneModified = true;
    }
}

void Editor::SelectObject(int index)
{
    if (index >= -1 && index < static_cast<int>(m_gameMap.GetMapObjects().size()))
    {
        m_selectedIndex = index;
    }
}

void Editor::ClearSelection()
{
    m_selectedIndex = -1;
}

void Editor::ClearScene()
{
    m_gameMap.GetMapObjectsMutable().clear();
    m_selectedIndex = -1;
    m_isSceneModified = true;
}

void Editor::ClearObjects()
{
    ClearScene();
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
    newObj.name = typeStr + " " + std::to_string(m_gameMap.GetMapObjects().size());
    newObj.position = {0.0f, 0.0f, 0.0f};
    newObj.rotation = {0.0f, 0.0f, 0.0f};
    newObj.scale = {1.0f, 1.0f, 1.0f};
    if (type != MapObjectType::SPAWN_ZONE)
        newObj.color = WHITE;

    AddObject(newObj);
}

void Editor::SaveMap(const std::string &filename)
{
    MapLoader loader;
    if (loader.SaveMap(m_gameMap, filename))
    {
        m_currentMapPath = filename;
        m_isSceneModified = false;
        TraceLog(LOG_INFO, "Map saved to %s", filename.c_str());
    }
    else
    {
        TraceLog(LOG_ERROR, "Failed to save map to %s", filename.c_str());
    }
}

void Editor::LoadMap(const std::string &filename)
{
    MapLoader loader;
    m_gameMap = loader.LoadMap(filename);

    // Check if map is valid (MapLoader::LoadMap returns an empty map on failure)
    if (!m_gameMap.GetMapMetaData().name.empty() || !m_gameMap.GetMapObjects().empty())
    {
        m_currentMapPath = filename;
        m_selectedIndex = -1;
        m_isSceneModified = false;

        ApplyMetadata(m_gameMap.GetMapMetaData());
        TraceLog(LOG_INFO, "Map loaded from %s", filename.c_str());
    }
    else
    {
        TraceLog(LOG_ERROR, "Failed to load map from %s", filename.c_str());
    }
}

void Editor::ApplyMetadata(const MapMetadata &metadata)
{
    m_gameMap.SetMapMetaData(metadata);
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
    if (m_gameMap.GetMapMetaData().skyboxTexture == texturePath && m_skybox && m_skybox->IsLoaded())
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
        m_gameMap.GetMapMetaDataMutable().skyboxTexture = "";
        return;
    }
    m_skybox->LoadMaterialTexture(texturePath);
    m_gameMap.GetMapMetaDataMutable().skyboxTexture = texturePath;
    TraceLog(LOG_INFO, "[Editor] Applied skybox texture: %s", texturePath.c_str());
}

std::string Editor::GetSkyboxAbsolutePath() const
{
    const std::string &path = m_gameMap.GetMapMetaData().skyboxTexture;
    if (path.empty())
        return "";
    return path;
}

MapObjectData *Editor::GetSelectedObject()
{
    if (m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_gameMap.GetMapObjects().size()))
    {
        return &m_gameMap.GetMapObjectsMutable()[m_selectedIndex];
    }
    return nullptr;
}

void Editor::LoadSpawnTexture()
{
    if (m_spawnTextureLoaded)
        return;

    std::string texturePath =
        std::string(PROJECT_ROOT_DIR) + "/resources/boxes/PlayerSpawnTexture.png";
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
            m_gameMap.GetMapModelsMutable()[info.name] = modelOpt->get();
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
    return m_selectedIndex;
}

GameMap &Editor::GetGameMap()
{
    return m_gameMap;
}

void Editor::SetSkyboxColor(Color color)
{
    m_clearColor = color;
    m_gameMap.GetMapMetaDataMutable().skyColor = color;
    TraceLog(LOG_INFO, "[Editor] Applied skybox color");
}
const std::string &Editor::GetSkyboxTexture() const
{
    return m_gameMap.GetMapMetaData().skyboxTexture;
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
    return m_currentMapPath;
}
bool Editor::IsSceneModified() const
{
    return m_isSceneModified;
}
void Editor::SetSceneModified(bool modified)
{
    m_isSceneModified = modified;
}
